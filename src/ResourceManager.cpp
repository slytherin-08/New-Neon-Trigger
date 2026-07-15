#include "ResourceManager.hpp"
#include "ResourceKeys.hpp"
#include <stdexcept>

void ResourceManager::Unload()
{
    for (auto& [name, tex] : _textures)
        UnloadTexture(tex);
    _textures.clear();
    
    for (auto& [name, img] : _images)
        UnloadImage(img);
    _images.clear();

    for (auto& [name, snd] : _sounds)
        UnloadSound(snd);
    _sounds.clear();

    for (auto& [name, mus] : _music)
        UnloadMusicStream(mus);
    _music.clear();

    TraceLog(LOG_INFO, "ResourceManager: all resources unloaded");
}

void ResourceManager::Load()
{
    ChangeDirectory(TextFormat("%s/../assets/images", GetApplicationDirectory()));

    loadTexture(RK::PLAYER, "survivor-idle_shotgun_0.png");
    loadTexture(RK::GAME_BG, "Floor.png");
    loadTexture(RK::GAME_FG, "Walls.png");
    loadTexture(RK::BULLET, "bullet.png");
    loadTexture(RK::COCKROACH_MOVE, "cockroach-move.png");
    loadTexture(RK::COCKROACH_DEATH, "cockroach-death.png");
    loadTexture(RK::SCORPION_MOVE, "scorpion-move.png");
    loadTexture(RK::SCORPION_DEATH, "scorpion-death-0.png");
    loadTexture(RK::KLIVER_MOVE, "kliver-move.png");
    loadTexture(RK::KLIVER_DEATH, "kliver-death.png");
    loadTexture(RK::TROLL_MOVE, "troll-move.png");
    loadTexture(RK::TROLL_DEATH, "troll-death.png");
    loadTexture(RK::HEALTH_POTION, "healthPotion.png");
    loadTexture(RK::PARTICLE_DOT, "ParticleDot.png");

    // Title background lives in the project root; optional so a missing
    // photo doesn't crash the game (a plain backdrop is drawn instead).
    if (FileExists("../../temp_bg.png"))
        loadTexture(RK::TITLE_BG, "../../temp_bg.png");

    loadImage(RK::GAME_BG_COLLISION, "gameBgCollision.png");

    ChangeDirectory(TextFormat("%s/../assets/audio", GetApplicationDirectory()));

    loadSound(RK::SND_SHOOT, "212606__nail.wav");
    loadSound(RK::SND_ZOMBIE_DEATH, "316260__Zombie_Death.wav");
    loadSound(RK::SND_MUTANT_DEATH, "76961_Mutant_Death.wav");
    loadSound(RK::SND_PLAYER_HURT1, "515624__PlayerHurt1.wav");
    loadSound(RK::SND_PLAYER_HURT2, "515623__PlayerHurt2.wav");
    loadSound(RK::SND_PLAYER_DIE, "469567__PlayerDie.wav");
    loadSound(RK::SND_POWERUP, "523655__powerup.wav");
    loadSound(RK::SND_FOOTSTEPS, "concrete-footsteps-6752.wav");
    loadSound(RK::SND_FOOTSTEP_STOP, "concrete-footsteps-stop.wav");
    loadSound(RK::SND_ZOMBIE_PAIN, "316264__Zombie_Pain.wav");
    loadSound(RK::SND_ZOMBIE_GRUNT, "316258__Zombie_Grunt.wav");
    loadSound(RK::SND_MUTANT_GRUNT, "76973__Mutant_Grunt.wav");
    loadSound(RK::SND_GRUNT_GRUNT, "76957__Grunt_Grunt.wav");
    loadSound(RK::SND_KNIGHT_GRUNT, "484298__Knight_Grunt.wav");

    loadMusic(RK::MUS_DARKLING, "Darkling.mp3");

    // Restore the working directory so save files land next to the exe.
    ChangeDirectory(GetApplicationDirectory());

    TraceLog(LOG_INFO, "ResourceManager: loaded %d textures", (int)_textures.size());
    TraceLog(LOG_INFO, "ResourceManager: loaded %d images", (int)_images.size());
    TraceLog(LOG_INFO, "ResourceManager: loaded %d sounds", (int)_sounds.size());
}

void ResourceManager::loadTexture(const std::string& name, const std::string& path)
{
    Texture2D tex = LoadTexture(path.c_str());
    if (tex.id == 0)
        throw std::runtime_error("Failed to load texture: " + path);
    _textures.emplace(name, std::move(tex));
}

void ResourceManager::loadImage(const std::string& name, const std::string& path)
{
    Image img = LoadImage(path.c_str());
    if (img.data == nullptr)
        throw std::runtime_error("Failed to load image: " + path);
    _images.emplace(name, std::move(img));
}

void ResourceManager::loadSound(const std::string& name, const std::string& path)
{
    Sound snd = LoadSound(path.c_str());
    if (snd.frameCount == 0)
        throw std::runtime_error("Failed to load sound: " + path);
    _sounds.emplace(name, snd);
}

void ResourceManager::loadMusic(const std::string& name, const std::string& path)
{
    Music mus = LoadMusicStream(path.c_str());
    if (mus.frameCount == 0)
        throw std::runtime_error("Failed to load music: " + path);
    _music.emplace(name, mus);
}

const Sound& ResourceManager::GetSound(const std::string& name) const
{
    auto it = _sounds.find(name);
    if (it == _sounds.end())
        throw std::runtime_error("Sound not found: '" + name + "'");
    return it->second;
}

Music& ResourceManager::GetMusic(const std::string& name)
{
    auto it = _music.find(name);
    if (it == _music.end())
        throw std::runtime_error("Music not found: '" + name + "'");
    return it->second;
}

const Texture2D& ResourceManager::GetTexture(const std::string& name) const
{
    auto it = _textures.find(name);
    if (it == _textures.end())
        throw std::runtime_error("Texture not found: '" + name + "'");
    return it->second;
}

const Image& ResourceManager::GetImage(const std::string& name) const
{
    auto it = _images.find(name);
    if (it == _images.end())
        throw std::runtime_error("Image not found: '" + name + "'");
    return it->second;
}