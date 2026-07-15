#pragma once
#include "raylib.h"
#include <vector>


// One particle = position + velocity + remaining lifetime.
// Plain struct, no heap allocation per particle: the whole pool is
// one contiguous vector, which is very cache-friendly to update.
struct Particle
{
    Vector2 pos = { 0.0f, 0.0f };
    Vector2 vel = { 0.0f, 0.0f };
    float life = 0.0f;      // seconds remaining
    float maxLife = 0.0f;
    float size = 1.0f;
    Color color = WHITE;
    bool alive = false;
};


class ParticleSystem
{
public:
    void Init();
    void Emit(Vector2 pos, int count, Color color,
              float speedMin, float speedMax,
              float lifeMin, float lifeMax, float size);
    void Update(float dt);
    void Draw() const;
    void Clear();

    int CountAlive() const {
        int n = 0;
        for (const auto& p : _pool) if (p.alive) n++;
        return n;
    }

private:
    std::vector<Particle> _pool;   // fixed size, slots recycled
    int _next = 0;                 // ring cursor: oldest slot is reused first
    const Texture2D* _tex = nullptr;
};
