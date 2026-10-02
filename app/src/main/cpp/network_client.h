#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <string>

class NetworkClient {
public:
    static NetworkClient& getInstance();

    bool isConnected() const;
    std::string fetchUrl(const std::string& url);
    bool downloadFile(const std::string& url, const std::string& savePath);

private:
    NetworkClient() = default;
};

#endif // NETWORK_CLIENT_H
