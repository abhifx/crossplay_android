cmake_minimum_required(VERSION 3.22.1)

project("crossplay_native")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to upstream CrossPlay repository
set(CROSSPLAY_UPSTREAM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../crossplay_upstream")

# Force-include Arduino.h in every translation unit (like PlatformIO/Arduino toolchains)
add_compile_options(-include "${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/Arduino.h")

# Include directories - sim-stubs MUST be first to shadow hardware headers with simulation stubs
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/ArduinoJsonRepo/src
    ${CROSSPLAY_UPSTREAM_DIR}/src
    ${CROSSPLAY_UPSTREAM_DIR}/src/apps_local

    # FreeInk SDK includes (SDK headers take precedence over legacy lib headers)
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/ui/FreeInkUI/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/assets/Icons/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/assets/ImageToneMapper/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/font/FreeInkFont/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/font/FreeInkFont/third_party/freetype/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/font/FreeInkFont/third_party/stb
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/book/ContentProtection/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/book/FreeInkBook/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/SecureNet/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/JsonSax/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/CatalogList/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/Opds/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/display/FreeInkDisplay/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/BoardConfig/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/InputManager/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/SDCardManager/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/PowerManager/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/BatteryMonitor/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/Imu/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/Rtc/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/EnvironmentSensor/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/Buzzer/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/LedManager/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/FrontlightManager/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/AudioManager/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/BleKeyboardHost/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/NearbyTransfer/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/XteinkDetect/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/MemoryManager/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/Microphone/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/UsbMassStorage/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/RecoveryBoot/include
    ${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/display/FreeInkDisplay/src/driver

    ${CROSSPLAY_UPSTREAM_DIR}/lib
    ${CROSSPLAY_UPSTREAM_DIR}/lib/hal
    ${CROSSPLAY_UPSTREAM_DIR}/lib/EpdFont
    ${CROSSPLAY_UPSTREAM_DIR}/lib/Epub
    ${CROSSPLAY_UPSTREAM_DIR}/lib/Txt
    ${CROSSPLAY_UPSTREAM_DIR}/lib/Xtc
    ${CROSSPLAY_UPSTREAM_DIR}/lib/I18n
    ${CROSSPLAY_UPSTREAM_DIR}/lib/Utf8
    ${CROSSPLAY_UPSTREAM_DIR}/lib/GfxRenderer
    ${CROSSPLAY_UPSTREAM_DIR}/lib/OpdsParser
    ${CROSSPLAY_UPSTREAM_DIR}/lib/KOReaderSync
    ${CROSSPLAY_UPSTREAM_DIR}/lib/LibraryIndex
    ${CROSSPLAY_UPSTREAM_DIR}/lib/JsonParser
    ${CROSSPLAY_UPSTREAM_DIR}/lib/decNumber
    ${CROSSPLAY_UPSTREAM_DIR}/lib/decNumber/include
    ${CROSSPLAY_UPSTREAM_DIR}/lib/decNumber/src
    ${CROSSPLAY_UPSTREAM_DIR}/lib/expat
    ${CROSSPLAY_UPSTREAM_DIR}/lib/miniz
    ${CROSSPLAY_UPSTREAM_DIR}/lib/miniz/src
    ${CROSSPLAY_UPSTREAM_DIR}/lib/uzlib
    ${CROSSPLAY_UPSTREAM_DIR}/lib/uzlib/src
    ${CROSSPLAY_UPSTREAM_DIR}/lib/zstd
    ${CROSSPLAY_UPSTREAM_DIR}/lib/zstd/include
    ${CROSSPLAY_UPSTREAM_DIR}/lib/zstd/src
    ${CROSSPLAY_UPSTREAM_DIR}/lib/Memory
    ${CROSSPLAY_UPSTREAM_DIR}/lib/Logging
    ${CROSSPLAY_UPSTREAM_DIR}/lib/ZipFile
    ${CROSSPLAY_UPSTREAM_DIR}/lib/DevInput
    ${CROSSPLAY_UPSTREAM_DIR}/lib/MiniBidi
    ${CROSSPLAY_UPSTREAM_DIR}/lib/TrustedTime
    ${CROSSPLAY_UPSTREAM_DIR}/lib/FsHelpers
    ${CROSSPLAY_UPSTREAM_DIR}/lib/Serialization
    ${CROSSPLAY_UPSTREAM_DIR}/lib/XmlParserUtils
    ${CROSSPLAY_UPSTREAM_DIR}/lib/PngToBmpConverter
    ${CROSSPLAY_UPSTREAM_DIR}/lib/JpegToBmpConverter
    ${CROSSPLAY_UPSTREAM_DIR}/lib/InflateReader
)

# NDK Native App Glue
set(NATIVE_APP_GLUE_DIR "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
include_directories(${NATIVE_APP_GLUE_DIR})

# Collect upstream source files
file(GLOB_RECURSE UPSTREAM_SRC_FILES CONFIGURE_DEPENDS
    "${CROSSPLAY_UPSTREAM_DIR}/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/src/*.c"
    "${CROSSPLAY_UPSTREAM_DIR}/src/**/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/src/**/*.c"
    "${CROSSPLAY_UPSTREAM_DIR}/lib/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/lib/*.c"
    "${CROSSPLAY_UPSTREAM_DIR}/lib/**/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/lib/**/*.c"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/ui/FreeInkUI/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/ui/FreeInkUI/src/*.c"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/font/FreeInkFont/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/font/FreeInkFont/src/*.c"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/book/ContentProtection/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/book/ContentProtection/src/*.c"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/assets/ImageToneMapper/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/hardware/MemoryManager/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/display/FreeInkDisplay/src/FreeInkDisplay.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/SecureNet/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/SecureNet/src/*.c"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/JsonSax/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/CatalogList/src/*.cpp"
    "${CROSSPLAY_UPSTREAM_DIR}/freeink-sdk/libs/network/Opds/src/*.cpp"
)

# Exclude main.cpp (firmware entry point) and ESP32-specific hardware/webserver/OTA files
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/src/main\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/FirmwareFlasher\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/OtaBootSwitch\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/OtaUpdater\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/FirmwareBoardTag\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/CrossPointWebServer\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/CrossPointWebServerActivity\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/CalibreConnectActivity\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/HttpDownloader\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/network/WebDAVHandler\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/settings/SdFirmwareUpdateActivity\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/settings/OtaUpdateActivity\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/src/DevMode\\.cpp$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/platform/skip_efuse_blk_check\\.c$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/decNumber/src/.*_impl\\.c$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/miniz/src/.*_impl\\.c$")
list(FILTER UPSTREAM_SRC_FILES EXCLUDE REGEX ".*/lib/OpdsParser/OpdsParser\\.cpp$")

add_compile_definitions(
    SIMULATOR
    FREEINK_DEVICE_X4PRO
    CROSSPOINT_EMULATED=1
    CROSSPOINT_SIMULATOR_PROJECT_WEBSERVER
    CROSSPOINT_VERSION="dev-android"
    EINK_DISPLAY_SINGLE_BUFFER_MODE=1
    MINIZ_NO_ZLIB_COMPATIBLE_NAMES=1
    XML_GE=0
    XML_CONTEXT_BYTES=1024
    USE_UTF8_LONG_NAMES=1
    PNG_MAX_BUFFERED_PIXELS=262144
    DISABLE_FS_H_WARNING=1
    DESTRUCTOR_CLOSES_FILE=1
    SIMULATOR_DEVICE_X4_PRO
    FT2_BUILD_LIBRARY
)

add_library(
    crossplay_native
    SHARED
    ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c
    crossplay_bridge.cpp
    android_hal_display.cpp
    android_hal_storage.cpp
    android_hal_input.cpp
    android_hal_net.cpp
    display_eink.cpp
    app_framework.cpp
    file_system.cpp
    network_client.cpp
    hardware_stubs.cpp
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/JPEGDEC.cpp
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/PNGdec.cpp
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/zutil.cpp
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/inftrees.cpp
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/inflate.cpp
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/inffast.cpp
    ${CROSSPLAY_UPSTREAM_DIR}/sim-stubs/adler32.cpp
    ${UPSTREAM_SRC_FILES}
)

find_library(log-lib log)
find_library(android-lib android)
find_library(jnigraphics-lib jnigraphics)
find_library(egl-lib EGL)
find_library(gles-lib GLESv2)

target_link_libraries(
    crossplay_native
    ${log-lib}
    ${android-lib}
    ${jnigraphics-lib}
    ${egl-lib}
    ${gles-lib}
    z
)
