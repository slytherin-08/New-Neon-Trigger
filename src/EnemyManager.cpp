#include "EnemyManager.hpp"
#include "raylib.h"
#include "raymath.h"
#include "GameConfig.hpp"
#include "SwarmUtils.hpp"
#include "Player.hpp"


void EnemyManager::Init(Player *player)
{
    _player = player;
}

Vector2 EnemyManager::pickSpawnPoint() const
{
    constexpr float MIN_DIST = 200.0f;
    constexpr float MIN_DIST_SQ = MIN_DIST * MIN_DIST;

    Vector2 playerPos = _player->GetPosition();
    Vector2 canditate;

    do {
        canditate.x = RandomFloat(0.0f, GameConfig::MAP_W);
        canditate.y = RandomFloat(0.0f, GameConfig::MAP_H);
    } while(Vector2DistanceSqr(canditate, playerPos) < MIN_DIST_SQ);


    return canditate;
}

void EnemyManager::SpawnBatch(int count)
{
    _batchRemaining = count;
    _staggerTimer = 0.0f;
    TraceLog(LOG_INFO, "ENEMY_MGR: Batch of %d enemies queued", count);
}

// Uniform random pick among the types this level allows.
EnemyType EnemyManager::pickType() const
{
    EnemyType allowed[ENEMY_TYPE_COUNT];
    int n = 0;
    for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
    {
        if (!_levelCfg || _levelCfg->allowed[i])
            allowed[n++] = (EnemyType)i;
    }
    return allowed[GetRandomValue(0, n - 1)];
}

void EnemyManager::Spawn(Vector2 pos, EnemyType type)
{
    float speedMult = _levelCfg ? _levelCfg->speedMult : 1.0f;
    int hpBonus = _levelCfg ? _levelCfg->hpBonus : 0;

    for (auto& enemy : _pool)
    {
        if (!enemy->IsAlive())
        {
            enemy->Activate(pos, type, speedMult, hpBonus);
            return;
        }
    }

    auto enemy = std::make_unique<Enemy>();
    enemy->SetPlayer(_player);
    enemy->Activate(pos, type, speedMult, hpBonus);
    _pool.push_back(std::move(enemy));

    TraceLog(LOG_INFO, "ENEMY_MGR: Pool growing (size: %d)", (int)_pool.size());
}

void EnemyManager::Update(float dt)
{
    if(_batchRemaining > 0)
    {
        //Spawn(pickSpawnPoint());
        //_batchRemaining--;
        _staggerTimer -= dt;
        if(_staggerTimer < 0.0f)
        {
            _staggerTimer = _staggerInterval;
            _batchRemaining--;
            Spawn(pickSpawnPoint(), pickType());
        }
    }
    for (auto& enemy : _pool) enemy->Update(dt);
}

void EnemyManager::Draw()
{
    for (auto& enemy : _pool) enemy->Draw();
}

void EnemyManager::DeactivateAll()
{
    for (auto& enemy : _pool) enemy->Deactivate();
}
