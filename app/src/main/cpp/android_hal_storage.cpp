#include "android_hal_storage.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "AndroidHalStorage"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace fs = std::filesystem;

AndroidHalStorage& AndroidHalStorage::getInstance() {
    static AndroidHalStorage instance;
    return instance;
}

void AndroidHalStorage::initialize(const std::string& internalDir, const std::string& externalDir) {
    m_internalDir = internalDir;
    m_externalDir = externalDir.empty() ? internalDir : externalDir;

    m_rootDir = m_externalDir + "/CrossPlay";
    mkdir(m_rootDir);
    mkdir(m_rootDir + "/Books");
    mkdir(m_rootDir + "/Flashcards");
    mkdir(m_rootDir + "/Wikipedia");
    mkdir(m_rootDir + "/Saves");
    mkdir(m_rootDir + "/Cache");

    std::string sampleBook = m_rootDir + "/Books/Sample_Guide.txt";
    if (!exists(sampleBook)) {
        writeFile(sampleBook, "Welcome to CrossPlay on Android!\n\nThis is a sample book file. You can add your EPUB, TXT, or markdown books into /sdcard/CrossPlay/Books/ or browse any folder on your SD card using Browse Files.\n");
    }

    LOGI("Android Storage Initialized at Root: %s", m_rootDir.c_str());
}

std::string AndroidHalStorage::resolvePath(const std::string& path) const {
    std::error_code ec;

    if (path.empty() || path == "/") {
        if (fs::exists("/storage/emulated/0", ec)) return "/storage/emulated/0";
        if (fs::exists("/sdcard", ec)) return "/sdcard";
        return m_externalDir;
    }

    std::string cleanPath = path;
    while (cleanPath.size() > 1 && (cleanPath.back() == '/' || cleanPath.back() == '\\')) {
        cleanPath.pop_back();
    }

    // Paths starting with internal subfolders (like /.crosspoint, /Cache, /Saves) MUST ALWAYS resolve under m_rootDir
    if (cleanPath.rfind("/.", 0) == 0 || cleanPath.rfind("/Cache", 0) == 0 || cleanPath.rfind("/Saves", 0) == 0) {
        return m_rootDir + cleanPath;
    }

    // Check if path is an absolute path on Android device
    if (cleanPath[0] == '/') {
        if (fs::exists(cleanPath, ec)) {
            return cleanPath;
        }

        if (cleanPath.rfind("/sdcard", 0) == 0 ||
            cleanPath.rfind("/storage", 0) == 0 ||
            cleanPath.rfind("/data", 0) == 0 ||
            (!m_internalDir.empty() && cleanPath.rfind(m_internalDir, 0) == 0) ||
            (!m_externalDir.empty() && cleanPath.rfind(m_externalDir, 0) == 0)) {
            return cleanPath;
        }

        // Check app root storage first
        std::string rootPath = m_rootDir + cleanPath;
        if (fs::exists(rootPath, ec)) {
            return rootPath;
        }

        // Check primary shared storage (/storage/emulated/0/Books, /storage/emulated/0/Download, etc.)
        std::string sdPath = "/storage/emulated/0" + cleanPath;
        if (fs::exists(sdPath, ec)) {
            return sdPath;
        }

        std::string extPath = m_externalDir + cleanPath;
        if (fs::exists(extPath, ec)) {
            return extPath;
        }

        return rootPath;
    }

    std::string rootPath = m_rootDir + "/" + cleanPath;
    if (fs::exists(rootPath, ec)) {
        return rootPath;
    }

    std::string sdPath = "/storage/emulated/0/" + cleanPath;
    if (fs::exists(sdPath, ec)) {
        return sdPath;
    }

    std::string extPath = m_externalDir + "/" + cleanPath;
    if (fs::exists(extPath, ec)) {
        return extPath;
    }

    if (cleanPath.rfind(".", 0) == 0 || cleanPath.rfind("Cache", 0) == 0 || cleanPath.rfind("Saves", 0) == 0) {
        return rootPath;
    }

    return rootPath;
}

bool AndroidHalStorage::exists(const std::string& path) const {
    std::string fullPath = resolvePath(path);
    std::error_code ec;
    return fs::exists(fullPath, ec);
}

bool AndroidHalStorage::mkdir(const std::string& path) const {
    std::string fullPath = resolvePath(path);
    std::error_code ec;
    return fs::create_directories(fullPath, ec);
}

bool AndroidHalStorage::remove(const std::string& path) const {
    std::string fullPath = resolvePath(path);
    std::error_code ec;
    return fs::remove(fullPath, ec);
}

bool AndroidHalStorage::removeDir(const std::string& path) const {
    std::string fullPath = resolvePath(path);
    std::error_code ec;
    return fs::remove_all(fullPath, ec) > 0;
}

std::vector<std::string> AndroidHalStorage::listFiles(const std::string& dirPath, const std::string& extension) const {
    std::vector<std::string> results;
    std::string fullPath = resolvePath(dirPath);

    std::error_code ec;
    if (!fs::exists(fullPath, ec) || !fs::is_directory(fullPath, ec)) {
        return results;
    }

    std::string extLower = extension;
    std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);

    for (const auto& entry : fs::directory_iterator(fullPath, fs::directory_options::skip_permission_denied, ec)) {
        if (entry.is_regular_file(ec)) {
            std::string filePath = entry.path().string();
            if (extLower.empty()) {
                results.push_back(filePath);
            } else {
                std::string currentExt = entry.path().extension().string();
                std::transform(currentExt.begin(), currentExt.end(), currentExt.begin(), ::tolower);
                if (currentExt == extLower) {
                    results.push_back(filePath);
                }
            }
        }
    }

    std::sort(results.begin(), results.end());
    return results;
}

std::string AndroidHalStorage::readFile(const std::string& path) const {
    std::string fullPath = resolvePath(path);
    std::ifstream inFile(fullPath, std::ios::in | std::ios::binary);
    if (!inFile.is_open()) return "";

    std::ostringstream ss;
    ss << inFile.rdbuf();
    return ss.str();
}

bool AndroidHalStorage::writeFile(const std::string& path, const std::string& content) const {
    std::string fullPath = resolvePath(path);

    // Ensure parent directory exists
    fs::path p(fullPath);
    if (p.has_parent_path()) {
        mkdir(p.parent_path().string());
    }

    std::ofstream outFile(fullPath, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) return false;

    outFile.write(content.data(), content.size());
    return outFile.good();
}
