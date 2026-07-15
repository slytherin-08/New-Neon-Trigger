#pragma once
#include <array>
#include <string>


struct Settings
{
    int music = 100;   // 0..100
    int sfx = 100;
};

struct ScoreEntry
{
    std::string name = "-----";
    int score = 0;
};

using HighScoreTable = std::array<ScoreEntry, 5>;


// All persistence is plain text files next to the executable,
// written/read with standard C++ streams (no black boxes).
namespace Persist
{
    Settings LoadSettings();                    // defaults if file missing
    void SaveSettings(const Settings& s);

    HighScoreTable LoadHighScores();            // empty slots if file missing
    void SaveHighScores(const HighScoreTable& t);
    // Inserts if it beats an existing entry; keeps table sorted descending.
    bool TryInsertScore(HighScoreTable& t, const std::string& name, int score);

    struct SaveData
    {
        std::string name = "PLAYER";
        int level = 1;
        int score = 0;
        int hp = 5;
    };
    bool SaveExists();
    bool LoadGame(SaveData& out);               // false if no/invalid file
    void SaveGame(const SaveData& d);

    std::string LoadCredits();                  // Credits.txt from project root
}
