#pragma once
#include "EnemyType.hpp"


// One row per level — same data-driven pattern as EnemyTypeConfig.
struct LevelConfig
{
    int   waveCount;                  // waves to clear this level
    float speedMult;                  // enemy speed multiplier
    int   hpBonus;                    // added to each enemy's base HP
    bool  allowed[ENEMY_TYPE_COUNT];  // spawn mix: cockroach, scorpion, kliver, troll
};

constexpr int LEVEL_COUNT = 5;

inline const LevelConfig& GetLevelConfig(int level)  // 1-based
{
    static const LevelConfig table[LEVEL_COUNT] = {
        // waves speed  hp+  roach  scorp  kliver troll
        {  4,    1.00f, 0, { true,  false, false, false } },
        {  4,    1.05f, 0, { true,  true,  false, false } },
        {  5,    1.10f, 0, { true,  true,  true,  false } },
        {  5,    1.15f, 1, { true,  true,  true,  true  } },
        {  6,    1.25f, 1, { true,  true,  true,  true  } },
    };
    if (level < 1) level = 1;
    if (level > LEVEL_COUNT) level = LEVEL_COUNT;
    return table[level - 1];
}
