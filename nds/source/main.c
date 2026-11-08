#include <nds.h>
#include <stddef.h>
#include <stdio.h>
#include <filesystem.h>
#include "audio.h"
#include "game.h"
#include "render.h"

int main(void) {
    Game game;
    touchPosition touch;

    /* The soundbank is in the embedded NitroFS image; no network or SD access. */
    if (!nitroFSInit(NULL)) {
        consoleDemoInit();
        printf("SKY DASH\n\nNitroFS could not be mounted.\nRebuild the NDS with NitroFS enabled.");
        while (1) swiWaitForVBlank();
    }

    audioInit();
    gameInit(&game);
    renderInit();
    lcdMainOnTop();

    while (1) {
        scanKeys();
        const int pressed = keysDown();
        const bool touched = (pressed & KEY_TOUCH) != 0;
        int tx = 0, ty = 0;

        if (touched) {
            touchRead(&touch);
            tx = touch.px;
            ty = touch.py;
        }

        gameUpdate(&game, pressed, touched, tx, ty);
        renderFrame(&game);
    }
    return 0;
}
