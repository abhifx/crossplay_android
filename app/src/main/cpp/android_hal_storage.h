#ifndef ANDROID_HAL_STORAGE_H
#define ANDROID_HAL_STORAGE_H

#include <string>
#include <vector>
#include <cstdint>

class AndroidHalStorage {
public:
    static AndroidHalStorage& getInstance();

    void initialize(const std::string& internalDir, const std::string& externalDir);

    std::string getInternalDir() const { return m_internalDir; }
    std::string getExternalDir() const { return m_externalDir; }
    std::string getRootDir() const { return m_rootDir; }

    std::string resolvePath(const std::string& relativeOrAbsolutePath) const;
    bool exists(const std::string& path) const;
    bool mkdir(const std::string& path) const;
    bool remove(const std::string& path) const;
    bool removeDir(const std::string& path) const;

    std::vector<std::string> listFiles(const std::string& dirPath, const std::string& extension = "") const;

    std::string readFile(const std::string& path) const;
    bool writeFile(const std::string& path, const std::string& content) const;

private:
    AndroidHalStorage() = default;
    std::string m_internalDir;
    std::string m_externalDir;
    std::string m_rootDir;
};

#endif // ANDROID_HAL_STORAGE_H
