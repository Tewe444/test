#include "audio.h"
#include "soundbank.h"
#include <maxmod9.h>

static int currentModule = -1;
static bool audioReady = false;
static bool explosionLoaded = false;
static bool levelCompleteLoaded = false;
static bool levelEnterLoaded = false;
static bool levelExitLoaded = false;

bool audioInit(void) {
    /* NitroFS is initialized by main.c before this call. */
    mmInitDefault("nitro:/soundbank.bin");
    audioReady = true;

    mmSetModuleVolume(880); /* Maxmod volume scale: 1024 is normal. */
    mmSetEffectsVolume(880);

    /* These Maxmod functions return void, so success cannot be checked here. */
    mmLoadEffect(SFX_EXPLOSION);
    explosionLoaded = true;
    mmLoadEffect(SFX_LEVEL_COMPLETE);
    levelCompleteLoaded = true;
    mmLoadEffect(SFX_LEVEL_ENTER);
    levelEnterLoaded = true;
    mmLoadEffect(SFX_LEVEL_EXIT);
    levelExitLoaded = true;
    return true;
}

void audioPlay(int moduleId) {
    if (!audioReady || moduleId < 0) return;
    if (currentModule == moduleId && mmActive()) return;

    if (currentModule >= 0) {
        mmStop();
        mmUnload((mm_word)currentModule);
        currentModule = -1;
    }

    /* mmLoad returns void; load the module, then start playback. */
    mmLoad((mm_word)moduleId);
    mmStart((mm_word)moduleId, MM_PLAY_LOOP);
    currentModule = moduleId;
}

void audioPlaySfx(int sampleId) {
    if (!audioReady) return;
    if (sampleId == SFX_EXPLOSION && explosionLoaded) mmEffect(SFX_EXPLOSION);
    else if (sampleId == SFX_LEVEL_COMPLETE && levelCompleteLoaded) mmEffect(SFX_LEVEL_COMPLETE);
    else if (sampleId == SFX_LEVEL_ENTER && levelEnterLoaded) mmEffect(SFX_LEVEL_ENTER);
    else if (sampleId == SFX_LEVEL_EXIT && levelExitLoaded) mmEffect(SFX_LEVEL_EXIT);
}

void audioPause(void) {
    if (audioReady && currentModule >= 0) mmPause();
}

void audioResume(void) {
    if (audioReady && currentModule >= 0) mmResume();
}

void audioStop(void) {
    if (audioReady && currentModule >= 0) {
        mmStop();
        mmUnload((mm_word)currentModule);
        currentModule = -1;
    }
}
