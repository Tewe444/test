#ifndef SKYDASH_GAME_H
#define SKYDASH_GAME_H

#include <nds.h>
#include <stdbool.h>
#include <stdint.h>

#define SKY_LEVEL_COUNT 20

typedef enum {
    SCENE_TITLE,
    SCENE_LEVEL_SELECT,
    SCENE_PLAYING,
    SCENE_PAUSED,
    SCENE_FAILED,
    SCENE_COMPLETE
} Scene;

typedef enum {
    OBJ_SPIKE,
    OBJ_DOUBLE_SPIKE,
    OBJ_BLOCK,
    OBJ_STOP_BLOCK
} ObjectType;

typedef struct {
    uint16_t x;
    ObjectType type;
} LevelObject;

typedef struct {
    const char *name;
    const char *difficulty;
    const char *songName;
    const LevelObject *objects;
    unsigned objectCount;
    float length;
    float speed;
    u16 accent;
    int musicId;
} LevelDef;

typedef struct {
    Scene scene;
    unsigned selectedLevel;
    unsigned deaths;
    unsigned bestProgress[SKY_LEVEL_COUNT];
    float distance;
    float playerY;
    float playerVelocity;
    float cubeAngle;
    bool grounded;
    bool jumpQueued;
} Game;

extern const LevelDef gLevels[SKY_LEVEL_COUNT];

void gameInit(Game *game);
void gameStartLevel(Game *game, unsigned level);
void gameUpdate(Game *game, int keysDown, bool touched, int touchX, int touchY);
float gameProgress(const Game *game);

#endif
