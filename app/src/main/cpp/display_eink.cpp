#include "display_eink.h"
#include <cstring>
#include <algorithm>

// 5x7 ASCII Bitmap Font Array (ASCII 32 ' ' to 126 '~')
static const uint8_t font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (32)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '\''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 's'
    {0x04, 0x3E, 0x44, 0x24, 0x08}, // 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // '}'
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // '~'
};

DisplayEInk::DisplayEInk(int width, int height)
    : m_width(width), m_height(height), m_frameBuffer(width * height, 0xFFFFFFFF) {
}

DisplayEInk::~DisplayEInk() {}

void DisplayEInk::clear(uint32_t color) {
    if (m_flashFrameCount > 0) {
        // E-Ink flash inversion effect
        m_flashFrameCount--;
        uint32_t flashColor = (m_flashFrameCount % 2 == 0) ? 0xFF000000 : 0xFFFFFFFF;
        std::fill(m_frameBuffer.begin(), m_frameBuffer.end(), flashColor);
        return;
    }
    std::fill(m_frameBuffer.begin(), m_frameBuffer.end(), color);
}

void DisplayEInk::triggerFlashRefresh() {
    if (m_einkSimulated) {
        m_flashFrameCount = 2; // Invert twice for authentic E-Ink waveform simulation
    }
}

void DisplayEInk::drawPixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
    m_frameBuffer[y * m_width + x] = color;
}

void DisplayEInk::drawRect(int x, int y, int w, int h, uint32_t color, bool fill) {
    int x2 = std::min(x + w, m_width);
    int y2 = std::min(y + h, m_height);
    x = std::max(0, x);
    y = std::max(0, y);

    if (fill) {
        for (int py = y; py < y2; ++py) {
            for (int px = x; px < x2; ++px) {
                m_frameBuffer[py * m_width + px] = color;
            }
        }
    } else {
        for (int px = x; px < x2; ++px) {
            drawPixel(px, y, color);
            drawPixel(px, y2 - 1, color);
        }
        for (int py = y; py < y2; ++py) {
            drawPixel(x, py, color);
            drawPixel(x2 - 1, py, color);
        }
    }
}

void DisplayEInk::drawRoundedRect(int x, int y, int w, int h, int r, uint32_t color, bool fill) {
    if (fill) {
        drawRect(x + r, y, w - 2 * r, h, color, true);
        drawRect(x, y + r, w, h - 2 * r, color, true);
        // Corner circles approximation
        for (int dy = 0; dy <= r; ++dy) {
            for (int dx = 0; dx <= r; ++dx) {
                if (dx * dx + dy * dy <= r * r) {
                    drawPixel(x + r - dx, y + r - dy, color);
                    drawPixel(x + w - r - 1 + dx, y + r - dy, color);
                    drawPixel(x + r - dx, y + h - r - 1 + dy, color);
                    drawPixel(x + w - r - 1 + dx, y + h - r - 1 + dy, color);
                }
            }
        }
    } else {
        drawRect(x + r, y, w - 2 * r, 1, color, true);
        drawRect(x + r, y + h - 1, w - 2 * r, 1, color, true);
        drawRect(x, y + r, 1, h - 2 * r, color, true);
        drawRect(x + w - 1, y + r, 1, h - 2 * r, color, true);
    }
}

void DisplayEInk::drawLine(int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void DisplayEInk::drawText(int x, int y, const char* text, uint32_t color, int scale) {
    if (!text) return;
    int curX = x;

    while (*text) {
        char c = *text;
        if (c == '\n') {
            y += (8 * scale);
            curX = x;
            text++;
            continue;
        }

        if (c >= 32 && c <= 126) {
            int fontIdx = c - 32;
            for (int col = 0; col < 5; col++) {
                uint8_t line = font5x7[fontIdx][col];
                for (int row = 0; row < 7; row++) {
                    if ((line >> row) & 0x01) {
                        drawRect(curX + col * scale, y + row * scale, scale, scale, color, true);
                    }
                }
            }
        }
        curX += (6 * scale); // 5px character + 1px spacing
        text++;
    }
}

void DisplayEInk::drawIcon(int x, int y, IconType icon, uint32_t color) {
    switch (icon) {
        case ICON_FOLDER: { // 📁 Folder
            drawRect(x, y + 4, 20, 14, color, false);
            drawRect(x + 2, y + 2, 8, 3, color, true);
            drawLine(x, y + 8, x + 20, y + 8, color);
            break;
        }
        case ICON_BOOK: { // 📖 Book & Ribbon
            drawRect(x + 2, y, 16, 20, color, false);
            drawRect(x + 5, y + 3, 10, 14, color, false);
            drawRect(x + 9, y, 3, 10, color, true); // Ribbon
            break;
        }
        case ICON_TRANSFER: { // ✈️ Paper Airplane / Transfer
            drawLine(x, y + 8, x + 20, y, color);
            drawLine(x + 20, y, x + 12, y + 20, color);
            drawLine(x + 12, y + 20, x + 8, y + 12, color);
            drawLine(x + 8, y + 12, x, y + 8, color);
            drawLine(x + 8, y + 12, x + 20, y, color);
            break;
        }
        case ICON_SETTINGS: { // 🎛️ Sliders
            // 3 Sliders
            drawLine(x + 2, y + 4, x + 18, y + 4, color);
            drawRect(x + 6, y + 2, 4, 5, color, true);

            drawLine(x + 2, y + 10, x + 18, y + 10, color);
            drawRect(x + 12, y + 8, 4, 5, color, true);

            drawLine(x + 2, y + 16, x + 16, y + 16, color);
            drawRect(x + 4, y + 14, 4, 5, color, true);
            break;
        }
        case ICON_GAMES: { // 🕹️ Gamepad / D-Pad
            drawRoundedRect(x, y + 4, 22, 14, 3, color, false);
            // D-pad
            drawRect(x + 4, y + 7, 2, 8, color, true);
            drawRect(x + 1, y + 10, 8, 2, color, true);
            // Buttons
            drawPixel(x + 16, y + 8, color);
            drawPixel(x + 18, y + 11, color);
            drawPixel(x + 14, y + 11, color);
            drawPixel(x + 16, y + 14, color);
            break;
        }
        case ICON_APPS: { // 🔲 2x2 Grid
            drawRect(x + 2, y + 2, 7, 7, color, false);
            drawRect(x + 11, y + 2, 7, 7, color, false);
            drawRect(x + 2, y + 11, 7, 7, color, false);
            drawRect(x + 11, y + 11, 7, 7, color, false);
            break;
        }
        case ICON_BATTERY: { // 🔋 Battery
            drawRect(x, y + 2, 22, 12, color, false);
            drawRect(x + 22, y + 5, 2, 6, color, true); // Battery Cap
            drawRect(x + 2, y + 4, 18, 8, color, true); // Full charge
            break;
        }
    }
}

void DisplayEInk::drawHeroCard(int x, int y, int w, int h, const char* title, const char* author) {
    // Gray container card (CrossPoint design)
    drawRoundedRect(x, y, w, h, 12, 0xFFCCCCCC, true);
    drawRoundedRect(x, y, w, h, 12, 0xFF999999, false);

    // Book Cover Box (Left)
    int coverX = x + 16;
    int coverY = y + 16;
    int coverW = 120;
    int coverH = 150;

    drawRect(coverX, coverY, coverW, coverH, 0xFFFFFFFF, true);
    drawRect(coverX, coverY, coverW, coverH, 0xFF000000, false);
    drawRect(coverX + 2, coverY + 2, coverW - 4, coverH - 4, 0xFF000000, false);

    // Book Cover Title Art Illustration
    drawText(coverX + 8, coverY + 8, "ALICE IN", 0xFF000000, 1);
    drawText(coverX + 8, coverY + 18, "WONDERLAND", 0xFF000000, 1);
    drawLine(coverX + 6, coverY + 30, coverX + coverW - 6, coverY + 30, 0xFF000000);

    // Cover Illustration Tree / Alice sketch outline
    drawRect(coverX + 16, coverY + 40, 88, 80, 0xFFEAEAEA, true);
    drawRect(coverX + 16, coverY + 40, 88, 80, 0xFF000000, false);
    drawText(coverX + 24, coverY + 65, "[ COVER ]", 0xFF000000, 1);

    drawText(coverX + 16, coverY + 132, "LEWIS CARROLL", 0xFF000000, 1);

    // Right Side Metadata
    int metaX = coverX + coverW + 24;
    int metaY = y + 36;

    if (title) {
        drawText(metaX, metaY, title, 0xFF000000, 2);
    }
    if (author) {
        drawText(metaX, metaY + 42, author, 0xFF444444, 2);
    }
}

void DisplayEInk::drawMenuItem(int x, int y, int w, int h, IconType icon, const char* label, bool selected) {
    if (selected) {
        drawRoundedRect(x, y, w, h, 8, 0xFF000000, true);
        drawIcon(x + 16, y + (h - 20) / 2, icon, 0xFFFFFFFF);
        drawText(x + 52, y + (h - 14) / 2, label, 0xFFFFFFFF, 2);
    } else {
        drawIcon(x + 16, y + (h - 20) / 2, icon, 0xFF000000);
        drawText(x + 52, y + (h - 14) / 2, label, 0xFF000000, 2);
    }
}

void DisplayEInk::drawCard(int x, int y, int w, int h, const char* title, const char* subtitle, bool selected) {
    uint32_t bgColor = selected ? 0xFF000000 : 0xFFFFFFFF;
    uint32_t fgColor = selected ? 0xFFFFFFFF : 0xFF000000;

    // Card background
    drawRect(x, y, w, h, bgColor, true);
    // Border
    drawRect(x, y, w, h, 0xFF000000, false);
    drawRect(x + 1, y + 1, w - 2, h - 2, 0xFF000000, false);

    if (title) {
        drawText(x + 12, y + 12, title, fgColor, 2);
    }
    if (subtitle) {
        drawText(x + 12, y + 40, subtitle, selected ? 0xFFCCCCCC : 0xFF444444, 1);
    }
}
