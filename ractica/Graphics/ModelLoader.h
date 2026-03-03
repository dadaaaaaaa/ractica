#pragma once
#include <string>
#include "../Shared/ConfigTypes.h"

// Объявление функции загрузки моделей
bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder);