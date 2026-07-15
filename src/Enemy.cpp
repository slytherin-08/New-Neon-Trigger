#include "Enemy.hpp"
#include "ResourceKeys.hpp"
#include "Audio.hpp"
#include "Player.hpp"
#include "GameConfig.hpp"
#include "raymath.h"


constexpr float HURT_FLASH_TIME = 0.12f;


Enemy::Enemy()
{
    applyType(EnemyType::Cockroach);
    _transform.scale = 1.0f;
    _transform.rotation = 0.0f;
}

// Pulls everything type-specific out of the config table.
// Called on every Activate, so a pooled enemy can be reused as a different type.
void Enemy::applyType(EnemyType type)
{
    _type = type;
    const EnemyTypeConfig& cfg = GetEnemyConfig(type);

    _spriteMove.Init(cfg.moveKey, 64, 64, cfg.moveFrames, cfg.moveFps);
    _spriteMove.rotationOffset = 90.0f;
    _spriteDeath.Init(cfg.deathKey, 64, 64, cfg.deathFrames, cfg.deathFps, false);
    _spriteDeath.rotationOffset = 90.0f;

    _speed = cfg.speed;
    _hp = cfg.hp;
    _collider.Init(cfg.colliderRadius, _transform);
}

bool Enemy::TakeDamage(int dmg)
{
    if (_state == EnemyState::Dying) return false;

    _hp -= dmg;
    _hurtTimer = HURT_FLASH_TIME;
    if (_hp <= 0)
    {
        Kill();
        return true;
    }

    // Survived: pain yelp. Random pitch keeps repeated hits from
    // sounding like the exact same sample every time.
    Audio::Play(RK::SND_ZOMBIE_PAIN, 1.0f, RandomFloat(0.9f, 1.15f));

    return false;
}

// Ambient grunt: volume falls off linearly with distance to the player,
// so far-away enemies are quiet and nearby ones feel threatening.
void Enemy::grunt()
{
    _gruntTimer = RandomFloat(GameConfig::GRUNT_MIN_INTERVAL,
                              GameConfig::GRUNT_MAX_INTERVAL);
    if (!_player) return;

    float dist = Vector2Distance(_transform.position, _player->GetPosition());
    if (dist >= GameConfig::GRUNT_HEARING_DIST) return;

    Audio::Play(GetEnemyConfig(_type).gruntSndKey,
                1.0f - dist / GameConfig::GRUNT_HEARING_DIST,
                RandomFloat(0.9f, 1.1f));
}

void Enemy::Kill()
{
    if(_state == EnemyState::Dying) return;

    _state = EnemyState::Dying;
    _spriteDeath.Reset();
    Audio::Play(GetEnemyConfig(_type).deathSndKey);
    TraceLog(LOG_INFO, "Enemy killed!");
}

void Enemy::Retarget()
{
    _transform.LookAt(_player->GetPosition());
    _retargetTimer = RandomFloat(_retargetMin, _retargetMax);
}

void Enemy::Update(float dt)
{
    if(!_alive) return;

    if (_hurtTimer > 0.0f) _hurtTimer -= dt;

    switch (_state)
    {
    case EnemyState::Moving:
        _retargetTimer -= dt;
        if(_retargetTimer < 0.0f) Retarget();

        _gruntTimer -= dt;
        if(_gruntTimer < 0.0f) grunt();

        _transform.MoveForward(_speed * dt);
        _spriteMove.Update(dt);
        break;
    case EnemyState::Dying:
        _spriteDeath.Update(dt);
        if(_spriteDeath.finished) Deactivate();

    default:
        break;
    }
}

void Enemy::Activate(Vector2 position, EnemyType type,
                     float speedMult, int hpBonus)
{
    applyType(type);
    // Level difficulty scaling on top of the base type stats
    _speed *= speedMult;
    _hp += hpBonus;
    _alive = true;
    _transform.position = position;
    _retargetTimer = 0.0f;
    _hurtTimer = 0.0f;
    // Random initial delay so a fresh wave doesn't grunt in unison.
    _gruntTimer = RandomFloat(0.0f, GameConfig::GRUNT_MAX_INTERVAL);
    _state = EnemyState::Moving;
    _spriteDeath.Reset();
    _spriteMove.Reset();
    TraceLog(LOG_INFO, "ENEMY: Activated (type %d)", (int)type);
}

void Enemy::Deactivate()
{
    _alive = false;
    _transform.position = GameConfig::OFFSCREEN_POSITION;
    TraceLog(LOG_INFO, "ENEMY: Deactivated");
}

void Enemy::Draw()
{
    if(!_alive) return;

    // Brief red tint after a hit gives feedback on multi-HP enemies.
    Color tint = (_hurtTimer > 0.0f) ? RED : WHITE;

    switch (_state)
    {
    case EnemyState::Moving:
        _spriteMove.Draw(_transform, tint);
        break;
    case EnemyState::Dying:
        _spriteDeath.Draw(_transform);
    }
    _collider.DrawDebug();
}

void Enemy::SetPosition(Vector2 position)
{
    _transform.position = position;
}

void Enemy::SetPlayer(const Player* player)
{
    _player = player;
}
