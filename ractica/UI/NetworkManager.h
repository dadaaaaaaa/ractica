#pragma once
#include "../Core/Types.h"
#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <json/json.h>

class NetworkManager {
private:
    std::string serverUrl;

    std::string getLocalIPAddress();
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* response);
    std::string httpPost(const std::string& endpoint, const std::string& data);
    std::string httpGet(const std::string& endpoint);
    bool parseJsonResponse(const std::string& jsonStr, class Json::Value& root);

public:
    NetworkManager(const std::string& url = "https://snake-game.loca.lt");
    ~NetworkManager();

    bool submitHighScore(const std::string& playerName, int score,
        float gameSpeed = 1.0f, int gameDuration = 0,
        int snakeLength = 0);
    void testAllEndpoints();
    HighScore parseHighScoreFromJson(const Json::Value& item);
    std::vector<HighScore> getTopScores();
    std::string getServerStatus();
    bool testConnection();
};