#ifndef CROSSPLAY_BRIDGE_H
#define CROSSPLAY_BRIDGE_H

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#define LOG_TAG "CrossPlayNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

constexpr int EINK_WIDTH = 480;
constexpr int EINK_HEIGHT = 800;

enum TouchAction {
    TOUCH_DOWN = 0,
    TOUCH_MOVE = 1,
    TOUCH_UP = 2
};

enum HardwareButton {
    BTN_BACK = 0,
    BTN_CONFIRM = 1,
    BTN_LEFT = 2,
    BTN_RIGHT = 3,
    BTN_UP = 4,
    BTN_DOWN = 5,
    BTN_PAGE_PREV = 6,
    BTN_PAGE_NEXT = 7,
    BTN_HOME = 8
};

void setForceRedraw(bool force);
void setFrameBufferReady(bool ready);

#endif // CROSSPLAY_BRIDGE_H
