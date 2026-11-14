#include "audio.h"
#include <array>
#include <raylib.h>

Audio audio_init() {
    InitAudioDevice();
    static std::array<short, 44100> beep_samples;
    for (int i = 0; i < 44100; ++i) {
        beep_samples[i] = (i % 100 < 50) ? 3000 : -3000;
    } // 100 samples per cycle

    Wave beep_wave = {
        .frameCount = 44100,
        .sampleRate = 44100,
        .sampleSize = 16,
        .channels = 1,
        .data = beep_samples.data(),
    };

    Audio a;
    a.beep = LoadSoundFromWave(beep_wave);
    return a;
}

void audio_update(Audio &audio, bool sound) {
    if (!IsSoundPlaying(audio.beep) && sound) {
        PlaySound(audio.beep);
    } else if (IsSoundPlaying(audio.beep) && !sound) {
        StopSound(audio.beep);
    }
}

void audio_shutdown() { CloseAudioDevice(); }
