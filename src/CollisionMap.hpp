#pragma once
#include <string>
#include "raylib.h"

class CollisionMap
{
public:
    void Init(const std::string& name);
    bool IsWalkable(float x, float y) const;
private:
    const Image* _img = nullptr;
};
