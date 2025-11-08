#include "../pch.h"
#include "NetworkManager.h"


// ============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// ============================================================================

NetworkManager::NetworkManager(const std::string& url)
    : serverUrl(url),
    isConnected(false),
    timeoutSeconds(NETWORK_TIMEOUT_SECONDS),
    connectTimeoutSeconds(NETWORK_CONNECT_TIMEOUT_SECONDS) {

    // Инициализируем curl глобально
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        std::cerr << "❌ Failed to initialize CURL: " << curl_easy_strerror(res) << std::endl;
        return;
    }

    if (ENABLE_DEBUG_INFO) {
        std::cout << "🌐 NetworkManager initialized with server: " << serverUrl << std::endl;
    }

    // Тестируем подключение при инициализации
    isConnected = testConnection();
}

NetworkManager::~NetworkManager() {
    curl_global_cleanup();
    if (ENABLE_DEBUG_INFO) {
        std::cout << "🌐 NetworkManager cleaned up" << std::endl;
    }
}

// ============================================================================
// СТАТИЧЕСКИЕ УТИЛИТЫ
// ============================================================================

std::string NetworkManager::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string NetworkManager::formatDuration(int seconds) {
    int minutes = seconds / 60;
    int secs = seconds % 60;

    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << minutes << ":"
        << std::setw(2) << std::setfill('0') << secs;
    return ss.str();
}

std::string NetworkManager::generateRequestId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);

    std::stringstream ss;
    ss << "REQ_" << std::hex << std::time(nullptr) << "_" << dis(gen);
    return ss.str();
}

// ============================================================================
// CALLBACK ФУНКЦИИ CURL
// ============================================================================

size_t NetworkManager::writeCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ
// ============================================================================

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

    if (!parsingSuccessful && LOG_NETWORK_REQUESTS) {
        std::cerr << "❌ JSON parsing failed: " << errors << std::endl;
        return false;
    }

    return parsingSuccessful;
}

void NetworkManager::logRequest(const std::string& endpoint, const std::string& method, const std::string& data) {
    if (!LOG_NETWORK_REQUESTS) return;

    std::cout << "📡 [" << getCurrentTimestamp() << "] " << method << " " << endpoint << std::endl;
    if (!data.empty()) {
        std::cout << "   Data: " << data.substr(0, 200) << (data.length() > 200 ? "..." : "") << std::endl;
    }
}

void NetworkManager::logResponse(const std::string& endpoint, const std::string& response, long httpCode) {
    if (!LOG_NETWORK_REQUESTS) return;

    std::cout << "📡 [" << getCurrentTimestamp() << "] Response from " << endpoint
        << " (HTTP " << httpCode << ")" << std::endl;
    if (!response.empty()) {
        std::cout << "   Response: " << response.substr(0, 200) << (response.length() > 200 ? "..." : "") << std::endl;
    }
}

bool NetworkManager::handleCurlError(CURLcode res, const std::string& operation) {
    if (res != CURLE_OK) {
        std::cerr << "❌ CURL error in " << operation << ": " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    return true;
}

bool NetworkManager::handleHttpError(long httpCode, const std::string& endpoint) {
    if (httpCode != 200) {
        std::cerr << "❌ HTTP Error " << httpCode << " from " << endpoint << std::endl;
        return false;
    }
    return true;
}

// ============================================================================
// ОСНОВНЫЕ HTTP МЕТОДЫ
// ============================================================================

std::string NetworkManager::httpPost(const std::string& endpoint, const std::string& data) {
    CURL* curl = curl_easy_init();
    std::string response;
    std::string requestId = generateRequestId();

    if (curl) {
        std::string url = serverUrl + endpoint;

        logRequest(endpoint, "POST", data);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, NETWORK_USER_AGENT.c_str());

        // Устанавливаем таймауты из констант
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectTimeoutSeconds);

        // Для JSON
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, ("X-Request-ID: " + requestId).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);

        // Получаем HTTP код ответа
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        logResponse(endpoint, response, http_code);

        if (!handleCurlError(res, "POST " + endpoint)) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::runtime_error("CURL operation failed");
        }

        if (!handleHttpError(http_code, endpoint)) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::runtime_error("HTTP error: " + std::to_string(http_code));
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
    std::string requestId = generateRequestId();

    if (curl) {
        std::string url = serverUrl + endpoint;

        logRequest(endpoint, "GET", "");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, NETWORK_USER_AGENT.c_str());

        // Устанавливаем таймауты из констант
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSeconds);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectTimeoutSeconds);

        // Добавляем заголовки
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, ("X-Request-ID: " + requestId).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);

        // Получаем HTTP код ответа
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        logResponse(endpoint, response, http_code);

        if (!handleCurlError(res, "GET " + endpoint)) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::runtime_error("CURL operation failed");
        }

        if (!handleHttpError(http_code, endpoint)) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::runtime_error("HTTP error: " + std::to_string(http_code));
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return response;
    }
    else {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

// ============================================================================
// ОСНОВНЫЕ МЕТОДЫ БИЗНЕС-ЛОГИКИ
// ============================================================================

bool NetworkManager::submitHighScore(const std::string& playerName, int score,
    float gameSpeed, int gameDuration, int snakeLength) {

    if (score < MIN_SCORE_FOR_HIGHSCORE) {
        if (ENABLE_DEBUG_INFO) {
            std::cout << "⚠️ Score too low (" << score << "), not submitting to server" << std::endl;
        }
        return false;
    }

    std::string ipAddress = getLocalIPAddress();

    if (ENABLE_DEBUG_INFO) {
        std::cout << "📡 Sending high score to server..." << std::endl;
        std::cout << "   Player: " << playerName << ", Score: " << score
            << ", Speed: " << gameSpeed << "x, Length: " << snakeLength
            << ", Time: " << gameDuration << "s, IP: " << ipAddress << std::endl;
    }

    // Подготовка JSON данных в snake_case (как ожидает сервер)
    Json::Value scoreData;
    scoreData["player_name"] = playerName.substr(0, MAX_PLAYER_NAME_LENGTH);
    scoreData["score"] = score;
    scoreData["game_speed"] = gameSpeed;
    scoreData["game_duration"] = gameDuration;
    scoreData["snake_length"] = snakeLength;
    scoreData["ip_address"] = ipAddress;
    scoreData["timestamp"] = static_cast<Json::UInt64>(std::time(nullptr));

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string jsonData = Json::writeString(writer, scoreData);

    try {
        std::string response = httpPost("/api/scores", jsonData);

        // Парсим ответ сервера
        Json::Value responseRoot;
        if (parseJsonResponse(response, responseRoot)) {
            bool success = responseRoot.get("success", false).asBool();
            if (success) {
                if (ENABLE_DEBUG_INFO) {
                    std::cout << "✅ High score submitted successfully!" << std::endl;
                    std::cout << "   Server response: " << responseRoot["message"].asString() << std::endl;
                }
                return true;
            }
            else {
                std::cerr << "❌ Server rejected high score: " << responseRoot["message"].asString() << std::endl;
                return false;
            }
        }
        else {
            std::cerr << "❌ Failed to parse server response" << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Failed to submit high score: " << e.what() << std::endl;
        return false;
    }
}

std::vector<HighScore> NetworkManager::getTopScores(int limit) {
    if (ENABLE_DEBUG_INFO) {
        std::cout << "📡 Fetching top " << limit << " scores from server..." << std::endl;
    }

    try {
        std::string endpoint = "/api/scores/top?limit=" + std::to_string(limit);
        std::string response = httpGet(endpoint);

        Json::Value root;
        if (parseJsonResponse(response, root)) {
            std::vector<HighScore> scores;

            // Проверяем разные возможные форматы ответа
            if (root.isMember("scores") && root["scores"].isArray()) {
                // Ответ в формате { "scores": [...] }
                for (const auto& item : root["scores"]) {
                    HighScore hs;

                    // Парсим поля в snake_case (как приходит от сервера)
                    hs.playerName = item.get("player_name", "Unknown").asString();
                    hs.score = item.get("score", 0).asInt();

                    // Обрабатываем дату
                    if (item.isMember("date_achieved")) {
                        std::string isoDate = item["date_achieved"].asString();
                        hs.date = isoDate.substr(0, 10); // Берем только YYYY-MM-DD
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

                    hs.gameSpeed = item.get("game_speed", 1.0f).asFloat();
                    hs.snakeLength = item.get("snake_length", 0).asInt();
                    hs.gameDuration = item.get("game_duration", 0).asInt();

                    scores.push_back(hs);
                }
            }
            else if (root.isArray()) {
                // Ответ в формате массива
                for (const auto& item : root) {
                    HighScore hs;
                    hs.playerName = item.get("player_name", "Unknown").asString();
                    hs.score = item.get("score", 0).asInt();

                    if (item.isMember("date_achieved")) {
                        std::string isoDate = item["date_achieved"].asString();
                        hs.date = isoDate.substr(0, 10);
                    }
                    else {
                        hs.date = "Unknown";
                    }

                    hs.gameSpeed = item.get("game_speed", 1.0f).asFloat();
                    hs.snakeLength = item.get("snake_length", 0).asInt();
                    hs.gameDuration = item.get("game_duration", 0).asInt();

                    scores.push_back(hs);
                }
            }
            else {
                std::cerr << "❌ Unexpected JSON format from server" << std::endl;
                if (LOG_NETWORK_REQUESTS) {
                    std::cout << "Response: " << response << std::endl;
                }
                return std::vector<HighScore>();
            }

            // Сортируем по убыванию счета
            std::sort(scores.begin(), scores.end(), [](const HighScore& a, const HighScore& b) {
                return a.score > b.score;
                });

            if (ENABLE_DEBUG_INFO) {
                std::cout << "✅ Loaded " << scores.size() << " scores from server" << std::endl;

                if (LOG_NETWORK_REQUESTS) {
                    for (const auto& score : scores) {
                        std::cout << "   - " << score.playerName << ": " << score.score
                            << " (speed: " << score.gameSpeed << "x, length: " << score.snakeLength
                            << ", date: " << score.date << ")" << std::endl;
                    }
                }
            }

            return scores;
        }
        else {
            std::cerr << "❌ Failed to parse server response as JSON" << std::endl;
            if (LOG_NETWORK_REQUESTS) {
                std::cout << "Raw response: " << response << std::endl;
            }
            return std::vector<HighScore>();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Failed to fetch scores: " << e.what() << std::endl;
        return std::vector<HighScore>();
    }
}

bool NetworkManager::testConnection() {
    try {
        std::string response = httpGet("/api/status");

        Json::Value root;
        if (parseJsonResponse(response, root)) {
            bool online = root.get("online", false).asBool();
            if (online) {
                if (ENABLE_DEBUG_INFO) {
                    std::cout << "✅ Server is online and responding" << std::endl;
                }
                return true;
            }
        }

        std::cerr << "❌ Server is not responding properly" << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Server connection test failed: " << e.what() << std::endl;
        return false;
    }
}

std::string NetworkManager::getServerStatus() const {
    std::string status = "Server: " + serverUrl;
    status += isConnected ? " (Connected)" : " (Disconnected)";
    status += " | Timeout: " + std::to_string(timeoutSeconds) + "s";
    return status;
}