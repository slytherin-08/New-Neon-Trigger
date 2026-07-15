#pragma once


// Central place all sound effects go through, so the user's volume
// settings can act on every sound from one spot.
namespace Audio
{
    // 0..1 master volumes, driven by the Audio settings screen
    inline float musicVol = 1.0f;
    inline float sfxVol = 1.0f;

    // vol/pitch are per-play values (e.g. grunt distance falloff);
    // the user's sfxVol is multiplied on top.
    void Play(const char* key, float vol = 1.0f, float pitch = 1.0f);
    void ApplyMusicVolume();
}
