#pragma once
#include "raylib.h"
#include "BulletManager.hpp"
#include "EnemyManager.hpp"
#include "PickupManager.hpp"
#include "Particles.hpp"
#include "Player.hpp"
#include "CollisionMap.hpp"
#include "Minimap.hpp"
#include "Persistence.hpp"
#include "LevelConfig.hpp"
#include <string>


// App-wide screen state machine (see DESIGN.md section 1).
// Exactly one screen is active; ESC means "back" inside menus.
enum class Screen
{
    // menu screens
    Title, Credits, MainMenu, Help, Controls, AudioSettings,
    HighScores, PlayMenu, NameEntry,
    // gameplay screens
    LevelIntro, Playing, Paused, LevelComplete, GameOver, Victory
};


class Game
{
public:
    Game();
    ~Game();

    bool HandleInput();   // returns true when the app should quit
    void Update(float dt);
    void Draw(RenderTexture2D& canvas);

private:
    // --- gameplay ---
    void drawWorld();
    void drawHUD();
    void updateEntities(float dt);
    void updateCamera();
    void updateShooting();
    void updateCollisions();
    void updateWaves();
    void startWave(int n);
    void startLevel(int n);
    void beginRun(const std::string& name, int level, int score, int hp);
    void finishRun();            // high-score submission at run end

    // --- screens ---
    void setScreen(Screen s);
    bool isMenuScreen() const { return _screen < Screen::LevelIntro; }
    bool handleMenus();          // returns true = quit app
    void drawMenuBackdrop(bool dim);
    void drawMenuList(const char* const* items, int count, int startY,
                      int disabledIndex = -1);
    void drawCenteredText(const char* text, int y, int size, Color color);
    void drawSlider(const char* label, int value, int y, bool selected);
    void drawTitle();
    void drawCredits();
    void drawMainMenu();
    void drawHelp();
    void drawControls();
    void drawAudioSettings();
    void drawHighScores();
    void drawPlayMenu();
    void drawNameEntry();
    void drawBanner(const char* line1, const char* line2);
    void drawPauseOverlay();
    void drawGameOverOverlay();
    void drawVictoryOverlay();

    // --- gameplay state ---
    Player _player;
    CollisionMap _collisionMap;
    Camera2D _camera = {};
    BulletManager _bullets;
    EnemyManager _enemies;
    PickupManager _pickups;
    ParticleSystem _particles;
    Minimap _minimap;
    int _level = 1;
    int _wave = 1;
    int _score = 0;

    // --- app state ---
    Screen _screen = Screen::Title;
    int _menuIndex = 0;          // selected item on the current menu
    std::string _playerName;
    std::string _nameBuffer;     // NameEntry typing buffer
    std::string _credits;        // Credits.txt content
    float _bannerTimer = 0.0f;   // LevelIntro / LevelComplete countdown
    bool _saveExists = false;    // cached when entering PlayMenu
    Settings _settings;
    HighScoreTable _highScores;
};
