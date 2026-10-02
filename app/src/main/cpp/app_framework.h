#ifndef APP_FRAMEWORK_H
#define APP_FRAMEWORK_H

#include "display_eink.h"
#include <string>
#include <vector>

enum AppId {
    APP_HOME = 0,
    APP_BROWSE_FILES = 1,
    APP_RECENT_BOOKS = 2,
    APP_FILE_TRANSFER = 3,
    APP_SETTINGS = 4,
    APP_GAMES_MENU = 5,
    APP_APPS_MENU = 6,

    // Sub-Apps
    APP_EPUB_READER = 10,
    APP_FLASHCARDS = 11,
    APP_HACKER_NEWS = 12,
    APP_XKCD = 13,
    APP_GET_BOOKS = 14,
    APP_INSTAPAPER = 15,
    APP_WIKIPEDIA = 16,
    APP_WALLPAPERS = 17,
    APP_CALCULATOR = 18,

    // Games
    GAME_CHESS = 20,
    GAME_BATTLESHIP = 21,
    GAME_CHECKERS = 22,
    GAME_CONNECT_FOUR = 23,
    GAME_YAHTZEE = 24,
    GAME_KNUCKLEBONES = 25,
    GAME_JAIPUR = 26,
    GAME_SEA_SALT = 27,
    GAME_TOY_BATTLE = 28,
    GAME_GO = 29,
    GAME_CONNECTIONS = 30,
    GAME_SOLITAIRE = 31,
    GAME_D_AND_DIAGRAMS = 32,
    GAME_INSIDER = 33,
    GAME_MURDLE = 34,
    GAME_MINESWEEPER = 35,
    GAME_SUDOKU = 36,
    GAME_PICROSS = 37,
    GAME_FOREHEAD = 38,
    GAME_TRIVIA = 39,
    GAME_WAVELENGTH = 40
};

struct MenuItem {
    int id;
    std::string title;
    std::string category;
    std::string description;
    IconType icon;
};

class CrossPlayEngine {
public:
    static CrossPlayEngine& getInstance();

    void initialize(DisplayEInk* display);
    void render();
    void handleTouch(int action, float x, float y);
    void handleButton(int btn, bool isDown);

    void switchApp(int appId);
    int getCurrentApp() const { return m_currentApp; }

    std::string getStatusText() const;

private:
    CrossPlayEngine();
    DisplayEInk* m_display = nullptr;
    int m_currentApp = APP_HOME;
    int m_selectedHomeIndex = 0;
    int m_selectedSubMenuIndex = 0;

    std::vector<MenuItem> m_homeMenuItems;
    std::vector<MenuItem> m_gameMenuItems;
    std::vector<MenuItem> m_appMenuItems;

    void renderHome();
    void renderGamesMenu();
    void renderAppsMenu();
    void renderBrowseFiles();
    void renderRecentBooks();
    void renderFileTransfer();
    void renderSettings();

    // Specific Apps
    void renderEpubReader();
    void renderFlashcards();
    void renderHackerNews();
    void renderXkcd();
    void renderGetBooks();
    void renderWikipedia();
    void renderCalculator();
    void renderGameGeneric(const char* gameTitle, const char* gameInfo);

    void handleHomeTouch(int action, float x, float y);
    void handleGamesMenuTouch(int action, float x, float y);
    void handleAppsMenuTouch(int action, float x, float y);
};

#endif // APP_FRAMEWORK_H
