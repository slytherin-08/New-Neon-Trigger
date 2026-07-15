#include "PickupManager.hpp"
#include "raylib.h"


// Same pooling idea as bullets/enemies: reuse a dead slot if one
// exists, otherwise grow the pool.
void PickupManager::Spawn(Vector2 pos)
{
    for (auto& p : _pool)
    {
        if (!p->IsAlive())
        {
            p->Activate(pos);
            return;
        }
    }

    auto p = std::make_unique<Pickup>();
    p->Activate(pos);
    _pool.push_back(std::move(p));

    TraceLog(LOG_INFO, "PICKUP_MGR: Pool growing (size: %d)", (int)_pool.size());
}

void PickupManager::Update(float dt)
{
    for (auto& p : _pool) p->Update(dt);
}

void PickupManager::Draw()
{
    for (auto& p : _pool) p->Draw();
}

void PickupManager::DeactivateAll()
{
    for (auto& p : _pool) p->Deactivate();
}
