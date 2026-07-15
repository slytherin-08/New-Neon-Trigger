#include "Player.hpp"
#include "GameConfig.hpp"
#include "GameInput.hpp"
#include "ResourceKeys.hpp"
#include "ResourceManager.hpp"
#include "Audio.hpp"
#include "raymath.h"

Player::Player(const std::string& textureName)
{
    _sprite.Init(textureName);
    _sprite.pivot = GameConfig::PLAYER_PIVOT;
    _transform.scale = GameConfig::PLAYER_SCALE;
    _movement.speed = GameConfig::PLAYER_SPEED;
    _muzzleOffset = GameConfig::PLAYER_MUZZLE_OFFSET;
    _collider.Init(GameConfig::PLAYER_COLLIDER_RADIUS, _transform);
    _maxHealth = GameConfig::PLAYER_MAX_HEALTH;
    _health = GameConfig::PLAYER_MAX_HEALTH;
    _invTime = GameConfig::PLAYER_INV_TIME;
}

void Player::Reset()
{
    _health = _maxHealth;
    _invTimer = 0.0f;
}

void Player::Update(float delta)
{
    _movement.Update(_transform, GI::get().State(), delta, _collisionMap);

    if(_invTimer > 0.0f) _invTimer -= delta;

    // Footsteps: keep the walk loop playing while moving; on stop,
    // cut it and play a short scuff.
    Vector2 dir = GI::get().State().moveDir;
    bool moving = (dir.x != 0.0f || dir.y != 0.0f);
    const Sound& steps = RM::get().GetSound(RK::SND_FOOTSTEPS);

    if (moving && !IsSoundPlaying(steps))
    {
        SetSoundVolume(steps, Audio::sfxVol);
        PlaySound(steps);
    }
    else if (!moving && IsSoundPlaying(steps))
    {
        StopSound(steps);
        Audio::Play(RK::SND_FOOTSTEP_STOP);
    }
}

void Player::SetPosition(Vector2 position)
{
    _transform.position = position;
}

Vector2 Player::GetPosition() const
{
    return _transform.position;
}

void Player::Hit()
{
    if( _invTimer > 0.0f || _health <= 0 ) return;
    _health--;
    _invTimer = _invTime;

    if (_health <= 0)
        Audio::Play(RK::SND_PLAYER_DIE);
    else
        Audio::Play(GetRandomValue(0, 1) ? RK::SND_PLAYER_HURT1 : RK::SND_PLAYER_HURT2);

    TraceLog(LOG_INFO, "Player hit! health:%d/%d", _health, _maxHealth);
}

void Player::SetHealth(int hp)
{
    if (hp < 1) hp = 1;
    if (hp > _maxHealth) hp = _maxHealth;
    _health = hp;
}

void Player::Heal(int amount)
{
    _health += amount;
    if (_health > _maxHealth) _health = _maxHealth;
    TraceLog(LOG_INFO, "Player healed! health:%d/%d", _health, _maxHealth);
}

Vector2 Player::GetFiringPosition() const
{
    float rad = _transform.rotation * DEG2RAD;
    Vector2 rotated = Vector2Rotate(_muzzleOffset, rad);
    return Vector2Add(_transform.position, rotated);
}

void Player::SetCollisionMap(const CollisionMap *collisionMap)
{
    _collisionMap = collisionMap;
}

void Player::Draw() const
{
    _sprite.Draw(
        _transform, 
        IsInvincible() 
        ? ColorAlpha(GREEN, fabsf(sinf(GetTime() * 10.0f))) 
        : WHITE
    );
    _collider.DrawDebug();
}