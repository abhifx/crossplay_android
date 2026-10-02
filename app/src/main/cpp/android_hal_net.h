#ifndef ANDROID_HAL_NET_H
#define ANDROID_HAL_NET_H

#include <jni.h>
#include <string>
#include <functional>

class AndroidHalNet {
public:
    using ProgressCb = std::function<void(size_t downloaded, size_t total)>;

    static AndroidHalNet& getInstance();

    void setJniEnv(JavaVM* jvm, jobject mainActivityObj);

    std::string fetchUrl(const std::string& url);
    bool downloadFile(const std::string& url, const std::string& savePath);

    void setProgressCallback(ProgressCb cb, const bool* cancelFlag);
    void clearProgressCallback();
    bool onDownloadProgress(long downloaded, long total);

private:
    AndroidHalNet() = default;
    JavaVM* m_jvm = nullptr;
    jobject m_activityObj = nullptr;

    ProgressCb m_currentProgressCb = nullptr;
    const bool* m_currentCancelFlag = nullptr;
};

#endif // ANDROID_HAL_NET_H
