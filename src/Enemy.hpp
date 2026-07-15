#pragma once
#include "Transform2D.hpp"
#include "Sprite.hpp"
#include "CircleCollider.hpp"
#include "EnemyType.hpp"


class Player;


enum class EnemyState
{
    Moving,
    Dying
};


class Enemy
{
public:
    Enemy();
    void Update(float dt);
    void Draw();
    void SetPosition(Vector2 position);
    void SetPlayer(const Player *player);
    Vector2 GetPosition() const { return _transform.position; }
    bool IsAlive() const { return _alive; }
    bool TakeDamage(int dmg);  // returns true if this hit killed the enemy
    void Kill();
    bool CanBeHit() const { return _state == EnemyState::Moving; }
    void Deactivate();
    void Activate(Vector2 pos, EnemyType type,
                  float speedMult = 1.0f, int hpBonus = 0);
    EnemyType GetType() const { return _type; }
    int GetScoreValue() const { return GetEnemyConfig(_type).score; }
    const CircleCollider& GetCollider() const { return _collider; }

private:
    Transform2D _transform;
    Sprite _spriteMove;
    Sprite _spriteDeath;
    CircleCollider _collider;
    const Player* _player = nullptr;

    EnemyType _type = EnemyType::Cockroach;
    EnemyState _state = EnemyState::Moving;

    int _hp = 1;
    float _hurtTimer = 0.0f;   // red flash countdown after taking a hit
    float _gruntTimer = 0.0f;  // countdown until next ambient grunt
    float _speed = 80.0f;
    float _retargetTimer = 0.0f;
    float _retargetMin = 1.0f;
    float _retargetMax = 2.0f;

    bool _alive = true;

    void applyType(EnemyType type);
    void Retarget();
    void grunt();
};
