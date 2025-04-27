#pragma once
#include "Drawable.h"
#include <vector>
#include <map>
#include <filesystem>

struct StringStream
{
    const std::string& str;
    size_t place;

    inline bool Done() const { return place >= str.size(); }
    inline void SkipSpaces() { while (!Done() && str[place] == ' ') place++; }
};

std::vector<std::pair<DrawableBase*,bool>> LoadDrawables(std::filesystem::path path);
