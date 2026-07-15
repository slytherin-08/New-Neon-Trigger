#include "Particles.hpp"
#include "ResourceKeys.hpp"
#include "ResourceManager.hpp"
#include "GameConfig.hpp"
#include "raymath.h"      // must precede SwarmUtils.hpp: it defines the Vector2 operators
#include "SwarmUtils.hpp"


void ParticleSystem::Init()
{
    _tex = &RM::get().GetTexture(RK::PARTICLE_DOT);
    _pool.resize(GameConfig::PARTICLE_POOL_SIZE);
}

// Burst emitter: 'count' particles fly out in random directions with
// random speed/lifetime in the given ranges. Uses a ring cursor instead
// of searching for dead slots — if the pool is full, the oldest
// particles simply get overwritten (visually unnoticeable).
void ParticleSystem::Emit(Vector2 pos, int count, Color color,
                          float speedMin, float speedMax,
                          float lifeMin, float lifeMax, float size)
{
    for (int i = 0; i < count; i++)
    {
        Particle& p = _pool[_next];
        _next = (_next + 1) % (int)_pool.size();

        float angle = RandomFloat(0.0f, 360.0f);
        float speed = RandomFloat(speedMin, speedMax);

        p.pos = pos;
        p.vel = Direction(angle) * speed;
        p.maxLife = RandomFloat(lifeMin, lifeMax);
        p.life = p.maxLife;
        p.size = size;
        p.color = color;
        p.alive = true;
    }
}

void ParticleSystem::Update(float dt)
{
    for (auto& p : _pool)
    {
        if (!p.alive) continue;

        p.life -= dt;
        if (p.life <= 0.0f)
        {
            p.alive = false;
            continue;
        }

        p.pos += p.vel * dt;
        // Exponential-ish drag: particles burst out fast, then settle.
        p.vel *= 1.0f - GameConfig::PARTICLE_DRAG * dt;
    }
}

void ParticleSystem::Draw() const
{
    if (!_tex) return;

    for (const auto& p : _pool)
    {
        if (!p.alive) continue;

        float t = p.life / p.maxLife;               // 1 at birth -> 0 at death
        Color c = ColorAlpha(p.color, t);           // fade out
        float s = p.size * (0.5f + 0.5f * t);       // shrink as it dies

        Rectangle src = { 0.0f, 0.0f, (float)_tex->width, (float)_tex->height };
        Rectangle dst = { p.pos.x, p.pos.y, s, s };
        DrawTexturePro(*_tex, src, dst, { s * 0.5f, s * 0.5f }, 0.0f, c);
    }
}

void ParticleSystem::Clear()
{
    for (auto& p : _pool) p.alive = false;
}
