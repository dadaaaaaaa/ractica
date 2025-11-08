#pragma once

// Стандартные библиотеки
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>
#include <cmath>
#include <fstream>
#include <sstream>
#include <map>
#include <ft2build.h>
#include <chrono>
#include <iomanip>
// чтобы убрать предупреждения
// OpenGL и графические библиотеки
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// Windows-специфичные библиотеки для шрифтов
#ifdef _WIN32
#include <windows.h>
#include <wingdi.h>
#endif