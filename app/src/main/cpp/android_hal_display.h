#ifndef ANDROID_HAL_DISPLAY_H
#define ANDROID_HAL_DISPLAY_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <jni.h>

class AndroidHalDisplay {
public:
    static AndroidHalDisplay& getInstance();

    int getDisplayWidth() const { return m_displayWidth; }
    int getDisplayHeight() const { return m_displayHeight; }
    int getLogicalWidth() const { return m_logicalWidth; }
    int getLogicalHeight() const { return m_logicalHeight; }
    int getDisplayWidthBytes() const { return m_displayWidthBytes; }
    size_t getBufferSize() const { return m_displayWidthBytes * m_displayHeight; }

    static constexpr int DISPLAY_WIDTH = 800;
    static constexpr int DISPLAY_HEIGHT = 480;
    static constexpr int DISPLAY_WIDTH_BYTES = DISPLAY_WIDTH / 8; // 100
    static constexpr size_t BUFFER_SIZE = DISPLAY_WIDTH_BYTES * DISPLAY_HEIGHT; // 48000

    void initialize();
    void setJniEnv(JavaVM* jvm, jobject mainActivityObj);
    void setDisplayResolution(int physW, int physH);

    void setGrayscaleMode(bool enabled) { m_enhancedGrayscale = enabled; }
    bool isGrayscaleMode() const { return m_enhancedGrayscale; }

    void setBrightness(uint8_t percent);
    void setWarmth(uint8_t warmPercent);
    void setLightOn(bool on);

    uint8_t getBrightness() const { return m_brightness; }
    uint8_t getWarmth() const { return m_warmth; }
    bool isLightOn() const { return m_lightOn; }

    void triggerFlashRefresh();
    void renderToPixelArray(const uint8_t* bwBuf, uint32_t* outPixels);
    void setGrayscaleBuffers(const uint8_t* lsb, const uint8_t* msb);
    void setHasGrayscale(bool val) { m_hasGrayscale = val; }

    void drawPixelGray(int x, int y, uint8_t gray);
    void drawPixelPhysical(int phyX, int phyY, uint8_t gray);
    void clearGrayBuffer();

private:
    AndroidHalDisplay() = default;
    void updateSystemBrightness();

    int m_displayWidth = 800;
    int m_displayHeight = 480;
    int m_logicalWidth = 480;
    int m_logicalHeight = 800;
    int m_displayWidthBytes = 100;

    bool m_enhancedGrayscale = true;
    int m_flashFrameCount = 0;

    uint8_t m_brightness = 100;
    uint8_t m_warmth = 0;
    bool m_lightOn = true;

    JavaVM* m_jvm = nullptr;
    jobject m_activityObj = nullptr;

    std::vector<uint8_t> m_lsbBuffer;
    std::vector<uint8_t> m_msbBuffer;
    std::vector<uint8_t> m_bwBuffer;
    std::vector<uint8_t> m_grayBuffer;
    bool m_hasGrayscale = false;
    bool m_hasGrayPixels = false;
};

#endif // ANDROID_HAL_DISPLAY_H
