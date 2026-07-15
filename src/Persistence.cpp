#include "Persistence.hpp"
#include "raylib.h"
#include <fstream>
#include <sstream>


// Files live next to the executable (like the assets folder does).
static std::string filePath(const char* name)
{
    return std::string(GetApplicationDirectory()) + "/" + name;
}

static int clampPct(int v)
{
    if (v < 0) return 0;
    if (v > 100) return 100;
    return v;
}


Settings Persist::LoadSettings()
{
    Settings s;
    std::ifstream in(filePath("settings.txt"));
    if (in >> s.music >> s.sfx)
    {
        s.music = clampPct(s.music);
        s.sfx = clampPct(s.sfx);
    }
    return s;  // defaults (100/100) if the file is missing or malformed
}

void Persist::SaveSettings(const Settings& s)
{
    std::ofstream out(filePath("settings.txt"));
    out << s.music << " " << s.sfx << "\n";
}


HighScoreTable Persist::LoadHighScores()
{
    HighScoreTable t;  // default-constructed: "-----" / 0
    std::ifstream in(filePath("highscores.txt"));
    for (auto& e : t)
    {
        std::string name; int score;
        if (in >> name >> score)
        {
            e.name = name;
            e.score = score;
        }
    }
    return t;
}

void Persist::SaveHighScores(const HighScoreTable& t)
{
    std::ofstream out(filePath("highscores.txt"));
    for (const auto& e : t)
        out << e.name << " " << e.score << "\n";
}

bool Persist::TryInsertScore(HighScoreTable& t, const std::string& name, int score)
{
    // Find the first slot this score beats, shift the rest down one.
    for (size_t i = 0; i < t.size(); i++)
    {
        if (score > t[i].score)
        {
            for (size_t j = t.size() - 1; j > i; j--)
                t[j] = t[j - 1];
            t[i] = { name, score };
            return true;
        }
    }
    return false;
}


bool Persist::SaveExists()
{
    return std::ifstream(filePath("savegame.txt")).good();
}

bool Persist::LoadGame(SaveData& out)
{
    std::ifstream in(filePath("savegame.txt"));
    SaveData d;
    if (in >> d.name >> d.level >> d.score >> d.hp)
    {
        out = d;
        return true;
    }
    return false;
}

void Persist::SaveGame(const SaveData& d)
{
    std::ofstream out(filePath("savegame.txt"));
    out << d.name << " " << d.level << " " << d.score << " " << d.hp << "\n";
}


std::string Persist::LoadCredits()
{
    // Credits.txt sits in the project root, one level above the exe (build/).
    std::ifstream in(filePath("../Credits.txt"));
    if (!in.good())
        return "SWARM SHOOTER\n\n(Credits.txt not found)";

    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
