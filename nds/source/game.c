#include "game.h"
#include "audio.h"
#include "soundbank.h"
#include <string.h>

#define PLAYER_SCREEN_X 58.0f
#define PLAYER_SIZE 16.0f
#define FLOOR_Y 151.0f
#define GRAVITY 0.36f
#define JUMP_FORCE -7.15f

static void resetPlayer(Game *g) {
    g->distance = 0.0f;
    g->playerY = FLOOR_Y - PLAYER_SIZE;
    g->playerVelocity = 0.0f;
    g->cubeAngle = 0.0f;
    g->grounded = true;
    g->jumpQueued = false;
}

static void returnToSelect(Game *g) {
    audioPlay(MOD_MENU);
    audioPlaySfx(SFX_LEVEL_EXIT);
    resetPlayer(g);
    g->scene = SCENE_LEVEL_SELECT;
}

void gameInit(Game *g) {
    memset(g, 0, sizeof(*g));
    g->scene = SCENE_TITLE;
    resetPlayer(g);
    audioPlay(MOD_MENU);
}

void gameStartLevel(Game *g, unsigned level) {
    if (level >= SKY_LEVEL_COUNT) level = 0;
    g->selectedLevel = level;
    resetPlayer(g);
    g->scene = SCENE_PLAYING;
    audioPlay(gLevels[level].musicId);
    audioPlaySfx(SFX_LEVEL_ENTER);
}

float gameProgress(const Game *g) {
    if (g->selectedLevel >= SKY_LEVEL_COUNT) return 0.0f;
    const float length = gLevels[g->selectedLevel].length;
    if (length <= 0.0f) return 0.0f;
    float p = g->distance / length;
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;
    return p;
}

static bool hitObject(const Game *g, const LevelObject *o) {
    const float sx = (float)o->x - g->distance;
    const float px0 = PLAYER_SCREEN_X + 2.0f;
    const float px1 = PLAYER_SCREEN_X + PLAYER_SIZE - 2.0f;

    if (o->type == OBJ_STOP_BLOCK) {
        /* S-Block placeholder: it is non-lethal until a dash-orb mechanic exists. */
        return false;
    }

    if (o->type == OBJ_BLOCK) {
        const float left = sx;
        const float right = sx + 22.0f;
        const float top = FLOOR_Y - 22.0f;
        return px1 > left && px0 < right &&
               g->playerY + PLAYER_SIZE > top && g->playerY < FLOOR_Y;
    }

    const float width = (o->type == OBJ_DOUBLE_SPIKE) ? 57.0f : 20.0f;
    const float spikeLeft = sx + 4.0f;
    const float spikeRight = sx + width - 4.0f;
    const float spikeTop = FLOOR_Y - 20.0f;
    return px1 > spikeLeft && px0 < spikeRight &&
           g->playerY + PLAYER_SIZE > spikeTop + 3.0f;
}

static void changeSelection(Game *g, int direction) {
    unsigned old = g->selectedLevel;
    if (direction < 0)
        g->selectedLevel = (old == 0) ? SKY_LEVEL_COUNT - 1 : old - 1;
    else
        g->selectedLevel = (old + 1) % SKY_LEVEL_COUNT;

    if (g->selectedLevel != old)
        audioPlay(gLevels[g->selectedLevel].musicId);
}

void gameUpdate(Game *g, int keys, bool touched, int touchX, int touchY) {
    if (g->selectedLevel >= SKY_LEVEL_COUNT) g->selectedLevel = 0;

    if (g->scene == SCENE_TITLE) {
        if ((keys & (KEY_A | KEY_START)) || touched) {
            g->scene = SCENE_LEVEL_SELECT;
            audioPlay(gLevels[g->selectedLevel].musicId);
        }
        return;
    }

    if (g->scene == SCENE_LEVEL_SELECT) {
        if (keys & (KEY_LEFT | KEY_L)) changeSelection(g, -1);
        if (keys & (KEY_RIGHT | KEY_R)) changeSelection(g, 1);

        if (touched && touchY >= 104) {
            if (touchX < 80) changeSelection(g, -1);
            else if (touchX > 176) changeSelection(g, 1);
            else gameStartLevel(g, g->selectedLevel);
        } else if (keys & (KEY_A | KEY_START)) {
            gameStartLevel(g, g->selectedLevel);
        }

        if (keys & KEY_B) {
            audioPlay(MOD_MENU);
            g->scene = SCENE_TITLE;
        }
        return;
    }

    if (g->scene == SCENE_PAUSED) {
        if (keys & (KEY_A | KEY_START)) {
            g->scene = SCENE_PLAYING;
            audioResume();
        }
        if (keys & KEY_B) returnToSelect(g);
        return;
    }

    if (g->scene == SCENE_FAILED) {
        if (keys & KEY_A) gameStartLevel(g, g->selectedLevel);
        else if (keys & (KEY_B | KEY_START)) returnToSelect(g);
        return;
    }

    if (g->scene == SCENE_COMPLETE) {
        if (keys & KEY_A) {
            if (g->selectedLevel + 1 < SKY_LEVEL_COUNT)
                gameStartLevel(g, g->selectedLevel + 1);
            else
                returnToSelect(g);
        } else if (keys & (KEY_B | KEY_START)) {
            returnToSelect(g);
        }
        return;
    }

    if (keys & KEY_START) {
        g->scene = SCENE_PAUSED;
        audioPause();
        return;
    }

    if ((keys & (KEY_A | KEY_B | KEY_X | KEY_Y | KEY_UP)) || touched)
        g->jumpQueued = true;

    if (g->jumpQueued && g->grounded) {
        g->playerVelocity = JUMP_FORCE;
        g->grounded = false;
    }
    g->jumpQueued = false;

    g->playerVelocity += GRAVITY;
    g->playerY += g->playerVelocity;
    if (g->playerY >= FLOOR_Y - PLAYER_SIZE) {
        g->playerY = FLOOR_Y - PLAYER_SIZE;
        g->playerVelocity = 0.0f;
        g->grounded = true;
    }

    if (!g->grounded) {
        g->cubeAngle += 8.0f;
        if (g->cubeAngle >= 360.0f) g->cubeAngle -= 360.0f;
    } else {
        /* Snap rotation to 90-degree increments on landing. */
        g->cubeAngle = (float)((((int)(g->cubeAngle + 45.0f) / 90) * 90) % 360);
    }

    const LevelDef *level = &gLevels[g->selectedLevel];
    g->distance += level->speed;

    for (unsigned i = 0; i < level->objectCount; ++i) {
        if (hitObject(g, &level->objects[i])) {
            unsigned percent = (unsigned)(gameProgress(g) * 100.0f);
            if (percent > g->bestProgress[g->selectedLevel])
                g->bestProgress[g->selectedLevel] = percent;
            ++g->deaths;
            g->scene = SCENE_FAILED;
            audioStop();
            audioPlaySfx(SFX_EXPLOSION);
            return;
        }
    }

    if (g->distance >= level->length) {
        g->bestProgress[g->selectedLevel] = 100;
        g->scene = SCENE_COMPLETE;
        audioStop();
        audioPlaySfx(SFX_LEVEL_COMPLETE);
    }
}
