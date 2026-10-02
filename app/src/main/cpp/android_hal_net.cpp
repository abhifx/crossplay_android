#include "android_hal_net.h"
#include <android/log.h>

#define LOG_TAG "AndroidHalNet"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AndroidHalNet& AndroidHalNet::getInstance() {
    static AndroidHalNet instance;
    return instance;
}

void AndroidHalNet::setJniEnv(JavaVM* jvm, jobject mainActivityObj) {
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

void AndroidHalNet::setProgressCallback(ProgressCb cb, const bool* cancelFlag) {
    m_currentProgressCb = std::move(cb);
    m_currentCancelFlag = cancelFlag;
}

void AndroidHalNet::clearProgressCallback() {
    m_currentProgressCb = nullptr;
    m_currentCancelFlag = nullptr;
}

bool AndroidHalNet::onDownloadProgress(long downloaded, long total) {
    if (m_currentCancelFlag && *m_currentCancelFlag) {
        return false; // Abort download
    }
    if (m_currentProgressCb) {
        m_currentProgressCb(static_cast<size_t>(downloaded), static_cast<size_t>(total));
    }
    return !(m_currentCancelFlag && *m_currentCancelFlag);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_crosspoint_crossplay_MainActivity_nativeOnDownloadProgress(JNIEnv* env, jobject obj, jlong downloaded, jlong total) {
    bool cont = AndroidHalNet::getInstance().onDownloadProgress(static_cast<long>(downloaded), static_cast<long>(total));
    return cont ? JNI_TRUE : JNI_FALSE;
}

std::string AndroidHalNet::fetchUrl(const std::string& url) {
    if (!m_jvm || !m_activityObj) {
        LOGE("JNI JavaVM or MainActivity reference missing for fetchUrl");
        return "";
    }

    JNIEnv* env = nullptr;
    bool needsDetach = false;
    jint getEnvRes = m_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (getEnvRes == JNI_EDETACHED) {
        if (m_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return "";
        needsDetach = true;
    } else if (getEnvRes != JNI_OK || !env) {
        return "";
    }

    std::string resultStr = "";
    jclass cls = env->GetObjectClass(m_activityObj);
    if (cls) {
        jmethodID methodId = env->GetMethodID(cls, "fetchUrlFromJava", "(Ljava/lang/String;)Ljava/lang/String;");
        if (methodId) {
            jstring jUrl = env->NewStringUTF(url.c_str());
            jstring jResult = static_cast<jstring>(env->CallObjectMethod(m_activityObj, methodId, jUrl));

            if (jResult) {
                const char* cRes = env->GetStringUTFChars(jResult, nullptr);
                if (cRes) {
                    resultStr = cRes;
                    env->ReleaseStringUTFChars(jResult, cRes);
                }
                env->DeleteLocalRef(jResult);
            }
            if (jUrl) env->DeleteLocalRef(jUrl);
        }
        env->DeleteLocalRef(cls);
    }

    if (needsDetach) m_jvm->DetachCurrentThread();
    return resultStr;
}

bool AndroidHalNet::downloadFile(const std::string& url, const std::string& savePath) {
    if (!m_jvm || !m_activityObj) {
        LOGE("JNI JavaVM or MainActivity reference missing for downloadFile");
        return false;
    }

    JNIEnv* env = nullptr;
    bool needsDetach = false;
    jint getEnvRes = m_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (getEnvRes == JNI_EDETACHED) {
        if (m_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
        needsDetach = true;
    } else if (getEnvRes != JNI_OK || !env) {
        return false;
    }

    bool success = false;
    jclass cls = env->GetObjectClass(m_activityObj);
    if (cls) {
        jmethodID methodId = env->GetMethodID(cls, "downloadFileFromJava", "(Ljava/lang/String;Ljava/lang/String;)Z");
        if (methodId) {
            jstring jUrl = env->NewStringUTF(url.c_str());
            jstring jPath = env->NewStringUTF(savePath.c_str());
            jboolean res = env->CallBooleanMethod(m_activityObj, methodId, jUrl, jPath);
            success = (res == JNI_TRUE);

            if (jUrl) env->DeleteLocalRef(jUrl);
            if (jPath) env->DeleteLocalRef(jPath);
        }
        env->DeleteLocalRef(cls);
    }

    if (needsDetach) m_jvm->DetachCurrentThread();
    return success;
}
