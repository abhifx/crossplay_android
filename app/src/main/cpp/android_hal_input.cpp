#include "android_hal_input.h"
#include <android/log.h>
#include <chrono>
#include <cmath>
#include <algorithm>

#define LOG_TAG "AndroidHalInput"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static uint32_t currentMillis() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

AndroidHalInput& AndroidHalInput::getInstance() {
    static AndroidHalInput instance;
    return instance;
}

void AndroidHalInput::initialize() {
    m_isDown = false;
    m_suppressed = false;
    m_longPressFired = false;

    m_downX = 0.0f;
    m_downY = 0.0f;
    m_currentX = 0.0f;
    m_currentY = 0.0f;

    m_downTimeMs = 0;
    m_lastTouchHeldMs = 0;

    m_nextPressActive = false;
    m_nextReleaseActive = false;
    m_nextTapActive = false;
    m_nextLongPressActive = false;

    m_framePressActive = false;
    m_frameReleaseActive = false;
    m_frameTapActive = false;
    m_frameLongPressActive = false;

    m_buttonState = 0;
    m_nextButtonPressed = 0;
    m_nextButtonReleased = 0;
    m_frameButtonPressed = 0;
    m_frameButtonReleased = 0;
}

void AndroidHalInput::advanceFrame() {
    // Promote pending signals to current frame
    m_framePressActive = m_nextPressActive;
    m_nextPressActive = false;

    m_frameReleaseActive = m_nextReleaseActive;
    m_nextReleaseActive = false;

    m_frameTapActive = m_nextTapActive;
    m_frameTapX = m_nextTapX;
    m_frameTapY = m_nextTapY;
    m_nextTapActive = false;

    m_frameLongPressActive = m_nextLongPressActive;
    m_nextLongPressActive = false;

    // Check time-based long press during active holding
    if (m_isDown && !m_suppressed && !m_longPressFired) {
        uint32_t heldMs = currentMillis() - m_downTimeMs;
        float dx = std::abs(m_currentX - m_downX);
        float dy = std::abs(m_currentY - m_downY);
        if (heldMs >= 500 && dx < 0.12f && dy < 0.12f) {
            m_frameLongPressActive = true;
            m_longPressFired = true;
            LOGI("Touch LONG PRESS registered at (%.2f, %.2f)", m_downX, m_downY);
        }
    }

    m_frameButtonPressed = m_nextButtonPressed;
    m_frameButtonReleased = m_nextButtonReleased;
    m_nextButtonPressed = 0;
    m_nextButtonReleased = 0;
}

void AndroidHalInput::setDisplayResolution(int physW, int physH) {
    if (physW > 0 && physH > 0) {
        m_physWidth = physW;
        m_physHeight = physH;
    }
}

void AndroidHalInput::handleTouchEvent(int action, float x, float y, int physW, int physH) {
    float floatW = physW > 0 ? static_cast<float>(physW) : static_cast<float>(m_physWidth);
    float floatH = physH > 0 ? static_cast<float>(physH) : static_cast<float>(m_physHeight);
    float nx = std::clamp(x / floatW, 0.0f, 1.0f);
    float ny = std::clamp(y / floatH, 0.0f, 1.0f);

    uint32_t now = currentMillis();

    if (action == 0) { // ACTION_DOWN
        m_isDown = true;
        m_suppressed = false;
        m_longPressFired = false;

        m_downX = nx;
        m_downY = ny;
        m_currentX = nx;
        m_currentY = ny;
        m_downTimeMs = now;

        m_nextPressActive = true; // One-shot press edge
        LOGI("Touch DOWN at (%.2f, %.2f)", nx, ny);
    } else if (action == 1) { // ACTION_MOVE
        m_currentX = nx;
        m_currentY = ny;
    } else if (action == 2) { // ACTION_UP or ACTION_CANCEL
        if (m_isDown) {
            m_lastTouchHeldMs = now - m_downTimeMs;
            m_currentX = nx;
            m_currentY = ny;
            m_nextReleaseActive = true; // One-shot release edge

            float dx = std::abs(nx - m_downX);
            float dy = std::abs(ny - m_downY);

            // If move distance is small and long press hasn't consumed it, register tap & emit Confirm button release
            if (!m_suppressed && !m_longPressFired && dx < 0.12f && dy < 0.12f) {
                m_nextTapActive = true;
                m_nextTapX = nx;
                m_nextTapY = ny;
                LOGI("Touch TAP registered at (%.2f, %.2f)", nx, ny);
            }
        }
        m_isDown = false;
    }
}

void AndroidHalInput::handleKeyEvent(int keyCode, bool isDown) {
    int btn = -1;
    if (keyCode == 66 || keyCode == 23 || keyCode == 13 || keyCode == 160) { // Enter / Center / Return / Numpad Enter
        btn = 1; // BTN_CONFIRM
    } else if (keyCode == 4 || keyCode == 0 || keyCode == 111) { // Back / Escape
        btn = 0; // BTN_BACK
    } else if (keyCode == 19) { // Up
        btn = 4; // BTN_UP
    } else if (keyCode == 20) { // Down
        btn = 5; // BTN_DOWN
    } else if (keyCode == 21) { // Left
        btn = 2; // BTN_LEFT
    } else if (keyCode == 22) { // Right
        btn = 3; // BTN_RIGHT
    } else if (keyCode == 24 || keyCode == 6 || keyCode == 92 || keyCode == 88) { // Vol Up / Page Up / Media Prev
        btn = 4; // BTN_UP
    } else if (keyCode == 25 || keyCode == 7 || keyCode == 93 || keyCode == 87 || keyCode == 62) { // Vol Down / Page Down / Media Next / Space
        btn = 5; // BTN_DOWN
    }

    LOGI("Key event: keyCode=%d, isDown=%d -> btn=%d", keyCode, isDown, btn);

    if (btn >= 0 && btn < 16) {
        if (isDown) {
            m_buttonState |= (1u << btn);
            m_nextButtonPressed |= (1u << btn);
        } else {
            m_buttonState &= ~(1u << btn);
            m_nextButtonReleased |= (1u << btn);
        }
    }
}



bool AndroidHalInput::isButtonPressed(uint8_t btn) const {
    if (btn < 16) return (m_buttonState & (1u << btn)) != 0;
    return false;
}

bool AndroidHalInput::wasButtonPressed(uint8_t btn) const {
    if (btn < 16) return (m_frameButtonPressed & (1u << btn)) != 0;
    return false;
}

bool AndroidHalInput::wasButtonReleased(uint8_t btn) const {
    if (btn < 16) return (m_frameButtonReleased & (1u << btn)) != 0;
    return false;
}

bool AndroidHalInput::wasTouchTap(float& nx, float& ny) {
    if (m_frameTapActive && !m_suppressed) {
        nx = m_frameTapX;
        ny = m_frameTapY;
        return true;
    }
    return false;
}

bool AndroidHalInput::wasTouchPressedAt(float& nx, float& ny) {
    if (m_framePressActive && !m_suppressed) {
        nx = m_downX;
        ny = m_downY;
        return true;
    }
    return false;
}

bool AndroidHalInput::wasTouchReleased() {
    if (m_frameReleaseActive) {
        return true;
    }
    return false;
}

bool AndroidHalInput::isTouchTapCandidate(float& nx, float& ny, unsigned long& heldMs) {
    if (m_isDown && !m_suppressed) {
        nx = m_currentX;
        ny = m_currentY;
        heldMs = currentMillis() - m_downTimeMs;
        return true;
    }
    return false;
}

bool AndroidHalInput::isTouchHeldAt(float& nx, float& ny) {
    if (m_isDown && !m_suppressed) {
        nx = m_currentX;
        ny = m_currentY;
        return true;
    }
    return false;
}

bool AndroidHalInput::wasTouchLongPress(float& nx, float& ny) {
    if (m_frameLongPressActive && !m_suppressed) {
        nx = m_downX;
        ny = m_downY;
        return true;
    }
    return false;
}

bool AndroidHalInput::wasSwipe(float& nxStart, float& nyStart, float& nxEnd, float& nyEnd) {
    if (m_frameReleaseActive) {
        float dx = m_currentX - m_downX;
        float dy = m_currentY - m_downY;
        if (std::abs(dx) > 0.08f || std::abs(dy) > 0.08f) {
            nxStart = m_downX;
            nyStart = m_downY;
            nxEnd = m_currentX;
            nyEnd = m_currentY;
            return true;
        }
    }
    return false;
}

bool AndroidHalInput::wasTouchActivity() {
    return m_isDown || m_framePressActive || m_frameReleaseActive || m_frameTapActive || m_frameButtonPressed || m_frameButtonReleased;
}

void AndroidHalInput::suppressTouchContact() {
    m_suppressed = true;
    m_frameTapActive = false;
    m_nextTapActive = false;
    m_framePressActive = false;
    m_nextPressActive = false;
}
