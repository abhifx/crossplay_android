#ifndef ANDROID_HAL_INPUT_H
#define ANDROID_HAL_INPUT_H

#include <cstdint>

class AndroidHalInput {
public:
    static AndroidHalInput& getInstance();

    void initialize();
    void setDisplayResolution(int physW, int physH);
    void handleTouchEvent(int action, float x, float y, int physW = 800, int physH = 480);
    void handleKeyEvent(int keyCode, bool isDown);
    void advanceFrame();

    bool hasTouch() const { return true; }
    bool wasTouchTap(float& nx, float& ny);
    bool wasTouchPressedAt(float& nx, float& ny);
    bool wasTouchReleased();
    bool isTouchTapCandidate(float& nx, float& ny, unsigned long& heldMs);
    bool isTouchHeldAt(float& nx, float& ny);
    bool wasTouchLongPress(float& nx, float& ny);
    bool wasSwipe(float& nxStart, float& nyStart, float& nxEnd, float& nyEnd);
    bool wasTouchActivity();
    void suppressTouchContact();
    unsigned long lastTouchHeldMs() const { return m_lastTouchHeldMs; }

    bool isButtonPressed(uint8_t btn) const;
    bool wasButtonPressed(uint8_t btn) const;
    bool wasButtonReleased(uint8_t btn) const;

private:
    AndroidHalInput() = default;

    int m_physWidth = 800;
    int m_physHeight = 480;

    bool m_isDown = false;
    bool m_suppressed = false;
    bool m_longPressFired = false;

    float m_downX = 0.0f;
    float m_downY = 0.0f;
    float m_currentX = 0.0f;
    float m_currentY = 0.0f;

    uint32_t m_downTimeMs = 0;
    uint32_t m_lastTouchHeldMs = 0;

    // Next pending signals
    bool m_nextPressActive = false;
    bool m_nextReleaseActive = false;
    bool m_nextTapActive = false;
    float m_nextTapX = 0.0f;
    float m_nextTapY = 0.0f;
    bool m_nextLongPressActive = false;

    // Active frame signals (latched for 1 frame)
    bool m_framePressActive = false;
    bool m_frameReleaseActive = false;
    bool m_frameTapActive = false;
    float m_frameTapX = 0.0f;
    float m_frameTapY = 0.0f;
    bool m_frameLongPressActive = false;

    uint32_t m_buttonState = 0;
    uint32_t m_nextButtonPressed = 0;
    uint32_t m_nextButtonReleased = 0;
    uint32_t m_frameButtonPressed = 0;
    uint32_t m_frameButtonReleased = 0;
};

#endif // ANDROID_HAL_INPUT_H
