#pragma once

#include <unordered_map>
#include <string>

#include "raylib.h"

class ResourceManager
{
public:

    static ResourceManager& instance()
    {
        static ResourceManager inst;
        return inst;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    void Load();
    void Unload();

    const Texture2D& GetTexture(const std::string& name) const;
    bool HasTexture(const std::string& name) const { return _textures.count(name) > 0; }
    const Image& GetImage(const std::string& name) const;
    const Sound& GetSound(const std::string& name) const;
    Music& GetMusic(const std::string& name);  // non-const: UpdateMusicStream mutates it


private:
    ResourceManager() = default;

    void loadTexture(const std::string& name, const std::string& path);
    void loadImage(const std::string& name, const std::string& path);
    void loadSound(const std::string& name, const std::string& path);
    void loadMusic(const std::string& name, const std::string& path);

    std::unordered_map<std::string, Texture2D> _textures;
    std::unordered_map<std::string, Image> _images;
    std::unordered_map<std::string, Sound> _sounds;
    std::unordered_map<std::string, Music> _music;

};

// RM::get()
namespace RM
{
    inline ResourceManager& get() { return ResourceManager::instance(); }
}
