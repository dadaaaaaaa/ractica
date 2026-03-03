#pragma once
#include <string>
#include "../Shared/ConfigTypes.h"

// ќбъ€влени€ функций из ConfigEditor.cpp, которые нужны в других файлах
bool loadFBXModel(const std::string& filename, ModelData& model, const std::string& subFolder = "");