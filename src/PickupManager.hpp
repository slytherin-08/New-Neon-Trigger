#pragma once
#include "Pickup.hpp"
#include <vector>
#include <memory>


class PickupManager
{
public:
    void Spawn(Vector2 pos);
    void Update(float dt);
    void Draw();
    void DeactivateAll();

    const std::vector<std::unique_ptr<Pickup>>& GetPool() const { return _pool; }

    int CountAlive() const {
        int n = 0;
        for (const auto& p : _pool) if (p->IsAlive()) n++;
        return n;
    }

private:
    std::vector<std::unique_ptr<Pickup>> _pool;
};
