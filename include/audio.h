#ifndef SKYDASH_AUDIO_H
#define SKYDASH_AUDIO_H

#include <stdbool.h>

bool audioInit(void);
void audioPlay(int moduleId);
void audioPlaySfx(int sampleId);
void audioPause(void);
void audioResume(void);
void audioStop(void);

#endif
