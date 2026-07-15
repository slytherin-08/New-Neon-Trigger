#include "Pickup.hpp"
#include "ResourceKeys.hpp"
#include "GameConfig.hpp"
#include <cmath>


Pickup::Pickup()
{
    _sprite.Init(RK::HEALTH_POTION);
    _transform.scale = GameConfig::POTION_SCALE;
    _collider.Init(GameConfig::POTION_COLLIDER_RADIUS, _transform);
}

void Pickup::Activate(Vector2 pos)
{
    _alive = true;
    _transform.position = pos;
    _lifeTimer = GameConfig::POTION_LIFETIME;
}

void Pickup::Deactivate()
{
    _alive = false;
    _transform.position = GameConfig::OFFSCREEN_POSITION;
}

void Pickup::Update(float dt)
{
    if (!_alive) return;

    _lifeTimer -= dt;
    if (_lifeTimer <= 0.0f) Deactivate();
}

void Pickup::Draw() const
{
    if (!_alive) return;

    // Blink during the last seconds before despawning so the player
    // knows it's about to disappear.
    Color tint = WHITE;
    if (_lifeTimer < GameConfig::POTION_BLINK_TIME)
        tint = ColorAlpha(WHITE, 0.4f + 0.6f * fabsf(sinf((float)GetTime() * 8.0f)));

    // Gentle vertical bob: purely visual, so we offset a copy of the
    // transform and leave the collider's real position untouched.
    Transform2D visual = _transform;
    visual.position.y += sinf((float)GetTime() * 3.0f) * 4.0f;

    _sprite.Draw(visual, tint);
    _collider.DrawDebug();
}
