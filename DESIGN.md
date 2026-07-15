# Swarm Shooter — Feature Design Document

Planning doc for the menu system, persistence, and level structure.
Written before implementation (design-first), so decisions are recorded with reasons.

---

## 1. Screen Flow (state machine)

The existing `GameState` enum grows into an app-wide screen state machine.
Exactly one screen is active at a time; ESC always means "go back" inside menus.

```
Title ──Start──▶ MainMenu ──Play──▶ PlayMenu ──New Game──▶ NameEntry ──ENTER──▶ LevelIntro ─▶ Playing
  │                │  │  │             └──Load Game(if save exists)───────────▶ LevelIntro ─▶ Playing
  └─Credits        │  │  └─Quit
                   │  └─HighScores (top 5)
                   └─Help ──▶ Controls
                          └─▶ Audio (Music 0–100%, SFX 0–100%)

Playing ⇄ Paused (P)
Playing ─▶ GameOver ─▶ (qualifies? enter high score) ─▶ MainMenu
Level 5 complete ─▶ Victory ─▶ (high score) ─▶ MainMenu
```

New enum (replaces current `GameState`):

```cpp
enum class Screen {
    Title, Credits, MainMenu, Help, Controls, AudioSettings,
    HighScores, PlayMenu, NameEntry,
    LevelIntro, Playing, Paused, LevelComplete, GameOver, Victory
};
```

Navigation: UP/DOWN select, ENTER confirm, ESC back.
(Keyboard-driven, because gameplay hides the OS cursor for mouse-aim;
mixing a visible menu cursor with relative mouse aim causes bugs.)
ESC's old job (cursor toggle) is dropped; the cursor is enabled
automatically on menu screens and disabled during Playing.

Title screen: background photo (asset to be provided) + Start / Credits.

---

## 2. Persistence (plain-text files, no libraries)

All files live next to the executable (same pattern as `assets/`).
Plain text so the format can be explained line-by-line.

| File            | Format (one item per line)          | When written                  |
|-----------------|--------------------------------------|-------------------------------|
| `settings.txt`  | `musicVol` `sfxVol` (ints 0–100)     | On leaving Audio screen       |
| `highscores.txt`| 5 × (`name` `score`)                 | When a run ends with a top-5 score |
| `savegame.txt`  | `name` `level` `score` `hp`          | Auto-save after each completed level |

Rules:
- High scores: fixed 5 slots, sorted descending. Empty slot = name `-----`, score `0`.
- Load Game menu item is greyed out / hidden when `savegame.txt` doesn't exist.
- Finishing the game (Victory) or dying deletes the save? **Decision: keep save
  until overwritten by a new game** — simpler, and lets the player retry a level.
- Save at *level boundaries only*. Mid-wave saving would require serializing every
  enemy/bullet/pickup — high effort, no gameplay value.

I/O: raylib `SaveFileText` / `LoadFileText` (thin wrappers over fopen), or
`std::ifstream/ofstream`. **Decision: std streams** — standard C++, teacher-friendly.

---

## 3. Audio Volume Control

Problem: `PlaySound()` is called raw at ~12 sites, so no single point applies a
user volume. Also `SetSoundVolume` is *sticky* per Sound object (the grunt
distance code already exploits this), so a global SFX volume must multiply
with per-play volumes, not fight them.

Plan: small `AudioPlayer` helper (namespace or static class):

```cpp
namespace Audio {
    inline float musicVol = 1.0f;  // 0..1, from settings
    inline float sfxVol   = 1.0f;

    void Play(const char* key, float vol = 1.0f, float pitch = 1.0f);
    // -> SetSoundVolume(snd, vol * sfxVol); SetSoundPitch(snd, pitch); PlaySound(snd);
    void ApplyMusicVolume();  // SetMusicVolume(music, 0.4f * musicVol)
}
```

Every `PlaySound(RM::get().GetSound(X))` call becomes `Audio::Play(X)`.
Grunt distance falloff passes its factor as `vol`. Sliders change
`musicVol`/`sfxVol`, take effect immediately, persist via `settings.txt`.

Slider UI: LEFT/RIGHT adjusts selected slider in 5% steps; drawn as a bar
(`DrawRectangle` fill proportional to value) with the percentage text.

---

## 4. Name Entry Screen

- raylib `GetCharPressed()` loop (drains the char queue each frame; handles
  key repeat and layout correctly), accept printable chars, max 12.
- BACKSPACE deletes, ENTER confirms (rejected if empty), ESC back to PlayMenu.
- Drawn as a dialogue box with a blinking caret (`fmodf(GetTime(),1) < 0.5`).
- Name is stored in the run state and used for high scores + save file.

---

## 5. Levels

**Structure: 5 finite levels → Victory screen.** Finite because it gives a
demo-able ending and clean save points.

Data-driven, same pattern as `EnemyTypeConfig`:

```cpp
struct LevelConfig {
    int   waveCount;        // waves to clear this level
    float speedMult;        // enemy speed multiplier
    int   hpBonus;          // added to each enemy's base HP
    bool  allowed[ENEMY_TYPE_COUNT]; // spawn mix for this level
};
```

| Level | Waves | Speed | HP+ | Enemy types                     |
|-------|-------|-------|-----|---------------------------------|
| 1     | 4     | 1.0   | 0   | cockroach                       |
| 2     | 4     | 1.05  | 0   | cockroach, scorpion             |
| 3     | 5     | 1.10  | 0   | cockroach, scorpion, kliver     |
| 4     | 5     | 1.15  | 1   | all four                        |
| 5     | 6     | 1.25  | 1   | all four                        |

Consequences:
- Per-type `minWave` in `EnemyTypeConfig` is **removed**; spawn mix now comes
  from `LevelConfig.allowed`. (`EnemyManager::pickType` reads the level table.)
- Wave enemy-count formula stays (`BASE + RAMP × wave`), wave resets to 1 each level.
- Flow: LevelIntro ("LEVEL N" banner, ~2 s) → waves → LevelComplete banner →
  auto-save → next LevelIntro. After level 5: Victory.
- Score persists across levels; HP carries over (potions remain the heal source).

---

## 6. Implementation Order (each step compiles & is testable)

1. **AudioPlayer helper** — refactor existing PlaySound sites. No visible change.
2. **Screen enum + Title/Credits/MainMenu/PlayMenu skeleton** — navigation only,
   Play jumps straight into the current game. Menus testable immediately.
3. **Help / Controls / Audio screens** — sliders wired to AudioPlayer;
   `settings.txt` load on boot, save on exit from Audio screen.
4. **Name entry screen** — feeds name into run state.
5. **Levels** — LevelConfig table, LevelIntro/LevelComplete/Victory, minWave removal.
6. **Save/Load** — write on LevelComplete, read on Load Game.
7. **High scores** — table + file + GameOver/Victory insertion + display screen.
8. Title background image when the photo is ready (one `DrawTexture`).

Rationale: audio refactor first because later screens depend on it; levels
before save/load because the save format stores the level number; high scores
last because they trigger at run end, which levels/victory define.

---

## 7. Open Questions (to decide before coding)

1. Does dying erase the save file, or can you retry from the saved level?
   (Proposed: save survives — retry allowed.)
2. High score = score at death/victory only, or also mid-run? (Proposed: run end only.)
3. Credits content — team member names/roles? (Text needed from team.)
4. Title background photo — dimensions ideally 1280×720 to match BASE_W/H.
