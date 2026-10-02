#include "app_framework.h"
#include "file_system.h"
#include "network_client.h"
#include <android/log.h>
#include <algorithm>

CrossPlayEngine& CrossPlayEngine::getInstance() {
    static CrossPlayEngine instance;
    return instance;
}

CrossPlayEngine::CrossPlayEngine() {
    // CrossPoint / CrossPlay Home Menu Items (matching device UI layout)
    m_homeMenuItems = {
        { APP_BROWSE_FILES, "Browse Files", "Main", "Explore files & ebooks on SD card", ICON_FOLDER },
        { APP_RECENT_BOOKS, "Recent Books", "Main", "Continue reading recent ebooks", ICON_BOOK },
        { APP_FILE_TRANSFER, "File Transfer", "Main", "Wi-Fi & OPDS catalog transfer", ICON_TRANSFER },
        { APP_SETTINGS, "Settings", "Main", "Device, refresh & display settings", ICON_SETTINGS },
        { APP_GAMES_MENU, "Games", "Main", "21 Turn-based & logic e-ink games", ICON_GAMES },
        { APP_APPS_MENU, "Apps", "Main", "8 Connected & offline e-ink apps", ICON_APPS }
    };

    // Sub-menu Apps
    m_appMenuItems = {
        { APP_EPUB_READER, "EPUB Reader", "Apps", "Read ebooks, manage library & bookmarks", ICON_BOOK },
        { APP_FLASHCARDS, "Flashcards (FSRS)", "Apps", "Spaced repetition learning & Anki decks", ICON_APPS },
        { APP_HACKER_NEWS, "Hacker News", "Apps", "Read HN frontpage via Jina AI & Algolia", ICON_TRANSFER },
        { APP_XKCD, "xkcd Reader", "Apps", "Daily webcomics rendered 1:1 for e-ink", ICON_APPS },
        { APP_GET_BOOKS, "Get Books (OPDS)", "Apps", "Download ebooks from free OPDS catalogs", ICON_FOLDER },
        { APP_INSTAPAPER, "Instapaper", "Apps", "Two-way read-later article queue sync", ICON_TRANSFER },
        { APP_WIKIPEDIA, "Wikipedia Offline", "Apps", "Search 50,000 offline articles on SD card", ICON_BOOK },
        { APP_CALCULATOR, "Calculator", "Apps", "Exact decimal arithmetic calculator", ICON_SETTINGS }
    };

    // Sub-menu Games
    m_gameMenuItems = {
        { GAME_CHESS, "Chess", "Games", "2-Player local / Wi-Fi peer-to-peer chess", ICON_GAMES },
        { GAME_SUDOKU, "Sudoku", "Games", "Unlimited logic puzzle generator", ICON_GAMES },
        { GAME_MINESWEEPER, "Minesweeper", "Games", "Classic logic grid sweeper", ICON_GAMES },
        { GAME_SOLITAIRE, "Solitaire", "Games", "Klondike card game", ICON_GAMES },
        { GAME_CONNECTIONS, "Connections", "Games", "Group 16 words into 4 categories", ICON_GAMES },
        { GAME_D_AND_DIAGRAMS, "D&Diagrams", "Games", "64 dungeon nonogram logic puzzles", ICON_GAMES },
        { GAME_MURDLE, "Murdle", "Games", "100 murder mystery deductive logic puzzles", ICON_GAMES },
        { GAME_TRIVIA, "Trivia", "Games", "50,000 Jeopardy questions", ICON_GAMES },
        { GAME_BATTLESHIP, "Battleship", "Games", "Naval fleet tactical game", ICON_GAMES },
        { GAME_CHECKERS, "Checkers", "Games", "8x8 draughts board game", ICON_GAMES },
        { GAME_CONNECT_FOUR, "Connect Four", "Games", "4-in-a-row vertical drop strategy", ICON_GAMES },
        { GAME_YAHTZEE, "Yahtzee", "Games", "Dice probability game", ICON_GAMES },
        { GAME_KNUCKLEBONES, "Knucklebones", "Games", "Cult-classic dice strategy", ICON_GAMES },
        { GAME_PICROSS, "Picross", "Games", "Picture crossword nonograms", ICON_GAMES },
        { GAME_GO, "Go / Baduk", "Games", "Ancient strategy board game", ICON_GAMES }
    };
}

void CrossPlayEngine::initialize(DisplayEInk* display) {
    m_display = display;
    __android_log_print(ANDROID_LOG_INFO, "CrossPlayEngine", "Engine initialized with 800x480 canvas");
}

void CrossPlayEngine::switchApp(int appId) {
    m_currentApp = appId;
    if (m_display) {
        m_display->triggerFlashRefresh();
    }
}

std::string CrossPlayEngine::getStatusText() const {
    return "100%";
}

void CrossPlayEngine::render() {
    if (!m_display) return;

    m_display->clear(0xFFFFFFFF); // Clean white background

    // Top Right Battery Status (CrossPoint design)
    m_display->drawText(EINK_WIDTH - 90, 8, getStatusText().c_str(), 0xFF000000, 1);
    m_display->drawIcon(EINK_WIDTH - 42, 4, ICON_BATTERY, 0xFF000000);

    switch (m_currentApp) {
        case APP_HOME:
            renderHome();
            break;
        case APP_BROWSE_FILES:
            renderBrowseFiles();
            break;
        case APP_RECENT_BOOKS:
            renderRecentBooks();
            break;
        case APP_FILE_TRANSFER:
            renderFileTransfer();
            break;
        case APP_SETTINGS:
            renderSettings();
            break;
        case APP_GAMES_MENU:
            renderGamesMenu();
            break;
        case APP_APPS_MENU:
            renderAppsMenu();
            break;

        // Apps
        case APP_EPUB_READER:
            renderEpubReader();
            break;
        case APP_FLASHCARDS:
            renderFlashcards();
            break;
        case APP_HACKER_NEWS:
            renderHackerNews();
            break;
        case APP_XKCD:
            renderXkcd();
            break;
        case APP_GET_BOOKS:
            renderGetBooks();
            break;
        case APP_WIKIPEDIA:
            renderWikipedia();
            break;
        case APP_CALCULATOR:
            renderCalculator();
            break;

        default: {
            for (const auto& item : m_gameMenuItems) {
                if (item.id == m_currentApp) {
                    renderGameGeneric(item.title.c_str(), item.description.c_str());
                    return;
                }
            }
            break;
        }
    }
}

void CrossPlayEngine::renderHome() {
    // 1. Hero / Currently Reading Card (Top Section)
    int heroX = 24;
    int heroY = 28;
    int heroW = EINK_WIDTH - 48; // 752 px
    int heroH = 180;

    m_display->drawHeroCard(heroX, heroY, heroW, heroH, "Alice's Adventures\nin Wonderland", "Lewis Carroll");

    // 2. Vertical Menu Options List
    int listX = 36;
    int startY = 224;
    int itemH = 38;
    int spacingY = 40;

    for (size_t i = 0; i < m_homeMenuItems.size(); ++i) {
        const auto& item = m_homeMenuItems[i];
        int itemY = startY + i * spacingY;
        bool isSelected = (static_cast<int>(i) == m_selectedHomeIndex);

        m_display->drawMenuItem(listX, itemY, heroW - 24, itemH, item.icon, item.title.c_str(), isSelected);
    }
}

void CrossPlayEngine::renderGamesMenu() {
    m_display->drawText(20, 28, "CROSSPLAY GAMES", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    int cardsPerPage = 6;
    int page = m_selectedSubMenuIndex / cardsPerPage;
    int startIdx = page * cardsPerPage;

    for (int i = 0; i < cardsPerPage; ++i) {
        int itemIdx = startIdx + i;
        if (itemIdx >= static_cast<int>(m_gameMenuItems.size())) break;

        const auto& item = m_gameMenuItems[itemIdx];
        int col = i % 2;
        int row = i / 2;

        int cardX = 20 + col * 370;
        int cardY = 70 + row * 105;
        bool isSelected = (itemIdx == m_selectedSubMenuIndex);

        m_display->drawCard(cardX, cardY, 350, 95, item.title.c_str(), item.description.c_str(), isSelected);
    }

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderAppsMenu() {
    m_display->drawText(20, 28, "CROSSPLAY APPS", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    int cardsPerPage = 6;
    int page = m_selectedSubMenuIndex / cardsPerPage;
    int startIdx = page * cardsPerPage;

    for (int i = 0; i < cardsPerPage; ++i) {
        int itemIdx = startIdx + i;
        if (itemIdx >= static_cast<int>(m_appMenuItems.size())) break;

        const auto& item = m_appMenuItems[itemIdx];
        int col = i % 2;
        int row = i / 2;

        int cardX = 20 + col * 370;
        int cardY = 70 + row * 105;
        bool isSelected = (itemIdx == m_selectedSubMenuIndex);

        m_display->drawCard(cardX, cardY, 350, 95, item.title.c_str(), item.description.c_str(), isSelected);
    }

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderBrowseFiles() {
    m_display->drawText(20, 28, "BROWSE FILES", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    auto books = CrossPlayFileSystem::getInstance().listFiles(CrossPlayFileSystem::getInstance().getBooksDir(), ".epub");

    if (books.empty()) {
        m_display->drawCard(20, 80, 760, 200, "Storage Directory Active",
            "Path: /sdcard/CrossPlay/Books/\nCopy .epub books to this folder or open files via Android File Manager.", false);
    } else {
        int y = 80;
        for (size_t i = 0; i < std::min(books.size(), size_t(4)); ++i) {
            std::string name = books[i].substr(books[i].find_last_of("/\\") + 1);
            m_display->drawCard(20, y, 760, 60, name.c_str(), "Tap to open book", false);
            y += 70;
        }
    }

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderRecentBooks() {
    m_display->drawText(20, 28, "RECENT BOOKS", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawHeroCard(24, 80, 752, 180, "Alice's Adventures\nin Wonderland", "Lewis Carroll - 42% Read");
    m_display->drawCard(20, 280, 760, 70, "The Adventures of Sherlock Holmes", "Arthur Conan Doyle - 15% Read", false);

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderFileTransfer() {
    m_display->drawText(20, 28, "FILE TRANSFER", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 80, 760, 90, "Wi-Fi Direct / Local Network Transfer", "IP Address: http://192.168.1.120:8080\nUpload books from any browser on local network", false);
    m_display->drawCard(20, 190, 760, 90, "Get Books (OPDS Catalog Sync)", "Browse Standard Ebooks & Project Gutenberg public domain catalogs", false);

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderSettings() {
    m_display->drawText(20, 28, "SETTINGS", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 80, 760, 60, "Display Mode: E-Ink Waveform Flash", "Simulate hardware e-ink waveform flash refresh on page turns", false);
    m_display->drawCard(20, 150, 760, 60, "Storage Path: /sdcard/CrossPlay/", "Internal & external SD card storage path", false);
    m_display->drawCard(20, 220, 760, 60, "Network: Wi-Fi Connected", "Status: Active | IP: 192.168.1.120", false);
    m_display->drawCard(20, 290, 760, 60, "Firmware: CrossPlay v1.0 Android Native NDK", "Based on CrossPoint EPUB reader & ESP32-S3 handheld platform", false);

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderEpubReader() {
    m_display->drawText(20, 28, "EPUB READER", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawHeroCard(24, 80, 752, 180, "Alice's Adventures\nin Wonderland", "Lewis Carroll");
    m_display->drawCard(20, 280, 760, 100, "CHAPTER I. Down the Rabbit-Hole", "Alice was beginning to get very tired of sitting by her sister on the bank, and of having nothing to do...", false);

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderFlashcards() {
    m_display->drawText(20, 28, "FLASHCARDS (FSRS)", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 80, 760, 250, "Anki / FSRS Deck Active", "Card 1/50: What is the main compiler used by CrossPlay?\nAnswer: PlatformIO / GCC / Clang", false);

    m_display->drawCard(20, 350, 170, 50, "1 - AGAIN", "1 day", false);
    m_display->drawCard(210, 350, 170, 50, "2 - HARD", "2 days", false);
    m_display->drawCard(400, 350, 170, 50, "3 - GOOD", "4 days", false);
    m_display->drawCard(590, 350, 170, 50, "4 - EASY", "7 days", false);

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderHackerNews() {
    m_display->drawText(20, 28, "HACKER NEWS", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 70, 760, 70, "1. Show HN: CrossPlay on Android", "482 points by crosspoint | 128 comments | 1 hour ago", false);
    m_display->drawCard(20, 150, 760, 70, "2. Fast low-refresh rendering for E-ink displays", "310 points by lowink | 95 comments | 3 hours ago", false);
    m_display->drawCard(20, 230, 760, 70, "3. FSRS: An open spaced-repetition algorithm", "190 points by anki_dev | 42 comments | 5 hours ago", false);

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderXkcd() {
    m_display->drawText(20, 28, "XKCD COMICS", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 70, 760, 310, "#2850 - E-Ink Handhelds", "[xkcd comic drawn 1:1 for 800x480 high contrast display]", false);
    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderGetBooks() {
    m_display->drawText(20, 28, "GET BOOKS (OPDS CATALOG)", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 80, 760, 70, "Standard Ebooks Catalog", "High quality public domain ebooks", false);
    m_display->drawCard(20, 160, 760, 70, "Project Gutenberg", "70,000+ free digital books", false);

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderWikipedia() {
    m_display->drawText(20, 28, "WIKIPEDIA OFFLINE", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 80, 760, 80, "Offline Database: 50,000 Articles", "Database path: /sdcard/CrossPlay/Wikipedia/", false);
    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderCalculator() {
    m_display->drawText(20, 28, "EXACT DECIMAL CALCULATOR", 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 70, 760, 70, "DISPLAY", "12,345.6789", false);

    int startY = 150;
    const char* keys[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"C", "0", "=", "+"}
    };

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            m_display->drawCard(20 + c * 190, startY + r * 55, 180, 50, keys[r][c], nullptr, false);
        }
    }

    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::renderGameGeneric(const char* gameTitle, const char* gameInfo) {
    m_display->drawText(20, 28, gameTitle, 0xFF000000, 3);
    m_display->drawLine(20, 60, EINK_WIDTH - 20, 60, 0xFF000000);

    m_display->drawCard(20, 80, 760, 300, gameTitle, gameInfo, false);
    m_display->drawCard(20, 410, 180, 50, "< BACK HOME", "Return to Main Menu", false);
}

void CrossPlayEngine::handleTouch(int action, float x, float y) {
    if (action != TOUCH_UP) return;

    // Check Back Home button (bottom-left region)
    if (m_currentApp != APP_HOME && x >= 20 && x <= 220 && y >= 400 && y <= 470) {
        if (m_currentApp == APP_GAMES_MENU || m_currentApp == APP_APPS_MENU ||
            m_currentApp == APP_BROWSE_FILES || m_currentApp == APP_RECENT_BOOKS ||
            m_currentApp == APP_FILE_TRANSFER || m_currentApp == APP_SETTINGS) {
            switchApp(APP_HOME);
        } else {
            // Return to sub-menu
            bool isGame = false;
            for (const auto& item : m_gameMenuItems) {
                if (item.id == m_currentApp) { isGame = true; break; }
            }
            if (isGame) switchApp(APP_GAMES_MENU);
            else switchApp(APP_APPS_MENU);
        }
        return;
    }

    if (m_currentApp == APP_HOME) {
        handleHomeTouch(action, x, y);
    } else if (m_currentApp == APP_GAMES_MENU) {
        handleGamesMenuTouch(action, x, y);
    } else if (m_currentApp == APP_APPS_MENU) {
        handleAppsMenuTouch(action, x, y);
    } else if (m_currentApp == APP_FILE_TRANSFER) {
        if (x >= 20 && x <= 780 && y >= 190 && y <= 280) {
            switchApp(APP_GET_BOOKS);
        }
    }
}

void CrossPlayEngine::handleHomeTouch(int action, float x, float y) {
    // Top Hero Reading Card tap -> Open EPUB reader
    if (x >= 24 && x <= 776 && y >= 28 && y <= 208) {
        switchApp(APP_EPUB_READER);
        return;
    }

    // Vertical Menu Items List
    int listX = 36;
    int startY = 224;
    int itemH = 38;
    int spacingY = 40;

    for (size_t i = 0; i < m_homeMenuItems.size(); ++i) {
        int itemY = startY + i * spacingY;
        if (x >= listX && x <= listX + 728 && y >= itemY && y <= itemY + itemH) {
            m_selectedHomeIndex = static_cast<int>(i);
            switchApp(m_homeMenuItems[i].id);
            return;
        }
    }
}

void CrossPlayEngine::handleGamesMenuTouch(int action, float x, float y) {
    int cardsPerPage = 6;
    int page = m_selectedSubMenuIndex / cardsPerPage;

    for (int i = 0; i < cardsPerPage; ++i) {
        int itemIdx = page * cardsPerPage + i;
        if (itemIdx >= static_cast<int>(m_gameMenuItems.size())) break;

        int col = i % 2;
        int row = i / 2;
        int cardX = 20 + col * 370;
        int cardY = 70 + row * 105;

        if (x >= cardX && x <= cardX + 350 && y >= cardY && y <= cardY + 95) {
            m_selectedSubMenuIndex = itemIdx;
            switchApp(m_gameMenuItems[itemIdx].id);
            return;
        }
    }
}

void CrossPlayEngine::handleAppsMenuTouch(int action, float x, float y) {
    int cardsPerPage = 6;
    int page = m_selectedSubMenuIndex / cardsPerPage;

    for (int i = 0; i < cardsPerPage; ++i) {
        int itemIdx = page * cardsPerPage + i;
        if (itemIdx >= static_cast<int>(m_appMenuItems.size())) break;

        int col = i % 2;
        int row = i / 2;
        int cardX = 20 + col * 370;
        int cardY = 70 + row * 105;

        if (x >= cardX && x <= cardX + 350 && y >= cardY && y <= cardY + 95) {
            m_selectedSubMenuIndex = itemIdx;
            switchApp(m_appMenuItems[itemIdx].id);
            return;
        }
    }
}

void CrossPlayEngine::handleButton(int btn, bool isDown) {
    if (!isDown) return;

    if (btn == BTN_HOME || btn == BTN_BACK) {
        switchApp(APP_HOME);
    } else if (btn == BTN_PAGE_NEXT || btn == BTN_DOWN) {
        if (m_currentApp == APP_HOME) {
            m_selectedHomeIndex = (m_selectedHomeIndex + 1) % m_homeMenuItems.size();
        }
        if (m_display) m_display->triggerFlashRefresh();
    } else if (btn == BTN_PAGE_PREV || btn == BTN_UP) {
        if (m_currentApp == APP_HOME) {
            m_selectedHomeIndex = (m_selectedHomeIndex - 1 + m_homeMenuItems.size()) % m_homeMenuItems.size();
        }
        if (m_display) m_display->triggerFlashRefresh();
    } else if (btn == BTN_CONFIRM) {
        if (m_currentApp == APP_HOME) {
            switchApp(m_homeMenuItems[m_selectedHomeIndex].id);
        }
    }
}
