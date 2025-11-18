#include "NetworkManager.h"
#include <iostream>

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <random>


NetworkManager::NetworkManager(const std::string& url) : serverUrl(url) {
    // Инициализируем curl глобально
    curl_global_init(CURL_GLOBAL_ALL);
    std::cout << "NetworkManager initialized with server: " << serverUrl << std::endl;
   
}

NetworkManager::~NetworkManager() {
    curl_global_cleanup();
}

std::string NetworkManager::getLocalIPAddress() {
    // Генерируем случайный локальный IP для демонстрации
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 254);

    std::stringstream ip;
    ip << "192.168." << dis(gen) << "." << dis(gen);
    return ip.str();
}

bool NetworkManager::parseJsonResponse(const std::string& jsonStr, Json::Value& root) {
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    std::string errors;

    bool parsingSuccessful = reader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.length(), &root, &errors);

    if (!parsingSuccessful) {
        std::cerr << " JSON parsing failed: " << errors << std::endl;
        return false;
    }

    return true;
}

bool NetworkManager::submitHighScore(const std::string& playerName, int score,
    float gameSpeed, int gameDuration, int snakeLength) {

    std::string ipAddress = getLocalIPAddress();

    std::cout << " Sending high score to server..." << std::endl;
    std::cout << "Player: " << playerName << ", Score: " << score
        << ", Speed: " << gameSpeed << "x, Length: " << snakeLength
        << ", Time: " << gameDuration << "s, IP: " << ipAddress << std::endl;

    // Подготовка JSON данных в snake_case (как ожидает сервер)
    Json::Value scoreData;
    scoreData["player_name"] = playerName;  // snake_case
    scoreData["score"] = score;
    scoreData["game_speed"] = gameSpeed;    // snake_case
    scoreData["game_duration"] = gameDuration; // snake_case
    scoreData["snake_length"] = snakeLength; // snake_case
    scoreData["ip_address"] = ipAddress;    // snake_case

    Json::StreamWriterBuilder writer;
    std::string jsonData = Json::writeString(writer, scoreData);

    try {
        std::string response = httpPost("/api/scores", jsonData);

        // Парсим ответ сервера
        Json::Value responseRoot;
        if (parseJsonResponse(response, responseRoot)) {
            bool success = responseRoot.get("success", false).asBool();
            if (success) {
                std::cout << "High score submitted successfully!" << std::endl;
                std::cout << "Server response: " << responseRoot["message"].asString() << std::endl;
                return true;
            }
            else {
                std::cerr << " Server rejected high score: " << responseRoot["message"].asString() << std::endl;
                return false;
            }
        }
        else {
            std::cerr << " Failed to parse server response" << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << " Failed to submit high score: " << e.what() << std::endl;
        return false;
    }
}

std::vector<HighScore> NetworkManager::getTopScores() {
    std::cout << " Fetching top scores from server..." << std::endl;

    try {
        std::string response = httpGet("/api/scores/top");

        Json::Value root;
        if (parseJsonResponse(response, root)) {
            std::vector<HighScore> scores;

            // Проверяем разные возможные форматы ответа
            if (root.isMember("scores") && root["scores"].isArray()) {
                // Ответ в формате { "scores": [...] }
                for (const auto& item : root["scores"]) {
                    HighScore hs;

                    // Парсим поля в snake_case (как приходит от сервера)
                    if (item.isMember("player_name")) {
                        hs.playerName = item["player_name"].asString();
                    }
                    else if (item.isMember("playerName")) {
                        hs.playerName = item["playerName"].asString();
                    }
                    else {
                        hs.playerName = "Unknown";
                    }

                    hs.score = item.get("score", 0).asInt();

                    // Обрабатываем дату
                    if (item.isMember("date_achieved")) {
                        // Конвертируем ISO дату в простой формат
                        std::string isoDate = item["date_achieved"].asString();
                        hs.date = isoDate.substr(0, 10); // Берем только YYYY-MM-DD
                    }
                    else if (item.isMember("date")) {
                        hs.date = item["date"].asString();
                    }
                    else if (item.isMember("timestamp")) {
                        time_t timestamp = item["timestamp"].asUInt64();
                        struct tm timeinfo;
                        localtime_s(&timeinfo, &timestamp);
                        char buffer[11];
                        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
                        hs.date = buffer;
                    }
                    else {
                        hs.date = "Unknown";
                    }

                    // Парсим скорость в snake_case
                    if (item.isMember("game_speed")) {
                        hs.gameSpeed = item["game_speed"].asFloat();
                    }
                    else {
                        hs.gameSpeed = item.get("gameSpeed", 1.0f).asFloat();
                    }

                    // Парсим длину змейки в snake_case
                    if (item.isMember("snake_length")) {
                        hs.snakeLength = item["snake_length"].asInt();
                    }
                    else {
                        hs.snakeLength = item.get("snakeLength", 0).asInt();
                    }

                    hs.gameDuration = item.get("gameDuration", 0).asInt();

                    scores.push_back(hs);
                }
            }
            else if (root.isArray()) {
                // Ответ в формате массива (если сервер изменится)
                for (const auto& item : root) {
                    HighScore hs;
                    // ... тот же код парсинга что выше ...
                    scores.push_back(hs);
                }
            }
            else {
                std::cerr << " Unexpected JSON format from server" << std::endl;
                std::cout << "Response: " << response << std::endl;
                return std::vector<HighScore>();
            }

            std::sort(scores.begin(), scores.end());
            std::cout << " Loaded " << scores.size() << " scores from server" << std::endl;

            // Выведем для отладки что получили
            for (const auto& score : scores) {
                std::cout << "   - " << score.playerName << ": " << score.score
                    << " (speed: " << score.gameSpeed << "x, length: " << score.snakeLength
                    << ", date: " << score.date << ")" << std::endl;
            }

            return scores;
        }
        else {
            std::cerr << " Failed to parse server response as JSON" << std::endl;
            std::cout << "Raw response: " << response << std::endl;
            return std::vector<HighScore>();
        }
    }
    catch (const std::exception& e) {
        std::cerr << " Failed to fetch scores: " << e.what() << std::endl;
        return std::vector<HighScore>();
    }
}

std::string NetworkManager::getServerStatus() {
    return "Server: " + serverUrl + " (CURL enabled)";
}

bool NetworkManager::testConnection() {
    try {
        std::string response = httpGet("/api/status");

        // Парсим ответ для проверки
        Json::Value root;
        if (parseJsonResponse(response, root)) {
            bool online = root.get("online", false).asBool();
            if (online) {
                std::cout << " Server is online and responding" << std::endl;
                return true;
            }
        }

        std::cerr << " Server is not responding properly" << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << " Server connection test failed: " << e.what() << std::endl;
        return false;
    }
}

size_t NetworkManager::writeCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

std::string NetworkManager::httpPost(const std::string& endpoint, const std::string& data) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        std::string url = serverUrl + endpoint;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "3D-Snake-Game/1.0");

        // Устанавливаем таймауты
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 секунд таймаут
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // 5 секунд на подключение

        // Для JSON
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << " curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            throw std::runtime_error(curl_easy_strerror(res));
        }

        // Получаем HTTP код ответа
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code != 200) {
            std::cerr << " HTTP Error: " << http_code << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return response;
    }
    else {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

std::string NetworkManager::httpGet(const std::string& endpoint) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        std::string url = serverUrl + endpoint;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "3D-Snake-Game/1.0");

        // Устанавливаем таймауты
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << " curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            throw std::runtime_error(curl_easy_strerror(res));
        }

        // Получаем HTTP код ответа
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code != 200) {
            std::cerr << " HTTP Error: " << http_code << std::endl;
        }

        curl_easy_cleanup(curl);

        return response;
    }
    else {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

HighScore NetworkManager::parseHighScoreFromJson(const Json::Value& item) {
    HighScore hs;

    // Проверяем разные возможные имена полей
    if (item.isMember("playerName")) {
        hs.playerName = item["playerName"].asString();
    }
    else if (item.isMember("player_name")) {
        hs.playerName = item["player_name"].asString();
    }
    else if (item.isMember("name")) {
        hs.playerName = item["name"].asString();
    }
    else {
        hs.playerName = "Unknown";
    }

    hs.score = item.get("score", 0).asInt();

    // Обрабатываем дату
    if (item.isMember("date")) {
        hs.date = item["date"].asString();
    }
    else if (item.isMember("created_at")) {
        hs.date = item["created_at"].asString();
    }
    else if (item.isMember("timestamp")) {
        time_t timestamp = item["timestamp"].asUInt64();
        struct tm timeinfo;
        localtime_s(&timeinfo, &timestamp);
        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
        hs.date = buffer;
    }
    else {
        hs.date = "Unknown";
    }

    hs.gameSpeed = item.get("gameSpeed", 1.0f).asFloat();
    hs.snakeLength = item.get("snakeLength", 0).asInt();
    hs.gameDuration = item.get("gameDuration", 0).asInt();

    return hs;
}