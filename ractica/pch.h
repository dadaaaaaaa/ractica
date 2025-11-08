#pragma once

// ============================================================================
// ПРЕДОТВРАЩЕНИЕ ПРЕДУПРЕЖДЕНИЙ
// ============================================================================

#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#define _ITERATOR_DEBUG_LEVEL 0
#define GLM_ENABLE_EXPERIMENTAL
// Отключение предупреждений для сторонних библиотек
#pragma warning(push, 0)
#pragma warning(disable: 4061 4062 4191 4244 4267 4365 4456 4457 4458 4464 4505 4514 4571 4623 4625 4626 4710 4711 4774 4820 5026 5027 5039 5204 5219 5220 5246)

// ============================================================================
// СТАНДАРТНЫЕ БИБЛИОТЕКИ C++
// ============================================================================

// Контейнеры и алгоритмы
#include <vector>
#include <array>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <queue>
#include <stack>
#include <deque>

// Строки и ввод/вывод
#include <string>
#include <string_view>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <format>

// Потоки и синхронизация
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <condition_variable>
#include <shared_mutex>

// Время и дата
#include <chrono>
#include <ctime>

// Математика
#include <cmath>
#include <numbers>
#include <random>
#include <algorithm>
#include <numeric>
#include <functional>
#include <limits>

// Утилиты
#include <memory>
#include <utility>
#include <tuple>
#include <optional>
#include <variant>
#include <any>
#include <type_traits>
#include <concepts>
#include <ranges>

// Файловая система
#include <filesystem>
#include <fstream>

// C библиотеки
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cassert>
#include <cstdint>
#include <cstddef>

// ============================================================================
// OPENGL И ГРАФИЧЕСКИЕ БИБЛИОТЕКИ
// ============================================================================

// GLEW (должен быть перед GLFW)
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM - математика для графики
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/compatibility.hpp>

// ============================================================================
// ШРИФТЫ (FREETYPE)
// ============================================================================

#include <ft2build.h>
#include FT_FREETYPE_H

// ============================================================================
// СЕТЬ И JSON
// ============================================================================

// cURL для сетевых запросов
#include <curl/curl.h>

// JSON для работы с данными
#include <json/json.h>

// ============================================================================
// WINDOWS-СПЕЦИФИЧНЫЕ БИБЛИОТЕКИ
// ============================================================================

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wingdi.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

// ============================================================================
// ВОССТАНОВЛЕНИЕ ПРЕДУПРЕЖДЕНИЙ
// ============================================================================

#pragma warning(pop)

// ============================================================================
// ОБЩИЕ ПСЕВДОНИМЫ И УТИЛИТЫ
// ============================================================================

// Псевдонимы для удобства
using namespace std::chrono_literals;
using namespace std::string_literals;

// Короткие псевдонимы для часто используемых типов
using uint = unsigned int;
using uchar = unsigned char;
using ushort = unsigned short;
using ulong = unsigned long;

// Математические константы
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI / 2.0f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;