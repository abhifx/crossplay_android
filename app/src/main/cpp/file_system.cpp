#include "file_system.h"
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <android/log.h>

#define LOG_TAG "CrossPlayFS"

CrossPlayFileSystem& CrossPlayFileSystem::getInstance() {
    static CrossPlayFileSystem instance;
    return instance;
}

void CrossPlayFileSystem::initialize(const std::string& internalDir, const std::string& externalDir) {
    m_internalDir = internalDir;
    m_externalDir = externalDir.empty() ? internalDir : externalDir;

    // Create standard CrossPlay folders
    createDirectory(getBooksDir());
    createDirectory(getFlashcardsDir());
    createDirectory(getWikipediaDir());
    createDirectory(getSaveDataDir());
}

std::string CrossPlayFileSystem::getBooksDir() const {
    return m_externalDir + "/CrossPlay/Books";
}

std::string CrossPlayFileSystem::getFlashcardsDir() const {
    return m_externalDir + "/CrossPlay/Flashcards";
}

std::string CrossPlayFileSystem::getWikipediaDir() const {
    return m_externalDir + "/CrossPlay/Wikipedia";
}

std::string CrossPlayFileSystem::getSaveDataDir() const {
    return m_internalDir + "/saves";
}

bool CrossPlayFileSystem::fileExists(const std::string& path) const {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool CrossPlayFileSystem::createDirectory(const std::string& path) const {
    if (path.empty()) return false;
    std::string currentPath;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/')) {
        if (item.empty()) continue;
        currentPath += "/" + item;
        mkdir(currentPath.c_str(), 0755);
    }
    return true;
}

std::vector<std::string> CrossPlayFileSystem::listFiles(const std::string& dirPath, const std::string& extension) const {
    std::vector<std::string> results;
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) return results;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        std::string fileName = entry->d_name;
        if (extension.empty() || (fileName.length() >= extension.length() &&
            fileName.compare(fileName.length() - extension.length(), extension.length(), extension) == 0)) {
            results.push_back(dirPath + "/" + fileName);
        }
    }
    closedir(dir);
    return results;
}

std::string CrossPlayFileSystem::readTextFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool CrossPlayFileSystem::writeTextFile(const std::string& path, const std::string& content) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return true;
}
