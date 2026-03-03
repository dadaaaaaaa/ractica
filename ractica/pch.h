#pragma once

// Стандартные библиотеки
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <memory>
#include <functional>
#include <filesystem>

// Windows
#include <windows.h>

// OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H

// STB Image
#include "stb_image.h"

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Наши заголовки
#include "Core/Constants.h"
#include "Core/Types.h"
#include "../Shared/ConfigTypes.h"
#include "Objects/Sprite.h"
#include "Objects/Bird.h"
#include "Objects/Obstacle.h"