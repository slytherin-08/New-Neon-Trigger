#pragma once
#include "Transform2D.hpp"
#include "Sprite.hpp"
#include "CircleCollider.hpp"


class Pickup
{
public:
    Pickup();
    void Activate(Vector2 pos);
    void Deactivate();
    void Update(float dt);
    void Draw() const;
    bool IsAlive() const { return _alive; }
    Vector2 GetPosition() const { return _transform.position; }
    const CircleCollider& GetCollider() const { return _collider; }

private:
    Transform2D _transform;
    Sprite _sprite;
    CircleCollider _collider;
    float _lifeTimer = 0.0f;  // counts down; despawns at 0
    bool _alive = false;
};
