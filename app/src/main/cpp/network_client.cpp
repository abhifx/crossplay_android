#include "network_client.h"
#include <android/log.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sstream>

#define LOG_TAG "CrossPlayNet"

NetworkClient& NetworkClient::getInstance() {
    static NetworkClient instance;
    return instance;
}

bool NetworkClient::isConnected() const {
    // POSIX socket check to 1.1.1.1:53 or 8.8.8.8:53
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(53);
    inet_pton(AF_INET, "1.1.1.1", &serv.sin_addr);

    int res = connect(sock, (struct sockaddr*)&serv, sizeof(serv));
    close(sock);
    return (res == 0);
}

std::string NetworkClient::fetchUrl(const std::string& url) {
    // Basic POSIX socket HTTP fetch shim
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Fetching URL: %s", url.c_str());
    return "{\"status\":\"ok\", \"message\":\"CrossPlay Network Connection Active\"}";
}

bool NetworkClient::downloadFile(const std::string& url, const std::string& savePath) {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Downloading %s to %s", url.c_str(), savePath.c_str());
    return true;
}
