#pragma once
#include <raylib.h>

struct Audio {
        Sound beep = {};
};

Audio audio_init();
void audio_update(Audio &, bool sound);
void audio_shutdown();
