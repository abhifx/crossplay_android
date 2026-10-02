#include "android_hal_storage.h"
#include "android_hal_net.h"
#include "android_hal_display.h"

#include <Arduino.h>
#include <Rtc.h>
#include <FrontlightManager.h>
#include <BatteryMonitor.h>
#include <PowerManager.h>
#include <SDCardManager.h>
#include <FreeInkDisplay.h>
#include <Imu.h>
#include <HalDisplay.h>
#include <HalGPIO.h>
#include <activities/Activity.h>
#include <activities/ActivityManager.h>
#include <GfxRenderer.h>
#include <MappedInputManager.h>
#include <SdCardFontSystem.h>
#include <network/HttpDownloader.h>
#include <PanelDriver.h>
#include <HardwareSerial.h>
#include <Wire.h>

#include <ctime>
#include <filesystem>

TwoWire Wire;
HardwareSerial Serial;

class DummyPanelDriver : public freeink::PanelDriver {
public:
    uint32_t spiHz() const override { return 4000000; }
    freeink::BusyPolarity busyPolarity() const override { return freeink::BusyPolarity::ActiveHigh; }
    freeink::PanelGeometry geometry() const override {
        uint16_t w = static_cast<uint16_t>(AndroidHalDisplay::getInstance().getDisplayWidth());
        uint16_t h = static_cast<uint16_t>(AndroidHalDisplay::getInstance().getDisplayHeight());
        uint16_t wb = static_cast<uint16_t>(AndroidHalDisplay::getInstance().getDisplayWidthBytes());
        uint32_t bufSize = static_cast<uint32_t>(AndroidHalDisplay::getInstance().getBufferSize());
        return {w, h, wb, bufSize};
    }
    void begin(freeink::EpdBus&) override {}
    void deepSleep(freeink::EpdBus&) override {}
    void display(freeink::EpdBus&, const uint8_t*, const uint8_t*, freeink::RefreshMode, bool) override {}
    freeink::GrayscaleCapabilities grayscaleCapabilities(freeink::GrayscaleMode = freeink::GrayscaleMode::Overlay) const override {
        return {freeink::GrayscaleEncoding::AbsolutePlanes, freeink::GrayscaleBase::Combined, true, true, true};
    }
};

static DummyPanelDriver dummyPanelDriver;

namespace freeink {
    bool Rtc::begin() { begun_ = true; return true; }
    bool Rtc::now(DateTime& dt) {
        std::time_t t = std::time(nullptr);
        std::tm tm;
        localtime_r(&t, &tm);
        dt = {
            static_cast<uint16_t>(tm.tm_year + 1900),
            static_cast<uint8_t>(tm.tm_mon + 1),
            static_cast<uint8_t>(tm.tm_mday),
            static_cast<uint8_t>(tm.tm_hour),
            static_cast<uint8_t>(tm.tm_min),
            static_cast<uint8_t>(tm.tm_sec),
            static_cast<uint8_t>(tm.tm_wday)
        };
        return true;
    }
    bool Rtc::set(const DateTime&) { return true; }
    bool Rtc::adjust(int32_t, DateTime*) { return true; }

    void PowerManager::powerDownRailsForSleep() {}
    [[noreturn]] void PowerManager::deepSleepUntilPowerButton() { while(true) {} }
    [[noreturn]] void PowerManager::deepSleepUntilPowerButtonOrTimer(uint64_t) { while(true) {} }

    const PanelDriver* uc8179Driver() { return &dummyPanelDriver; }
    const PanelDriver* uc8279X4Driver() { return &dummyPanelDriver; }
    const PanelDriver* ssd1677Driver() { return &dummyPanelDriver; }
    const PanelDriver* uc8279Driver() { return &dummyPanelDriver; }
    const PanelDriver* uc8253X3Driver() { return &dummyPanelDriver; }
    const PanelDriver* ed2208M5Driver() { return &dummyPanelDriver; }

    void EpdBus::begin(const EpdPins&, unsigned int, BusyPolarity, int8_t, int8_t) {}

    bool Imu::begin() { return true; }
    bool Imu::sleep() { return true; }
    bool Imu::wake() { return true; }
    bool Imu::read(Sample&) { return false; }
}

bool FrontlightManager::begin() { return false; }
void FrontlightManager::setBrightness(uint8_t) {}
void FrontlightManager::setBrightnessLevel(uint8_t) {}
void FrontlightManager::off() {}
void FrontlightManager::on() {}
void FrontlightManager::setColorTemperature(uint8_t) {}

BatteryMonitor::BatteryMonitor() {}
bool BatteryMonitor::isCharging() const { return false; }
bool BatteryMonitor::readPercentageChecked(uint16_t& pct) const { pct = 100; return true; }
uint16_t BatteryMonitor::readPercentage() const { return 100; }

SDCardManager::SDCardManager() {}
SDCardManager SDCardManager::instance;
bool SDCardManager::begin() { return true; }
bool SDCardManager::ready() const { return true; }
void SDCardManager::shutdown() {}

uint64_t SDCardManager::sdTotalBytes() const {
    try {
        auto space = std::filesystem::space(AndroidHalStorage::getInstance().getRootDir());
        return space.capacity;
    } catch (...) {
        return 64ULL * 1024 * 1024 * 1024;
    }
}

bool SDCardManager::sdFreeBytes(uint64_t& out) {
    try {
        auto space = std::filesystem::space(AndroidHalStorage::getInstance().getRootDir());
        out = space.available;
        return true;
    } catch (...) {
        out = 56ULL * 1024 * 1024 * 1024;
        return true;
    }
}

uint64_t SDCardManager::sdUsedBytes() {
    uint64_t freeB = 0;
    if (sdFreeBytes(freeB)) {
        uint64_t totalB = sdTotalBytes();
        return (totalB > freeB) ? (totalB - freeB) : 0;
    }
    return 8ULL * 1024 * 1024 * 1024;
}

std::vector<String> SDCardManager::listFiles(const char* path, int maxFiles) {
    std::vector<String> res;
    auto files = AndroidHalStorage::getInstance().listFiles(path ? path : "/");
    for (const auto& f : files) {
        res.push_back(String(f.c_str()));
    }
    return res;
}

bool SDCardManager::readFileToStream(const char* path, Print& out, size_t chunkSize) {
    std::string content = AndroidHalStorage::getInstance().readFile(path ? path : "");
    if (content.empty()) return false;
    out.write(reinterpret_cast<const uint8_t*>(content.data()), content.size());
    return true;
}

size_t SDCardManager::readFileToBuffer(const char* path, char* buffer, size_t bufferSize, size_t maxBytes) {
    if (!buffer || bufferSize == 0) return 0;
    std::string content = AndroidHalStorage::getInstance().readFile(path ? path : "");
    if (content.empty()) return 0;
    size_t toCopy = std::min(content.size(), bufferSize - 1);
    if (maxBytes > 0) toCopy = std::min(toCopy, maxBytes);
    memcpy(buffer, content.data(), toCopy);
    buffer[toCopy] = '\0';
    return toCopy;
}

String SDCardManager::readFile(const char* path) {
    std::string content = AndroidHalStorage::getInstance().readFile(path ? path : "");
    return String(content.c_str());
}

bool SDCardManager::writeFile(const char* path, const String& content) {
    return AndroidHalStorage::getInstance().writeFile(path ? path : "", content.c_str());
}

bool SDCardManager::ensureDirectoryExists(const char* path) {
    return AndroidHalStorage::getInstance().mkdir(path ? path : "");
}

bool SDCardManager::openFileForRead(const char* moduleName, const char* path, FsFile& file) {
    std::string resolved = AndroidHalStorage::getInstance().resolvePath(path ? path : "");
    file = FsFile(resolved, "rb");
    return file.isOpen();
}

bool SDCardManager::openFileForRead(const char* moduleName, const std::string& path, FsFile& file) {
    return openFileForRead(moduleName, path.c_str(), file);
}

bool SDCardManager::openFileForRead(const char* moduleName, const String& path, FsFile& file) {
    return openFileForRead(moduleName, path.c_str(), file);
}

bool SDCardManager::openFileForWrite(const char* moduleName, const char* path, FsFile& file) {
    std::string resolved = AndroidHalStorage::getInstance().resolvePath(path ? path : "");
    std::filesystem::path p(resolved);
    if (p.has_parent_path()) {
        AndroidHalStorage::getInstance().mkdir(p.parent_path().string());
    }
    file = FsFile(resolved, "wb+");
    return file.isOpen();
}

bool SDCardManager::openFileForWrite(const char* moduleName, const std::string& path, FsFile& file) {
    return openFileForWrite(moduleName, path.c_str(), file);
}

bool SDCardManager::openFileForWrite(const char* moduleName, const String& path, FsFile& file) {
    return openFileForWrite(moduleName, path.c_str(), file);
}

bool SDCardManager::removeDir(const char* path) {
    return AndroidHalStorage::getInstance().removeDir(path ? path : "");
}

class CrossPointWebServer {
public:
    enum Surface { Home, Files, Settings, Reader };
    CrossPointWebServer(Surface);
    ~CrossPointWebServer();
    void begin();
    void stop();
    void handleClient();
};
CrossPointWebServer::CrossPointWebServer(Surface) {}
CrossPointWebServer::~CrossPointWebServer() = default;
void CrossPointWebServer::begin() {}
void CrossPointWebServer::stop() {}
void CrossPointWebServer::handleClient() {}

class CrossPointWebServerActivity : public Activity {
public:
    CrossPointWebServerActivity(GfxRenderer& r, MappedInputManager& m);
    ~CrossPointWebServerActivity() override;
    void render(RenderLock&&) override;
};
CrossPointWebServerActivity::CrossPointWebServerActivity(GfxRenderer& r, MappedInputManager& m) : Activity("CrossPointWebServer", r, m) {}
CrossPointWebServerActivity::~CrossPointWebServerActivity() = default;
void CrossPointWebServerActivity::render(RenderLock&&) {}

class SdFirmwareUpdateActivity : public Activity {
public:
    SdFirmwareUpdateActivity(GfxRenderer& r, MappedInputManager& m);
    ~SdFirmwareUpdateActivity() override;
    void render(RenderLock&&) override;
};
SdFirmwareUpdateActivity::SdFirmwareUpdateActivity(GfxRenderer& r, MappedInputManager& m) : Activity("SdFirmwareUpdate", r, m) {}
SdFirmwareUpdateActivity::~SdFirmwareUpdateActivity() = default;
void SdFirmwareUpdateActivity::render(RenderLock&&) {}

class OtaUpdateActivity : public Activity {
public:
    OtaUpdateActivity(GfxRenderer& r, MappedInputManager& m);
    ~OtaUpdateActivity() override;
    void render(RenderLock&&) override;
};
OtaUpdateActivity::OtaUpdateActivity(GfxRenderer& r, MappedInputManager& m) : Activity("OtaUpdate", r, m) {}
OtaUpdateActivity::~OtaUpdateActivity() = default;
void OtaUpdateActivity::render(RenderLock&&) {}

namespace devmode {
    bool holdsRadio() { return false; }
    void pause() {}
    void resume() {}
    int status() { return 0; }
}

void silentRestart() {}
void silentRestartToReader() {}
void silentRestartToSettings() {}
void restartToHomeAfterStorageHandoff() {}
void restartToAppAfterStorageHandoff() {}

int HttpDownloader::lastStatus() { return 200; }

bool HttpDownloader::fetchUrl(const std::string& url, std::string& outContent, const std::string& username, const std::string& password) {
    outContent = AndroidHalNet::getInstance().fetchUrl(url);
    return !outContent.empty();
}

bool HttpDownloader::fetchUrl(const std::string& url, Stream& stream, const std::string& username, const std::string& password) {
    std::string outContent = AndroidHalNet::getInstance().fetchUrl(url);
    if (outContent.empty()) return false;
    stream.write(reinterpret_cast<const uint8_t*>(outContent.data()), outContent.size());
    return true;
}

bool HttpDownloader::fetchUrl(const std::string& url, const DataCallback& onData, const std::string& username, const std::string& password) {
    std::string outContent = AndroidHalNet::getInstance().fetchUrl(url);
    if (outContent.empty()) return false;
    if (onData) {
        return onData(reinterpret_cast<const uint8_t*>(outContent.data()), outContent.size());
    }
    return true;
}

HttpDownloader::DownloadError HttpDownloader::downloadToFile(const std::string& url, const std::string& destPath, ProgressCallback progress, const bool* cancelFlag, const std::string& username, const std::string& password, const std::vector<Header>& headers, bool downgradeRedirectsToHttp) {
    std::string resolvedPath = AndroidHalStorage::getInstance().resolvePath(destPath);
    AndroidHalNet::getInstance().setProgressCallback(progress, cancelFlag);
    bool ok = AndroidHalNet::getInstance().downloadFile(url, resolvedPath);
    bool cancelled = cancelFlag && *cancelFlag;
    AndroidHalNet::getInstance().clearProgressCallback();
    if (cancelled) {
        return ABORTED;
    }
    if (ok) {
        if (progress) progress(100, 100);
        return OK;
    }
    return HTTP_ERROR;
}

GfxRenderer renderer(display);
MappedInputManager mappedInputManager(gpio, renderer);
ActivityManager activityManager(renderer, mappedInputManager);
SdCardFontSystem sdFontSystem;

extern "C" {
    uint32_t uzlib_adler32(const uint8_t*, size_t, uint32_t) { return 0; }
    uint32_t uzlib_crc32(const uint8_t*, size_t, uint32_t) { return 0; }
    int tinfl_decompress(void* r, const uint8_t* pIn_buf_next, size_t* pIn_buf_size, uint8_t* pOut_buf_start, uint8_t* pOut_buf_next, size_t* pOut_buf_size, uint32_t decomp_flags);
    int crosspoint_tinfl_decompress(void* r, const uint8_t* pIn_buf_next, size_t* pIn_buf_size, uint8_t* pOut_buf_start, uint8_t* pOut_buf_next, size_t* pOut_buf_size, uint32_t decomp_flags) {
        return tinfl_decompress(r, pIn_buf_next, pIn_buf_size, pOut_buf_start, pOut_buf_next, pOut_buf_size, decomp_flags);
    }
    void __real_panic_abort() { abort(); }
    void __real_panic_print_backtrace() {}
}
