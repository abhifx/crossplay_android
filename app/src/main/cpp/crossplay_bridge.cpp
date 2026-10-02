#include "crossplay_bridge.h"
#include "android_hal_display.h"
#include "android_hal_storage.h"
#include "android_hal_input.h"
#include "android_hal_net.h"

#include <HalDisplay.h>
#include <HalGPIO.h>
#include <GfxRenderer.h>
#include <PaintClock.h>
#include <MappedInputManager.h>
#include <activities/Activity.h>
#include <activities/ActivityManager.h>
#include <activities/RenderLock.h>
#include <CrossPointSettings.h>
#include <CrossPointState.h>
#include <RecentBooksStore.h>
#include <OpdsServerStore.h>
#include <KOReaderCredentialStore.h>
#include <I18n.h>
#include <components/UITheme.h>
#include <builtinFonts/all.h>
#include <fontIds.h>
#include <apps_local/ui/ToyboxFonts.h>

#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <android/log.h>
#include <android/window.h>
#include <android_native_app_glue.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <FontCacheManager.h>
#include <FontDecompressor.h>

#undef LOGI
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "CrossPlayBridge", __VA_ARGS__)

extern HalDisplay display;
extern HalGPIO gpio;
extern GfxRenderer renderer;
extern MappedInputManager mappedInputManager;
extern ActivityManager activityManager;

static FontDecompressor fontDecompressor;
static FontCacheManager fontCacheManager(renderer.getFontMap(), renderer.getSdCardFonts(), renderer.getTtfFonts());

static bool g_forceRedraw = true;
static struct android_app* g_app = nullptr;
static bool g_isPaused = false;

static uint32_t* g_tempFb = nullptr;
static size_t g_tempFbCap = 0;
static uint32_t* g_portraitFb = nullptr;
static size_t g_portraitFbCap = 0;

static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;
static EGLContext g_eglContext = EGL_NO_CONTEXT;
static GLuint g_glProgram = 0;
static GLuint g_glTexture = 0;
static GLuint g_glVbo = 0;
static ANativeWindow* g_currentWindow = nullptr;

static const char* vShaderSource = R"(
    attribute vec4 aPosition;
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;
    void main() {
        gl_Position = aPosition;
        vTexCoord = aTexCoord;
    }
)";

static const char* fShaderSource = R"(
    precision mediump float;
    varying vec2 vTexCoord;
    uniform sampler2D uTexture;
    void main() {
        gl_FragColor = texture2D(uTexture, vTexCoord);
    }
)";

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen) {
            char* buf = (char*) malloc(infoLen);
            if (buf) {
                glGetShaderInfoLog(shader, infoLen, NULL, buf);
                LOGI("Could not compile shader %d:\n%s\n", type, buf);
                free(buf);
            }
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static void terminateEGL() {
    if (g_eglDisplay != EGL_NO_DISPLAY) {
        if (g_glVbo != 0) {
            glDeleteBuffers(1, &g_glVbo);
            g_glVbo = 0;
        }
        if (g_glTexture != 0) {
            glDeleteTextures(1, &g_glTexture);
            g_glTexture = 0;
        }
        if (g_glProgram != 0) {
            glDeleteProgram(g_glProgram);
            g_glProgram = 0;
        }

        eglMakeCurrent(g_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (g_eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(g_eglDisplay, g_eglSurface);
            g_eglSurface = EGL_NO_SURFACE;
        }
        if (g_eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(g_eglDisplay, g_eglContext);
            g_eglContext = EGL_NO_CONTEXT;
        }
        eglTerminate(g_eglDisplay);
        g_eglDisplay = EGL_NO_DISPLAY;
    }
    g_currentWindow = nullptr;
}

static bool initEGL(ANativeWindow* window) {
    if (!window || g_isPaused) return false;
    if (g_eglDisplay != EGL_NO_DISPLAY && g_eglSurface != EGL_NO_SURFACE && g_currentWindow == window) {
        eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext);
        return true;
    }

    if (g_eglDisplay != EGL_NO_DISPLAY) {
        terminateEGL();
    }
    g_currentWindow = window;

    // Configure native window buffer format for EGL 32-bit RGBA 8888
    ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBA_8888);

    g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_eglDisplay == EGL_NO_DISPLAY) return false;

    if (!eglInitialize(g_eglDisplay, nullptr, nullptr)) return false;

    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(g_eglDisplay, configAttribs, &config, 1, &numConfigs) || numConfigs <= 0) return false;

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    g_eglContext = eglCreateContext(g_eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (g_eglContext == EGL_NO_CONTEXT) return false;

    g_eglSurface = eglCreateWindowSurface(g_eglDisplay, config, window, nullptr);
    if (g_eglSurface == EGL_NO_SURFACE) return false;

    if (eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext) == EGL_FALSE) {
        LOGI("Failed eglMakeCurrent");
        return false;
    }

    GLuint vShader = compileShader(GL_VERTEX_SHADER, vShaderSource);
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fShaderSource);

    if (!vShader || !fShader) return false;

    g_glProgram = glCreateProgram();
    glAttachShader(g_glProgram, vShader);
    glAttachShader(g_glProgram, fShader);

    glBindAttribLocation(g_glProgram, 0, "aPosition");
    glBindAttribLocation(g_glProgram, 1, "aTexCoord");

    glLinkProgram(g_glProgram);

    GLint linkStatus = GL_FALSE;
    glGetProgramiv(g_glProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
        LOGI("Could not link program");
        glDeleteProgram(g_glProgram);
        g_glProgram = 0;
        return false;
    }

    glGenTextures(1, &g_glTexture);
    glBindTexture(GL_TEXTURE_2D, g_glTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    int logW = AndroidHalDisplay::getInstance().getLogicalWidth();
    int logH = AndroidHalDisplay::getInstance().getLogicalHeight();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logW, logH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenBuffers(1, &g_glVbo);

    LOGI("EGL / OpenGL ES 2.0 initialized successfully!");
    return true;
}

static void initEngineFonts(GfxRenderer& renderer) {
    static EpdFont notoserif14RegularFont(&notoserif_14_regular);
    static EpdFont notoserif14BoldFont(&notoserif_14_bold);
    static EpdFont notoserif14ItalicFont(&notoserif_14_italic);
    static EpdFont notoserif14BoldItalicFont(&notoserif_14_bolditalic);
    static EpdFontFamily notoserif14FontFamily(&notoserif14RegularFont, &notoserif14BoldFont, &notoserif14ItalicFont, &notoserif14BoldItalicFont);

    static EpdFont notoserif12RegularFont(&notoserif_12_regular);
    static EpdFont notoserif12BoldFont(&notoserif_12_bold);
    static EpdFont notoserif12ItalicFont(&notoserif_12_italic);
    static EpdFont notoserif12BoldItalicFont(&notoserif_12_bolditalic);
    static EpdFontFamily notoserif12FontFamily(&notoserif12RegularFont, &notoserif12BoldFont, &notoserif12ItalicFont, &notoserif12BoldItalicFont);

    static EpdFont notoserif16RegularFont(&notoserif_16_regular);
    static EpdFont notoserif16BoldFont(&notoserif_16_bold);
    static EpdFont notoserif16ItalicFont(&notoserif_16_italic);
    static EpdFont notoserif16BoldItalicFont(&notoserif_16_bolditalic);
    static EpdFontFamily notoserif16FontFamily(&notoserif16RegularFont, &notoserif16BoldFont, &notoserif16ItalicFont, &notoserif16BoldItalicFont);

    static EpdFont notoserif18RegularFont(&notoserif_18_regular);
    static EpdFont notoserif18BoldFont(&notoserif_18_bold);
    static EpdFont notoserif18ItalicFont(&notoserif_18_italic);
    static EpdFont notoserif18BoldItalicFont(&notoserif_18_bolditalic);
    static EpdFontFamily notoserif18FontFamily(&notoserif18RegularFont, &notoserif18BoldFont, &notoserif18ItalicFont, &notoserif18BoldItalicFont);

    static EpdFont notosans12RegularFont(&notosans_12_regular);
    static EpdFont notosans12BoldFont(&notosans_12_bold);
    static EpdFont notosans12ItalicFont(&notosans_12_italic);
    static EpdFont notosans12BoldItalicFont(&notosans_12_bolditalic);
    static EpdFontFamily notosans12FontFamily(&notosans12RegularFont, &notosans12BoldFont, &notosans12ItalicFont, &notosans12BoldItalicFont);

    static EpdFont notosans14RegularFont(&notosans_14_regular);
    static EpdFont notosans14BoldFont(&notosans_14_bold);
    static EpdFont notosans14ItalicFont(&notosans_14_italic);
    static EpdFont notosans14BoldItalicFont(&notosans_14_bolditalic);
    static EpdFontFamily notosans14FontFamily(&notosans14RegularFont, &notosans14BoldFont, &notosans14ItalicFont, &notosans14BoldItalicFont);

    static EpdFont notosans16RegularFont(&notosans_16_regular);
    static EpdFont notosans16BoldFont(&notosans_16_bold);
    static EpdFont notosans16ItalicFont(&notosans_16_italic);
    static EpdFontFamily notosans16FontFamily(&notosans16RegularFont, &notosans16BoldFont, &notosans16ItalicFont);

    static EpdFont notosans18RegularFont(&notosans_18_regular);
    static EpdFont notosans18BoldFont(&notosans_18_bold);
    static EpdFont notosans18ItalicFont(&notosans_18_italic);
    static EpdFont notosans18BoldItalicFont(&notosans_18_bolditalic);
    static EpdFontFamily notosans18FontFamily(&notosans18RegularFont, &notosans18BoldFont, &notosans18ItalicFont, &notosans18BoldItalicFont);

    static EpdFont ui10RegularFont(&ubuntu_10_regular);
    static EpdFont ui10BoldFont(&ubuntu_10_bold);
    static EpdFontFamily ui10FontFamily(&ui10RegularFont, &ui10BoldFont);

    static EpdFont ui12RegularFont(&ubuntu_12_regular);
    static EpdFont ui12BoldFont(&ubuntu_12_bold);
    static EpdFontFamily ui12FontFamily(&ui12RegularFont, &ui12BoldFont);

    static EpdFont smallRegularFont(&notosans_8_regular);
    static EpdFontFamily smallFontFamily(&smallRegularFont);

    renderer.insertFont(NOTOSERIF_12_FONT_ID, notoserif12FontFamily);
    renderer.insertFont(NOTOSERIF_14_FONT_ID, notoserif14FontFamily);
    renderer.insertFont(NOTOSERIF_16_FONT_ID, notoserif16FontFamily);
    renderer.insertFont(NOTOSERIF_18_FONT_ID, notoserif18FontFamily);

    renderer.insertFont(NOTOSANS_12_FONT_ID, notosans12FontFamily);
    renderer.insertFont(NOTOSANS_14_FONT_ID, notosans14FontFamily);
    renderer.insertFont(NOTOSANS_16_FONT_ID, notosans16FontFamily);
    renderer.insertFont(NOTOSANS_18_FONT_ID, notosans18FontFamily);

    renderer.insertFont(UI_10_FONT_ID, ui10FontFamily);
    renderer.insertFont(UI_12_FONT_ID, ui12FontFamily);
    renderer.insertFont(SMALL_FONT_ID, smallFontFamily);

    toybox::ensureFonts(renderer);
}

std::string getInternalDir(JNIEnv* env, jobject activityObj) {
    jclass clazz = env->GetObjectClass(activityObj);
    if (!clazz) return "/data/local/tmp";
    jmethodID getFilesDir = env->GetMethodID(clazz, "getFilesDir", "()Ljava/io/File;");
    if (!getFilesDir) {
        env->DeleteLocalRef(clazz);
        return "/data/local/tmp";
    }
    jobject fileObj = env->CallObjectMethod(activityObj, getFilesDir);
    if (!fileObj) {
        env->DeleteLocalRef(clazz);
        return "/data/local/tmp";
    }
    jclass fileClass = env->GetObjectClass(fileObj);
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring pathStr = (jstring)env->CallObjectMethod(fileObj, getAbsolutePath);
    const char* path = env->GetStringUTFChars(pathStr, nullptr);
    std::string result(path ? path : "/data/local/tmp");
    if (path) env->ReleaseStringUTFChars(pathStr, path);
    if (pathStr) env->DeleteLocalRef(pathStr);
    if (fileClass) env->DeleteLocalRef(fileClass);
    if (fileObj) env->DeleteLocalRef(fileObj);
    env->DeleteLocalRef(clazz);
    return result;
}

static bool g_frameBufferReady = false;

void setForceRedraw(bool force) {
    g_forceRedraw = force;
}

void setFrameBufferReady(bool ready) {
    g_frameBufferReady = ready;
    g_forceRedraw = true;
}

std::string getExternalDir(JNIEnv* env, jobject activityObj) {
    jclass clazz = env->GetObjectClass(activityObj);
    if (!clazz) return "/sdcard";
    jmethodID getExtFilesDir = env->GetMethodID(clazz, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    if (!getExtFilesDir) {
        env->DeleteLocalRef(clazz);
        return "/sdcard";
    }
    jobject fileObj = env->CallObjectMethod(activityObj, getExtFilesDir, nullptr);
    if (!fileObj) {
        env->DeleteLocalRef(clazz);
        return "/sdcard";
    }
    jclass fileClass = env->GetObjectClass(fileObj);
    jmethodID getAbsolutePath = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring pathStr = (jstring)env->CallObjectMethod(fileObj, getAbsolutePath);
    const char* path = env->GetStringUTFChars(pathStr, nullptr);
    std::string result(path ? path : "/sdcard");
    if (path) env->ReleaseStringUTFChars(pathStr, path);
    if (pathStr) env->DeleteLocalRef(pathStr);
    if (fileClass) env->DeleteLocalRef(fileClass);
    if (fileObj) env->DeleteLocalRef(fileObj);
    env->DeleteLocalRef(clazz);
    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_crosspoint_crossplay_MainActivity_nativeSwitchApp(
    JNIEnv* env, jobject thiz, jint appId) {

    g_forceRedraw = true;
    if (appId == 0) {
        activityManager.goHome();
    } else if (appId == 1) {
        activityManager.goToBrowser();
    } else if (appId == 3) {
        activityManager.goToSettings();
    } else {
        activityManager.goHome();
    }
}

void renderEngineFrame() {
    if (g_isPaused || !g_app) return;

    ANativeWindow* win = g_app->window;
    if (!win || g_isPaused) return;

    if (!initEGL(win)) {
        g_isPaused = true;
        return;
    }
    if (g_eglSurface == EGL_NO_SURFACE || g_glVbo == 0 || g_isPaused) return;

    if (g_glVbo == 0) {
        glGenBuffers(1, &g_glVbo);
    }
    if (g_glVbo == 0) return;

    int w = ANativeWindow_getWidth(win);
    int h = ANativeWindow_getHeight(win);
    if (w <= 0 || h <= 0) return;

    int targetLogicalW = 480;
    int targetLogicalH = 800;
    if (w >= 1080 || h >= 1800) {
        targetLogicalW = 720;
        targetLogicalH = 1200;
    }
    int targetPhysW = targetLogicalH;
    int targetPhysH = targetLogicalW;

    if (AndroidHalDisplay::getInstance().getLogicalWidth() != targetLogicalW ||
        AndroidHalDisplay::getInstance().getLogicalHeight() != targetLogicalH) {
        HalDisplay::setResolution(targetPhysW, targetPhysH);
        display.begin(true);
        renderer.begin();
        renderer.setOrientation(GfxRenderer::Portrait);
        initEngineFonts(renderer);
        delete[] g_tempFb; g_tempFb = nullptr; g_tempFbCap = 0;
        delete[] g_portraitFb; g_portraitFb = nullptr; g_portraitFbCap = 0;
        glBindTexture(GL_TEXTURE_2D, g_glTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, targetLogicalW, targetLogicalH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        activityManager.requestUpdate(true);
        g_forceRedraw = true;
    }

    uint8_t* fb = nullptr;
    {
        RenderLock lock;
        fb = renderer.getFrameBuffer();
        if (!fb) fb = display.getFrameBuffer();
        if (!fb || g_isPaused) return;
    }

    int logW = AndroidHalDisplay::getInstance().getLogicalWidth();
    int logH = AndroidHalDisplay::getInstance().getLogicalHeight();
    int physW = AndroidHalDisplay::getInstance().getDisplayWidth();
    int physH = AndroidHalDisplay::getInstance().getDisplayHeight();

    size_t reqTempCap = static_cast<size_t>(physW * physH);
    if (!g_tempFb || g_tempFbCap < reqTempCap) {
        delete[] g_tempFb;
        g_tempFb = new uint32_t[reqTempCap];
        g_tempFbCap = reqTempCap;
    }

    size_t reqPortraitCap = static_cast<size_t>(logW * logH);
    if (!g_portraitFb || g_portraitFbCap < reqPortraitCap) {
        delete[] g_portraitFb;
        g_portraitFb = new uint32_t[reqPortraitCap];
        g_portraitFbCap = reqPortraitCap;
    }

    // 1. Convert physical buffer to 32-bit ARGB
    AndroidHalDisplay::getInstance().renderToPixelArray(fb, g_tempFb);

    // 2. Un-rotate physical buffer to logical portrait frame (RGBA format for OpenGL)
    for (int y = 0; y < logH; ++y) {
        for (int x = 0; x < logW; ++x) {
            int phyX = y;
            int phyY = (logW - 1) - x;
            uint32_t argb = g_tempFb[phyY * physW + phyX];
            uint8_t a = (argb >> 24) & 0xFF;
            uint8_t r = (argb >> 16) & 0xFF;
            uint8_t g = (argb >> 8) & 0xFF;
            uint8_t b = argb & 0xFF;
            // Pack as ABGR for GL_RGBA
            g_portraitFb[y * logW + x] = (static_cast<uint32_t>(a) << 24) |
                                        (static_cast<uint32_t>(b) << 16) |
                                        (static_cast<uint32_t>(g) << 8)  |
                                        static_cast<uint32_t>(r);
        }
    }

    // 3. Upload texture
    glUseProgram(g_glProgram);
    GLint uTex = glGetUniformLocation(g_glProgram, "uTexture");
    if (uTex >= 0) glUniform1i(uTex, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_glTexture);

    static int g_lastTexW = 0, g_lastTexH = 0;
    if (g_lastTexW != logW || g_lastTexH != logH) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logW, logH, 0, GL_RGBA, GL_UNSIGNED_BYTE, g_portraitFb);
        g_lastTexW = logW;
        g_lastTexH = logH;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, logW, logH, GL_RGBA, GL_UNSIGNED_BYTE, g_portraitFb);
    }

    // 4. Render aspect-ratio scaled full-screen quad via OpenGL ES 2.0 VBO
    glViewport(0, 0, w, h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    float screenAspect = (float)w / (float)h;
    float targetAspect = static_cast<float>(logW) / static_cast<float>(logH);

    float scaleX = 1.0f;
    float scaleY = 1.0f;

    if (screenAspect > targetAspect) {
        scaleX = targetAspect / screenAspect;
    } else {
        scaleY = screenAspect / targetAspect;
    }

    GLfloat vertices[] = {
        -scaleX,  scaleY,  0.0f, 0.0f,
        -scaleX, -scaleY,  0.0f, 1.0f,
         scaleX,  scaleY,  1.0f, 0.0f,
         scaleX, -scaleY,  1.0f, 1.0f,
    };

    glBindBuffer(GL_ARRAY_BUFFER, g_glVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (eglSwapBuffers(g_eglDisplay, g_eglSurface) == EGL_FALSE) {
        LOGI("eglSwapBuffers failed, terminating EGL");
        terminateEGL();
    } else {
        renderer.waitRefreshComplete();
    }
}

static int32_t handleInputEvent(struct android_app* app, AInputEvent* event) {
    if (g_isPaused) return 0;

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);

        if (app->window) {
            int w = ANativeWindow_getWidth(app->window);
            int h = ANativeWindow_getHeight(app->window);
            if (w <= 0 || h <= 0) return 1;

            int logW = AndroidHalDisplay::getInstance().getLogicalWidth();
            int logH = AndroidHalDisplay::getInstance().getLogicalHeight();
            int physW = AndroidHalDisplay::getInstance().getDisplayWidth();
            int physH = AndroidHalDisplay::getInstance().getDisplayHeight();

            float screenAspect = (float)w / (float)h;
            float targetAspect = static_cast<float>(logW) / static_cast<float>(logH);

            float scaleX = 1.0f;
            float scaleY = 1.0f;

            if (screenAspect > targetAspect) {
                scaleX = targetAspect / screenAspect;
            } else {
                scaleY = screenAspect / targetAspect;
            }

            // Map NDC touch (-1..1) to logical frame
            float ndcX = (x / (float)w) * 2.0f - 1.0f;
            float ndcY = 1.0f - (y / (float)h) * 2.0f;

            float lx = ((ndcX / scaleX) + 1.0f) * 0.5f * static_cast<float>(logW);
            float ly = (1.0f - (ndcY / scaleY)) * 0.5f * static_cast<float>(logH);

            if (lx >= 0 && lx < logW && ly >= 0 && ly < logH) {
                // Map logical portrait touch (lx, ly) to physical panel (phyX, phyY)
                float phyX = ly;
                float phyY = static_cast<float>(logW - 1) - lx;

                int einkAction = 1; // Move
                if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) einkAction = 0;
                else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_CANCEL) einkAction = 2;

                AndroidHalInput::getInstance().handleTouchEvent(einkAction, phyX, phyY, physW, physH);
                if (einkAction != 1) { // Redraw on DOWN and UP/CANCEL, avoid jitter re-renders on MOVE
                    g_forceRedraw = true;
                    activityManager.requestUpdate();
                }
            }
        }
        return 1;
    } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        int32_t keyCode = AKeyEvent_getKeyCode(event);
        int32_t action = AKeyEvent_getAction(event);
        bool isDown = (action == AKEY_EVENT_ACTION_DOWN);
        AndroidHalInput::getInstance().handleKeyEvent(keyCode, isDown);
        g_forceRedraw = true;
        activityManager.requestUpdate();
        return 1;
    }
    return 0;
}

static bool g_engineInitialized = false;

static void initEngineWithWindowSize(int w, int h) {
    int targetLogicalW = 480;
    int targetLogicalH = 800;
    if (w >= 1080 || h >= 1800) {
        targetLogicalW = 720;
        targetLogicalH = 1200;
    }
    int targetPhysW = targetLogicalH;
    int targetPhysH = targetLogicalW;

    HalDisplay::setResolution(targetPhysW, targetPhysH);
    display.begin(true);
    renderer.begin();
    renderer.setOrientation(GfxRenderer::Portrait);
    initEngineFonts(renderer);
    if (fontDecompressor.init()) {
        fontCacheManager.setFontDecompressor(&fontDecompressor);
    }
    renderer.setFontCacheManager(&fontCacheManager);
    if (gpio.hasTouch()) {
        SETTINGS.readerMenuStyle = CrossPointSettings::READER_MENU_TOOLBAR;
    }
    SETTINGS.loadFromFile();
    APP_STATE.loadFromFile();
    RECENT_BOOKS.loadFromFile();
    OPDS_STORE.loadFromFile();
    KOREADER_STORE.loadFromFile();

    I18N.setLanguage(static_cast<Language>(SETTINGS.language));
    UITheme::getInstance().reload();

    activityManager.begin();
    activityManager.goHome();
    activityManager.requestUpdate(true);
    g_engineInitialized = true;
}

static void handleCmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != nullptr) {
                int w = ANativeWindow_getWidth(app->window);
                int h = ANativeWindow_getHeight(app->window);
                if (w > 0 && h > 0) {
                    if (!g_engineInitialized) {
                        initEngineWithWindowSize(w, h);
                    }
                }
                g_isPaused = false;
                g_forceRedraw = true;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            g_isPaused = true;
            terminateEGL();
            break;
        case APP_CMD_LOST_FOCUS:
        case APP_CMD_PAUSE:
            g_isPaused = true;
            break;
        case APP_CMD_STOP:
            g_isPaused = true;
            terminateEGL();
            break;
        case APP_CMD_CONFIG_CHANGED:
            if (app->window != nullptr) {
                int w = ANativeWindow_getWidth(app->window);
                int h = ANativeWindow_getHeight(app->window);
                if (w > 0 && h > 0) {
                    display.begin(true);
                    renderer.begin();
                    activityManager.requestUpdate(true);
                }
            }
            g_forceRedraw = true;
            break;
        case APP_CMD_GAINED_FOCUS:
        case APP_CMD_RESUME:
            g_isPaused = false;
            g_forceRedraw = true;
            break;
    }
}

void android_main(struct android_app* state) {
    g_app = state;
    state->onAppCmd = handleCmd;
    state->onInputEvent = handleInputEvent;

    JNIEnv* env;
    state->activity->vm->AttachCurrentThread(&env, nullptr);
    AndroidHalNet::getInstance().setJniEnv(state->activity->vm, state->activity->clazz);
    AndroidHalDisplay::getInstance().setJniEnv(state->activity->vm, state->activity->clazz);

    std::string internalDir = getInternalDir(env, state->activity->clazz);
    std::string externalDir = getExternalDir(env, state->activity->clazz);

    LOGI("NativeActivity Internal: %s, External: %s", internalDir.c_str(), externalDir.c_str());

    AndroidHalStorage::getInstance().initialize(internalDir.c_str(), externalDir.c_str());
    AndroidHalDisplay::getInstance().initialize();

    if (state->window != nullptr) {
        int w = ANativeWindow_getWidth(state->window);
        int h = ANativeWindow_getHeight(state->window);
        if (w > 0 && h > 0) {
            initEngineWithWindowSize(w, h);
        }
    }

    g_forceRedraw = true;

    while (1) {
        int ident;
        int events;
        struct android_poll_source* source;

        // Non-blocking poll: process all queued events then proceed to render
        while ((ident = ALooper_pollOnce(0, nullptr, &events, (void**)&source)) >= 0) {
            if (source != nullptr) {
                source->process(state, source);
            }
            if (state->destroyRequested != 0) {
                terminateEGL();
                state->activity->vm->DetachCurrentThread();
                return;
            }
        }

        if (g_engineInitialized && state->window != nullptr && !g_isPaused) {
            AndroidHalInput::getInstance().advanceFrame();
            gpio.update();
            activityManager.loop();

            if (g_frameBufferReady || g_forceRedraw) {
                renderEngineFrame();
                g_frameBufferReady = false;
                g_forceRedraw = false;
            }
        }

        // Throttle idle loop to ~250 Hz
        usleep(4000);
    }
}
