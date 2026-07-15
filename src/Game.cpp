#include <algorithm>
#include <cmath>
#include "Game.hpp"
#include "GameInput.hpp"
#include "GameConfig.hpp"
#include "ResourceKeys.hpp"
#include "ResourceManager.hpp"
#include "Audio.hpp"


// ---------------------------------------------------------------- setup ---

Game::Game() : _player(RK::PLAYER)
{
    const Texture2D &background = RM::get().GetTexture(RK::GAME_BG);
    GameConfig::MAP_W = (float)background.width;
    GameConfig::MAP_H = (float)background.height;

    _minimap.Init(_player, _enemies);
    _collisionMap.Init(RK::GAME_BG_COLLISION);

    _player.SetPosition(GameConfig::MapCenter());
    _player.SetCollisionMap(&_collisionMap);

    _camera.zoom = 1.0f;
    _camera.offset = { GameConfig::HALF_BASE_W, GameConfig::HALF_BASE_H };
    _camera.target = GameConfig::MapCenter();

    _enemies.Init(&_player);
    _particles.Init();

    // Persistent data
    _settings = Persist::LoadSettings();
    _highScores = Persist::LoadHighScores();
    _credits = Persist::LoadCredits();

    Audio::musicVol = _settings.music / 100.0f;
    Audio::sfxVol = _settings.sfx / 100.0f;

    Music& music = RM::get().GetMusic(RK::MUS_DARKLING);
    music.looping = true;
    PlayMusicStream(music);
    Audio::ApplyMusicVolume();

    setScreen(Screen::Title);
}

Game::~Game() {}


// -------------------------------------------------------------- screens ---

void Game::setScreen(Screen s)
{
    _screen = s;
    _menuIndex = 0;

    if (s == Screen::LevelIntro || s == Screen::LevelComplete)
        _bannerTimer = GameConfig::BANNER_TIME;

    if (s == Screen::PlayMenu)
        _saveExists = Persist::SaveExists();

    // Mouse-aim needs a captured cursor; menus need a free one.
    if (s == Screen::Playing) { if (!IsCursorHidden()) DisableCursor(); }
    else                      { if (IsCursorHidden()) EnableCursor(); }
}

void Game::beginRun(const std::string& name, int level, int score, int hp)
{
    _playerName = name;
    _score = score;
    _player.Reset();
    _player.SetHealth(hp);
    startLevel(level);
}

void Game::startLevel(int n)
{
    _level = n;
    _wave = 0;
    _enemies.SetLevel(&GetLevelConfig(n));
    _enemies.DeactivateAll();
    _bullets.DeactivateAll();
    _pickups.DeactivateAll();
    _particles.Clear();
    _player.SetPosition(GameConfig::MapCenter());
    setScreen(Screen::LevelIntro);
}

void Game::startWave(int n)
{
    _wave = n;
    _enemies.SpawnBatch(GameConfig::WAVE_ENEMY_BASE + GameConfig::WAVE_ENEMY_RAMP * n);
    Audio::Play(RK::SND_KNIGHT_GRUNT);  // battle cry
}

void Game::finishRun()
{
    if (Persist::TryInsertScore(_highScores, _playerName, _score))
        Persist::SaveHighScores(_highScores);
}


// ---------------------------------------------------------------- input ---

static bool navUp()   { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
static bool navDown() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }

bool Game::HandleInput()
{
    if (IsKeyPressed(KEY_F1))
        GameConfig::SHOW_DEBUG = !GameConfig::SHOW_DEBUG;

    return handleMenus();
}

bool Game::handleMenus()
{
    switch (_screen)
    {
    case Screen::Title:
        if (navUp())   _menuIndex = (_menuIndex + 1) % 2;
        if (navDown()) _menuIndex = (_menuIndex + 1) % 2;
        if (IsKeyPressed(KEY_ENTER))
            setScreen(_menuIndex == 0 ? Screen::MainMenu : Screen::Credits);
        break;

    case Screen::Credits:
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
            setScreen(Screen::Title);
        break;

    case Screen::MainMenu:
        if (navUp())   _menuIndex = (_menuIndex + 3) % 4;
        if (navDown()) _menuIndex = (_menuIndex + 1) % 4;
        if (IsKeyPressed(KEY_ENTER))
        {
            switch (_menuIndex)
            {
            case 0: setScreen(Screen::PlayMenu); break;
            case 1: setScreen(Screen::Help); break;
            case 2: setScreen(Screen::HighScores); break;
            case 3: return true;  // Quit
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) setScreen(Screen::Title);
        break;

    case Screen::Help:
        if (navUp() || navDown()) _menuIndex = 1 - _menuIndex;
        if (IsKeyPressed(KEY_ENTER))
            setScreen(_menuIndex == 0 ? Screen::Controls : Screen::AudioSettings);
        if (IsKeyPressed(KEY_ESCAPE)) setScreen(Screen::MainMenu);
        break;

    case Screen::Controls:
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
            setScreen(Screen::Help);
        break;

    case Screen::AudioSettings:
    {
        if (navUp() || navDown()) _menuIndex = 1 - _menuIndex;

        int* value = (_menuIndex == 0) ? &_settings.music : &_settings.sfx;
        int step = 0;
        if (IsKeyPressed(KEY_LEFT))  step = -5;
        if (IsKeyPressed(KEY_RIGHT)) step = +5;
        if (step != 0)
        {
            *value = std::clamp(*value + step, 0, 100);
            Audio::musicVol = _settings.music / 100.0f;
            Audio::sfxVol = _settings.sfx / 100.0f;
            Audio::ApplyMusicVolume();
            if (_menuIndex == 1)                     // audible SFX feedback
                Audio::Play(RK::SND_FOOTSTEP_STOP);
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER))
        {
            Persist::SaveSettings(_settings);
            setScreen(Screen::Help);
        }
        break;
    }

    case Screen::HighScores:
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
            setScreen(Screen::MainMenu);
        break;

    case Screen::PlayMenu:
        if (navUp() || navDown()) _menuIndex = 1 - _menuIndex;
        if (IsKeyPressed(KEY_ENTER))
        {
            if (_menuIndex == 0)
            {
                _nameBuffer.clear();
                setScreen(Screen::NameEntry);
            }
            else if (_saveExists)
            {
                Persist::SaveData d;
                if (Persist::LoadGame(d))
                    beginRun(d.name, d.level, d.score, d.hp);
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) setScreen(Screen::MainMenu);
        break;

    case Screen::NameEntry:
    {
        // Drain the typed-character queue (handles key repeat + layout).
        int c;
        while ((c = GetCharPressed()) != 0)
        {
            if (c >= 33 && c <= 126 &&                       // printable, no spaces
                (int)_nameBuffer.size() < GameConfig::NAME_MAX_LEN)
                _nameBuffer.push_back((char)c);
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !_nameBuffer.empty())
            _nameBuffer.pop_back();
        if (IsKeyPressed(KEY_ENTER) && !_nameBuffer.empty())
            beginRun(_nameBuffer, 1, 0, GameConfig::PLAYER_MAX_HEALTH);
        if (IsKeyPressed(KEY_ESCAPE)) setScreen(Screen::PlayMenu);
        break;
    }

    case Screen::LevelIntro:
    case Screen::LevelComplete:
        if (IsKeyPressed(KEY_ENTER)) _bannerTimer = 0.0f;  // skip banner
        break;

    case Screen::Playing:
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
            setScreen(Screen::Paused);
        break;

    case Screen::Paused:
        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE))
            setScreen(Screen::Playing);
        if (IsKeyPressed(KEY_ENTER))    // abandon run (last level-save remains)
            setScreen(Screen::MainMenu);
        break;

    case Screen::GameOver:
    case Screen::Victory:
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE))
            setScreen(Screen::MainMenu);
        break;
    }
    return false;
}


// --------------------------------------------------------------- update ---

void Game::Update(float dt)
{
    // Music streams in small chunks; feed it every frame on every screen.
    UpdateMusicStream(RM::get().GetMusic(RK::MUS_DARKLING));

    switch (_screen)
    {
    case Screen::LevelIntro:
        _bannerTimer -= dt;
        if (_bannerTimer <= 0.0f)
        {
            setScreen(Screen::Playing);
            startWave(1);
        }
        break;

    case Screen::LevelComplete:
        _bannerTimer -= dt;
        if (_bannerTimer <= 0.0f)
            startLevel(_level + 1);
        break;

    case Screen::Playing:
        updateEntities(dt);
        updateShooting();
        updateCollisions();
        updateWaves();
        updateCamera();

        if (_player.IsDead())
        {
            finishRun();
            setScreen(Screen::GameOver);
        }
        break;

    default:
        break;  // menus don't simulate anything
    }
}

void Game::updateWaves()
{
    if (!_enemies.IsBatchComplete()) return;

    const LevelConfig& cfg = GetLevelConfig(_level);

    if (_wave < cfg.waveCount)
    {
        startWave(_wave + 1);
    }
    else if (_level == LEVEL_COUNT)
    {
        finishRun();
        setScreen(Screen::Victory);
    }
    else
    {
        // Auto-save at the level boundary: next level, carried score & HP.
        Persist::SaveGame({ _playerName, _level + 1, _score, _player.GetHealth() });
        setScreen(Screen::LevelComplete);
    }
}

void Game::updateShooting()
{
    if (GI::get().State().shoot)
    {
        _bullets.Spawn(_player.GetFiringPosition(), GI::get().State().aimAngle);
        Audio::Play(RK::SND_SHOOT);
        _particles.Emit(_player.GetFiringPosition(), 5, GOLD,
                        60.0f, 160.0f, 0.05f, 0.15f, 10.0f);
    }
}

void Game::updateEntities(float dt)
{
    GI::get().Update();
    _player.Update(dt);
    _bullets.Update(dt);
    _enemies.Update(dt);
    _pickups.Update(dt);
    _particles.Update(dt);
}

void Game::updateCollisions()
{
    for (auto &bullet : _bullets.GetPool())
    {
        if (!bullet->IsAlive()) continue;
        for (auto &enemy : _enemies.GetPool())
        {
            if (!enemy->IsAlive() || !enemy->CanBeHit()) continue;
            if (bullet->GetCollider().IsCollidingWith(enemy->GetCollider()))
            {
                bullet->Deactivate();
                if (enemy->TakeDamage(1))
                {
                    _score += enemy->GetScoreValue();
                    _particles.Emit(enemy->GetPosition(), 24, MAROON,
                                    30.0f, 180.0f, 0.3f, 0.8f, 10.0f);
                    if (RandomFloat(0.0f, 1.0f) < GameConfig::POTION_DROP_CHANCE)
                        _pickups.Spawn(enemy->GetPosition());
                }
                else
                {
                    _particles.Emit(bullet->GetPosition(), 8, RED,
                                    40.0f, 140.0f, 0.2f, 0.5f, 8.0f);
                }
                break;
            }
        }
    }

    for (auto &enemy : _enemies.GetPool())
    {
        if (!enemy->IsAlive() || !enemy->CanBeHit()) continue;
        if (_player.GetCollider().IsCollidingWith(enemy->GetCollider()))
            _player.Hit();
    }

    for (auto &pickup : _pickups.GetPool())
    {
        if (!pickup->IsAlive()) continue;
        if (_player.GetHealth() < _player.GetMaxHealth() &&
            _player.GetCollider().IsCollidingWith(pickup->GetCollider()))
        {
            _player.Heal(GameConfig::POTION_HEAL);
            pickup->Deactivate();
            Audio::Play(RK::SND_POWERUP);
            _particles.Emit(_player.GetPosition(), 14, GREEN,
                            40.0f, 120.0f, 0.3f, 0.6f, 8.0f);
        }
    }
}

void Game::updateCamera()
{
    _camera.target = _player.GetPosition();
    _camera.target.x = std::clamp(_camera.target.x,
                                  GameConfig::HALF_BASE_W,
                                  GameConfig::MAP_W - GameConfig::HALF_BASE_W);
    _camera.target.y = std::clamp(_camera.target.y,
                                  GameConfig::HALF_BASE_H,
                                  GameConfig::MAP_H - GameConfig::HALF_BASE_H);
}


// ----------------------------------------------------------- draw: game ---

void Game::drawWorld()
{
    BeginMode2D(_camera);
    DrawTexture(RM::get().GetTexture(RK::GAME_BG), 0, 0, WHITE);
    _pickups.Draw();
    _player.Draw();
    _bullets.Draw();
    _enemies.Draw();
    _particles.Draw();
    DrawTexture(RM::get().GetTexture(RK::GAME_FG), 0, 0, WHITE);
    EndMode2D();
}

void Game::drawHUD()
{
    DrawText(TextFormat("SCORE %d", _score), 14, 12, 30, RAYWHITE);

    const LevelConfig& cfg = GetLevelConfig(_level);
    const char* waveText = TextFormat("LEVEL %d   WAVE %d/%d", _level, _wave, cfg.waveCount);
    DrawText(waveText, GameConfig::BASE_W - MeasureText(waveText, 30) - 14, 12, 30, RAYWHITE);

    constexpr int barX = 14, barY = 50, barW = 200, barH = 18;
    float ratio = (float)_player.GetHealth() / (float)_player.GetMaxHealth();
    DrawRectangle(barX, barY, barW, barH, ColorAlpha(BLACK, 0.5f));
    DrawRectangle(barX, barY, (int)(barW * ratio), barH, ratio > 0.4f ? GREEN : RED);
    DrawRectangleLines(barX, barY, barW, barH, RAYWHITE);
    DrawText(_playerName.c_str(), barX, barY + barH + 6, 20, LIGHTGRAY);

    if (!GameConfig::SHOW_DEBUG) return;

    DrawRectangle(0, GameConfig::BASE_H - 32, GameConfig::BASE_W, 32, ColorAlpha(DARKBLUE, 0.6f));
    DrawText(TextFormat("Player: %.0f,%.0f", _player.GetPosition().x, _player.GetPosition().y),
             12, GameConfig::BASE_H - 24, 20, LIME);
    DrawText(TextFormat("Camera: %.0f,%.0f", _camera.target.x, _camera.target.y),
             256, GameConfig::BASE_H - 24, 20, LIME);
    DrawText(TextFormat("Aim: %.1f", GI::get().State().aimAngle),
             512, GameConfig::BASE_H - 24, 20, LIME);
    DrawText(TextFormat("Bullets: %d/%d  Enemies: %d/%d",
                        _bullets.CountAlive(), _bullets.GetPoolTotal(),
                        _enemies.CountAlive(), _enemies.GetPoolTotal()),
             700, GameConfig::BASE_H - 24, 20, LIME);
}


// ---------------------------------------------------------- draw: menus ---

void Game::drawCenteredText(const char* text, int y, int size, Color color)
{
    DrawText(text, (GameConfig::BASE_W - MeasureText(text, size)) / 2, y, size, color);
}

void Game::drawMenuBackdrop(bool dim)
{
    if (RM::get().HasTexture(RK::TITLE_BG))
    {
        const Texture2D& bg = RM::get().GetTexture(RK::TITLE_BG);
        Rectangle src = { 0, 0, (float)bg.width, (float)bg.height };
        Rectangle dst = { 0, 0, (float)GameConfig::BASE_W, (float)GameConfig::BASE_H };
        DrawTexturePro(bg, src, dst, { 0, 0 }, 0.0f, WHITE);
    }
    else
    {
        DrawRectangleGradientV(0, 0, GameConfig::BASE_W, GameConfig::BASE_H,
                               { 20, 20, 30, 255 }, { 5, 5, 10, 255 });
    }

    if (dim)
        DrawRectangle(0, 0, GameConfig::BASE_W, GameConfig::BASE_H, ColorAlpha(BLACK, 0.6f));
}

void Game::drawMenuList(const char* const* items, int count, int startY, int disabledIndex)
{
    for (int i = 0; i < count; i++)
    {
        Color c = (i == disabledIndex) ? DARKGRAY
                : (i == _menuIndex)    ? GOLD
                                       : RAYWHITE;
        const char* text = (i == _menuIndex) ? TextFormat("> %s <", items[i]) : items[i];
        drawCenteredText(text, startY + i * 48, 34, c);
    }
}

void Game::drawTitle()
{
    drawMenuBackdrop(false);
    // Soft dark band behind the text so it reads on any photo
    DrawRectangle(0, GameConfig::BASE_H / 2 - 130, GameConfig::BASE_W, 300,
                  ColorAlpha(BLACK, 0.45f));
    drawCenteredText("SWARM SHOOTER", GameConfig::BASE_H / 2 - 110, 72, RAYWHITE);

    const char* items[] = { "Start", "Credits" };
    drawMenuList(items, 2, GameConfig::BASE_H / 2 + 10);
    drawCenteredText("UP/DOWN select   ENTER confirm", GameConfig::BASE_H - 40, 20, GRAY);
}

void Game::drawCredits()
{
    drawMenuBackdrop(true);
    drawCenteredText("CREDITS", 70, 48, GOLD);
    DrawText(_credits.c_str(), 340, 170, 24, RAYWHITE);   // DrawText handles \n
    drawCenteredText("ESC - back", GameConfig::BASE_H - 40, 20, GRAY);
}

void Game::drawMainMenu()
{
    drawMenuBackdrop(true);
    drawCenteredText("SWARM SHOOTER", 90, 60, RAYWHITE);

    const char* items[] = { "Play", "Help", "High Scores", "Quit" };
    drawMenuList(items, 4, 260);
    drawCenteredText("ESC - back", GameConfig::BASE_H - 40, 20, GRAY);
}

void Game::drawHelp()
{
    drawMenuBackdrop(true);
    drawCenteredText("HELP", 90, 60, RAYWHITE);

    const char* items[] = { "Controls", "Audio" };
    drawMenuList(items, 2, 280);
    drawCenteredText("ESC - back", GameConfig::BASE_H - 40, 20, GRAY);
}

void Game::drawControls()
{
    drawMenuBackdrop(true);
    drawCenteredText("CONTROLS", 80, 48, GOLD);

    const char* lines[] = {
        "W A S D      -  move",
        "Mouse        -  aim",
        "Left Click   -  shoot",
        "P / ESC      -  pause",
        "F1           -  debug overlay",
        "ENTER        -  confirm / skip banner",
    };
    for (int i = 0; i < 6; i++)
        DrawText(lines[i], 420, 190 + i * 46, 28, RAYWHITE);

    drawCenteredText("ESC - back", GameConfig::BASE_H - 40, 20, GRAY);
}

void Game::drawSlider(const char* label, int value, int y, bool selected)
{
    Color c = selected ? GOLD : RAYWHITE;
    DrawText(label, 380, y, 30, c);

    constexpr int barX = 640, barW = 240, barH = 22;
    DrawRectangle(barX, y + 4, barW, barH, ColorAlpha(BLACK, 0.6f));
    DrawRectangle(barX, y + 4, barW * value / 100, barH, selected ? GOLD : GRAY);
    DrawRectangleLines(barX, y + 4, barW, barH, RAYWHITE);
    DrawText(TextFormat("%d%%", value), barX + barW + 16, y, 30, c);
}

void Game::drawAudioSettings()
{
    drawMenuBackdrop(true);
    drawCenteredText("AUDIO", 90, 48, GOLD);

    drawSlider("Music",         _settings.music, 280, _menuIndex == 0);
    drawSlider("Sound Effects", _settings.sfx,   360, _menuIndex == 1);

    drawCenteredText("LEFT/RIGHT adjust    ESC - save & back",
                     GameConfig::BASE_H - 40, 20, GRAY);
}

void Game::drawHighScores()
{
    drawMenuBackdrop(true);
    drawCenteredText("HIGH SCORES", 80, 48, GOLD);

    for (int i = 0; i < (int)_highScores.size(); i++)
    {
        const ScoreEntry& e = _highScores[i];
        DrawText(TextFormat("%d.", i + 1), 420, 200 + i * 54, 32, GRAY);
        DrawText(e.name.c_str(), 480, 200 + i * 54, 32, RAYWHITE);
        const char* sc = TextFormat("%d", e.score);
        DrawText(sc, 860 - MeasureText(sc, 32), 200 + i * 54, 32, GOLD);
    }

    drawCenteredText("ESC - back", GameConfig::BASE_H - 40, 20, GRAY);
}

void Game::drawPlayMenu()
{
    drawMenuBackdrop(true);
    drawCenteredText("PLAY", 90, 60, RAYWHITE);

    const char* items[] = { "New Game", "Load Game" };
    drawMenuList(items, 2, 280, _saveExists ? -1 : 1);

    if (!_saveExists)
        drawCenteredText("(no saved game found)", 385, 20, GRAY);
    drawCenteredText("ESC - back", GameConfig::BASE_H - 40, 20, GRAY);
}

void Game::drawNameEntry()
{
    drawMenuBackdrop(true);
    drawCenteredText("ENTER YOUR NAME", 180, 44, RAYWHITE);

    // Dialogue box with blinking caret
    constexpr int boxW = 420, boxH = 64;
    int boxX = (GameConfig::BASE_W - boxW) / 2;
    int boxY = 300;
    DrawRectangle(boxX, boxY, boxW, boxH, ColorAlpha(BLACK, 0.7f));
    DrawRectangleLines(boxX, boxY, boxW, boxH, GOLD);

    const char* shown = _nameBuffer.empty() ? "" : _nameBuffer.c_str();
    DrawText(shown, boxX + 18, boxY + 14, 36, RAYWHITE);
    if (fmodf((float)GetTime(), 1.0f) < 0.5f)
        DrawText("_", boxX + 18 + MeasureText(shown, 36) + 4, boxY + 14, 36, GOLD);

    drawCenteredText(TextFormat("max %d characters, no spaces", GameConfig::NAME_MAX_LEN),
                     390, 20, GRAY);
    drawCenteredText("ENTER - start    ESC - back", GameConfig::BASE_H - 40, 20, GRAY);
}


// ------------------------------------------------------- draw: overlays ---

void Game::drawBanner(const char* line1, const char* line2)
{
    DrawRectangle(0, 0, GameConfig::BASE_W, GameConfig::BASE_H, ColorAlpha(BLACK, 0.55f));
    drawCenteredText(line1, GameConfig::BASE_H / 2 - 60, 64, RAYWHITE);
    if (line2)
        drawCenteredText(line2, GameConfig::BASE_H / 2 + 20, 28, LIGHTGRAY);
}

void Game::drawPauseOverlay()
{
    DrawRectangle(0, 0, GameConfig::BASE_W, GameConfig::BASE_H, ColorAlpha(BLACK, 0.6f));
    drawCenteredText("PAUSED", GameConfig::BASE_H / 2 - 60, 60, RAYWHITE);
    drawCenteredText("P - resume    ENTER - main menu",
                     GameConfig::BASE_H / 2 + 12, 28, LIGHTGRAY);
}

void Game::drawGameOverOverlay()
{
    DrawRectangle(0, 0, GameConfig::BASE_W, GameConfig::BASE_H, ColorAlpha(BLACK, 0.7f));
    drawCenteredText("GAME OVER", GameConfig::BASE_H / 2 - 90, 60, RED);
    drawCenteredText(TextFormat("%s    Score: %d    Best: %d",
                                _playerName.c_str(), _score, _highScores[0].score),
                     GameConfig::BASE_H / 2 - 10, 28, RAYWHITE);
    drawCenteredText("ENTER - main menu", GameConfig::BASE_H / 2 + 50, 28, LIGHTGRAY);
}

void Game::drawVictoryOverlay()
{
    DrawRectangle(0, 0, GameConfig::BASE_W, GameConfig::BASE_H, ColorAlpha(BLACK, 0.7f));
    drawCenteredText("VICTORY!", GameConfig::BASE_H / 2 - 90, 64, GOLD);
    drawCenteredText(TextFormat("%s cleared all %d levels    Score: %d",
                                _playerName.c_str(), LEVEL_COUNT, _score),
                     GameConfig::BASE_H / 2 - 10, 28, RAYWHITE);
    drawCenteredText("ENTER - main menu", GameConfig::BASE_H / 2 + 50, 28, LIGHTGRAY);
}


// ----------------------------------------------------------------- draw ---

void Game::Draw(RenderTexture2D& canvas)
{
    BeginTextureMode(canvas);
    ClearBackground(BLACK);

    if (isMenuScreen())
    {
        switch (_screen)
        {
        case Screen::Title:         drawTitle(); break;
        case Screen::Credits:       drawCredits(); break;
        case Screen::MainMenu:      drawMainMenu(); break;
        case Screen::Help:          drawHelp(); break;
        case Screen::Controls:      drawControls(); break;
        case Screen::AudioSettings: drawAudioSettings(); break;
        case Screen::HighScores:    drawHighScores(); break;
        case Screen::PlayMenu:      drawPlayMenu(); break;
        case Screen::NameEntry:     drawNameEntry(); break;
        default: break;
        }
    }
    else
    {
        drawWorld();

        switch (_screen)
        {
        case Screen::LevelIntro:
            drawBanner(TextFormat("LEVEL %d", _level),
                       TextFormat("Good luck, %s!", _playerName.c_str()));
            break;
        case Screen::Playing:
            drawHUD();
            _minimap.Draw();
            break;
        case Screen::Paused:
            drawHUD();
            _minimap.Draw();
            drawPauseOverlay();
            break;
        case Screen::LevelComplete:
            drawHUD();
            drawBanner(TextFormat("LEVEL %d COMPLETE", _level), "progress saved");
            break;
        case Screen::GameOver:
            drawGameOverOverlay();
            break;
        case Screen::Victory:
            drawVictoryOverlay();
            break;
        default: break;
        }
    }

    EndTextureMode();
}
