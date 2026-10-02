#include "android_hal_display.h"
#include "android_hal_input.h"
#include <HalDisplay.h>
#include <algorithm>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "AndroidHalDisplay"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AndroidHalDisplay& AndroidHalDisplay::getInstance() {
    static AndroidHalDisplay instance;
    return instance;
}

void AndroidHalDisplay::initialize() {
    m_flashFrameCount = 0;
    m_enhancedGrayscale = true;
    m_brightness = 100;
    m_warmth = 0;
    m_lightOn = true;
    setDisplayResolution(800, 480);
}

void AndroidHalDisplay::setJniEnv(JavaVM* jvm, jobject mainActivityObj) {
    m_jvm = jvm;
    if (m_activityObj && jvm) {
        JNIEnv* env = nullptr;
        jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (env) env->DeleteGlobalRef(m_activityObj);
    }
    if (jvm && mainActivityObj) {
        JNIEnv* env = nullptr;
        jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (env) {
            m_activityObj = env->NewGlobalRef(mainActivityObj);
        }
    }
}

void AndroidHalDisplay::setBrightness(uint8_t percent) {
    m_brightness = percent > 100 ? 100 : percent;
    updateSystemBrightness();
}

void AndroidHalDisplay::setWarmth(uint8_t warmPercent) {
    m_warmth = warmPercent > 100 ? 100 : warmPercent;
}

void AndroidHalDisplay::setLightOn(bool on) {
    m_lightOn = on;
    updateSystemBrightness();
}

void AndroidHalDisplay::updateSystemBrightness() {
    if (!m_jvm || !m_activityObj) return;

    JNIEnv* env = nullptr;
    bool needsDetach = false;
    jint getEnvRes = m_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (getEnvRes == JNI_EDETACHED) {
        if (m_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        needsDetach = true;
    } else if (getEnvRes != JNI_OK || !env) {
        return;
    }

    jclass cls = env->GetObjectClass(m_activityObj);
    if (cls) {
        jmethodID methodId = env->GetMethodID(cls, "setScreenBrightnessFromNative", "(I)V");
        if (methodId) {
            int percent = m_lightOn ? m_brightness : 0;
            env->CallVoidMethod(m_activityObj, methodId, percent);
        }
        env->DeleteLocalRef(cls);
    }

    if (needsDetach) m_jvm->DetachCurrentThread();
}

void AndroidHalDisplay::setDisplayResolution(int physW, int physH) {
    if (physW <= 0 || physH <= 0) return;
    m_displayWidth = physW;
    m_displayHeight = physH;
    m_logicalWidth = physH;
    m_logicalHeight = physW;
    m_displayWidthBytes = (m_displayWidth + 7) / 8;

    m_bwBuffer.assign(m_displayWidthBytes * m_displayHeight, 0xFF);
    m_grayBuffer.assign(m_displayWidth * m_displayHeight, 0xFF);
    m_lsbBuffer.assign(m_displayWidthBytes * m_displayHeight, 0xFF);
    m_msbBuffer.assign(m_displayWidthBytes * m_displayHeight, 0xFF);

    AndroidHalInput::getInstance().setDisplayResolution(physW, physH);
}

void AndroidHalDisplay::triggerFlashRefresh() {
    m_flashFrameCount = 0;
}

void AndroidHalDisplay::setGrayscaleBuffers(const uint8_t* lsb, const uint8_t* msb) {
    if (!lsb || !msb) {
        m_hasGrayscale = false;
        return;
    }
    size_t planeBytes = m_displayWidthBytes * m_displayHeight;
    if (m_lsbBuffer.size() >= planeBytes && m_msbBuffer.size() >= planeBytes) {
        memcpy(m_lsbBuffer.data(), lsb, planeBytes);
        memcpy(m_msbBuffer.data(), msb, planeBytes);
        m_hasGrayscale = true;
    }
}

void AndroidHalDisplay::drawPixelPhysical(int phyX, int phyY, uint8_t gray) {
    if (phyX >= 0 && phyX < m_displayWidth && phyY >= 0 && phyY < m_displayHeight) {
        if (!m_grayBuffer.empty()) {
            m_grayBuffer[phyY * m_displayWidth + phyX] = gray;
            m_hasGrayPixels = true;
        }
    }
}

void AndroidHalDisplay::drawPixelGray(int x, int y, uint8_t gray) {
    if (x < 0 || x >= m_logicalWidth || y < 0 || y >= m_logicalHeight) return;
    int phyX = y;
    int phyY = (m_logicalWidth - 1) - x;
    if (phyX >= 0 && phyX < m_displayWidth && phyY >= 0 && phyY < m_displayHeight) {
        if (!m_grayBuffer.empty()) {
            m_grayBuffer[phyY * m_displayWidth + phyX] = gray;
            m_hasGrayPixels = true;
        }
    }
}

void AndroidHalDisplay::clearGrayBuffer() {
    if (!m_grayBuffer.empty()) {
        memset(m_grayBuffer.data(), 0xFF, m_grayBuffer.size());
    }
    m_hasGrayPixels = false;
}

void AndroidHalDisplay::renderToPixelArray(const uint8_t* bwBuf, uint32_t* outPixels) {
    if (!bwBuf || !outPixels) return;

    const int dispW = m_displayWidth;
    const int dispH = m_displayHeight;
    const int dispWidthBytes = m_displayWidthBytes;

    if (m_flashFrameCount > 0) {
        m_flashFrameCount--;
        uint32_t flashColor = (m_flashFrameCount % 2 == 0) ? 0xFF000000 : 0xFFFFFFFF;
        std::fill(outPixels, outPixels + (dispW * dispH), flashColor);
        return;
    }

    const bool inverted = display.isInverted();
    const uint8_t curBrightness = m_lightOn ? m_brightness : 100;
    const uint8_t curWarmth = m_warmth;

    float brightFactor = curBrightness / 100.0f;
    if (brightFactor < 0.1f) brightFactor = 0.1f;

    float rFactor = brightFactor;
    float gFactor = brightFactor * (1.0f - (curWarmth / 100.0f) * 0.15f);
    float bFactor = brightFactor * (1.0f - (curWarmth / 100.0f) * 0.40f);

    if (m_hasGrayPixels && !m_grayBuffer.empty()) {
        for (int y = 0; y < dispH; ++y) {
            for (int x = 0; x < dispW; ++x) {
                int byteIdx = (y * dispWidthBytes) + (x / 8);
                int bitIdx = 7 - (x % 8);
                bool bwWhite = (bwBuf[byteIdx] >> bitIdx) & 1;
                uint8_t grayVal = m_grayBuffer[y * dispW + x];

                uint8_t finalGray = 0;
                if (!bwWhite) {
                    finalGray = 0;
                } else {
                    finalGray = (grayVal == 0) ? 255 : grayVal;
                }
                if (inverted) {
                    finalGray = 255 - finalGray;
                }

                uint8_t r = static_cast<uint8_t>(std::min(255.0f, finalGray * rFactor));
                uint8_t g = static_cast<uint8_t>(std::min(255.0f, finalGray * gFactor));
                uint8_t b = static_cast<uint8_t>(std::min(255.0f, finalGray * bFactor));

                outPixels[y * dispW + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }
        return;
    }

    // Fallback 1-bit B/W mode
    for (int y = 0; y < dispH; ++y) {
        for (int x = 0; x < dispW; ++x) {
            int byteIdx = (y * dispWidthBytes) + (x / 8);
            int bitIdx = 7 - (x % 8);
            bool white = (bwBuf[byteIdx] >> bitIdx) & 1;
            uint8_t finalGray = white ? 255 : 0;
            if (inverted) {
                finalGray = 255 - finalGray;
            }

            uint8_t r = static_cast<uint8_t>(std::min(255.0f, finalGray * rFactor));
            uint8_t g = static_cast<uint8_t>(std::min(255.0f, finalGray * gFactor));
            uint8_t b = static_cast<uint8_t>(std::min(255.0f, finalGray * bFactor));

            outPixels[y * dispW + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
}
