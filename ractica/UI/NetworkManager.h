#pragma once

#include <string>
#include <vector>
#include <curl/curl.h>
#include <json/json.h>
#include "../Core/Constants.h"
#include "../Core/Types.h"
#include "../pch.h"
class NetworkManager {
private:
    std::string serverUrl;
    bool isConnected;
    int timeoutSeconds;
    int connectTimeoutSeconds;

    // Callback функция для записи ответа
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* response);

    // Вспомогательные методы
    std::string getLocalIPAddress();
    bool parseJsonResponse(const std::string& jsonStr, Json::Value& root);
    std::string httpPost(const std::string& endpoint, const std::string& data);
    std::string httpGet(const std::string& endpoint);

    // Методы для обработки ошибок
    bool handleCurlError(CURLcode res, const std::string& operation);
    bool handleHttpError(long httpCode, const std::string& endpoint);

    // Логирование
    void logRequest(const std::string& endpoint, const std::string& method, const std::string& data = "");
    void logResponse(const std::string& endpoint, const std::string& response, long httpCode);

public:
    NetworkManager(const std::string& url = DEFAULT_SERVER_URL);
    ~NetworkManager();

    // Основные методы
    bool submitHighScore(const std::string& playerName, int score,
        float gameSpeed = 1.0f, int gameDuration = 0,
        int snakeLength = 0);

    std::vector<HighScore> getTopScores(int limit = HIGH_SCORES_DISPLAY_LIMIT);
    bool testConnection();

    // Геттеры
    std::string getServerStatus() const;
    bool isServerConnected() const { return isConnected; }
    std::string getServerUrl() const { return serverUrl; }

    // Сеттеры
    void setServerUrl(const std::string& url) { serverUrl = url; }
    void setTimeout(int seconds) { timeoutSeconds = seconds; }
    void setConnectTimeout(int seconds) { connectTimeoutSeconds = seconds; }

    // Статические утилиты
    static std::string getCurrentTimestamp();
    static std::string formatDuration(int seconds);
    static std::string generateRequestId();
};