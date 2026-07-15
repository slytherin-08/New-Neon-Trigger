#pragma once
#include "ResourceKeys.hpp"


enum class EnemyType { Cockroach, Scorpion, Kliver, Troll };
constexpr int ENEMY_TYPE_COUNT = 4;


// All per-type tuning lives in this one table (data-driven design):
// adding a new enemy = one texture pair + one row here.
struct EnemyTypeConfig
{
    const char* moveKey;
    const char* deathKey;
    int   moveFrames;
    float moveFps;
    int   deathFrames;
    float deathFps;
    float speed;
    int   hp;
    float colliderRadius;
    int   score;    // awarded on kill
    const char* deathSndKey;
    const char* gruntSndKey;
};

inline const EnemyTypeConfig& GetEnemyConfig(EnemyType type)
{
    static const EnemyTypeConfig table[ENEMY_TYPE_COUNT] = {
        // moveKey              deathKey               mvFr mvFps dthFr dthFps speed   hp radius score deathSnd               gruntSnd
        { RK::COCKROACH_MOVE,   RK::COCKROACH_DEATH,   8,   8.0f, 32,   64.0f,  80.0f, 1, 30.0f, 10,   RK::SND_ZOMBIE_DEATH,  RK::SND_ZOMBIE_GRUNT },
        { RK::SCORPION_MOVE,    RK::SCORPION_DEATH,    4,   8.0f,  8,   16.0f, 110.0f, 2, 24.0f, 20,   RK::SND_MUTANT_DEATH,  RK::SND_MUTANT_GRUNT },
        { RK::KLIVER_MOVE,      RK::KLIVER_DEATH,      8,   8.0f, 16,   32.0f,  60.0f, 3, 30.0f, 30,   RK::SND_MUTANT_DEATH,  RK::SND_MUTANT_GRUNT },
        { RK::TROLL_MOVE,       RK::TROLL_DEATH,       8,   8.0f, 32,   64.0f,  40.0f, 5, 32.0f, 50,   RK::SND_ZOMBIE_DEATH,  RK::SND_GRUNT_GRUNT },
    };
    return table[(int)type];
}
