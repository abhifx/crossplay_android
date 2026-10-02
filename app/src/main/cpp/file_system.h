#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>
#include <vector>

class CrossPlayFileSystem {
public:
    static CrossPlayFileSystem& getInstance();

    void initialize(const std::string& internalDir, const std::string& externalDir);

    std::string getInternalDir() const { return m_internalDir; }
    std::string getExternalDir() const { return m_externalDir; }
    std::string getBooksDir() const;
    std::string getFlashcardsDir() const;
    std::string getWikipediaDir() const;
    std::string getSaveDataDir() const;

    bool fileExists(const std::string& path) const;
    bool createDirectory(const std::string& path) const;
    std::vector<std::string> listFiles(const std::string& dirPath, const std::string& extension = "") const;

    std::string readTextFile(const std::string& path) const;
    bool writeTextFile(const std::string& path, const std::string& content) const;

private:
    CrossPlayFileSystem() = default;
    std::string m_internalDir;
    std::string m_externalDir;
};

#endif // FILE_SYSTEM_H
