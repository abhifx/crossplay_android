#ifndef DISPLAY_EINK_H
#define DISPLAY_EINK_H

#include "crossplay_bridge.h"
#include <vector>
#include <cstdint>

enum IconType {
    ICON_FOLDER = 0,
    ICON_BOOK = 1,
    ICON_TRANSFER = 2,
    ICON_SETTINGS = 3,
    ICON_GAMES = 4,
    ICON_APPS = 5,
    ICON_BATTERY = 6
};

class DisplayEInk {
public:
    DisplayEInk(int width = EINK_WIDTH, int height = EINK_HEIGHT);
    ~DisplayEInk();

    void clear(uint32_t color = 0xFFFFFFFF);
    void drawPixel(int x, int y, uint32_t color);
    void drawRect(int x, int y, int w, int h, uint32_t color, bool fill = false);
    void drawRoundedRect(int x, int y, int w, int h, int r, uint32_t color, bool fill = false);
    void drawLine(int x1, int y1, int x2, int y2, uint32_t color);
    void drawText(int x, int y, const char* text, uint32_t color, int scale = 2);
    void drawCard(int x, int y, int w, int h, const char* title, const char* subtitle, bool selected = false);

    void drawIcon(int x, int y, IconType icon, uint32_t color);
    void drawHeroCard(int x, int y, int w, int h, const char* title, const char* author);
    void drawMenuItem(int x, int y, int w, int h, IconType icon, const char* label, bool selected = false);

    void setEInkRefreshSimulated(bool enable) { m_einkSimulated = enable; }
    bool isEInkRefreshSimulated() const { return m_einkSimulated; }

    void triggerFlashRefresh();

    const uint32_t* getBuffer() const { return m_frameBuffer.data(); }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    int m_width;
    int m_height;
    std::vector<uint32_t> m_frameBuffer;
    bool m_einkSimulated = false;
    int m_flashFrameCount = 0;
};

#endif // DISPLAY_EINK_H
