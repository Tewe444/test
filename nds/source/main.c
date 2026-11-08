/*
 * Sky Dash - DSi Homebrew Skeleton
 * Rebranded concept based on Geometry Dash-style gameplay ideas.
 * Online / WLAN features intentionally removed.
 *
 * This is a MINIMAL skeleton only.
 * A full 1:1 port of the web version is not possible without a complete rewrite
 * of the entire game engine in native C/C++ for the Nintendo DS/DSi hardware.
 *
 * Built with libnds / devkitPro.
 */

#include <nds.h>
#include <stdio.h>
#include <gl2d.h>   // optional, for simple 2D

// Simple title screen
void showTitleScreen(void) {
    // Clear both screens
    consoleDemoInit();

    iprintf("\n\n");
    iprintf("        SKY DASH\n");
    iprintf("\n");
    iprintf("      by Sky Games\n");
    iprintf("\n\n");
    iprintf("  DSi Homebrew Skeleton\n");
    iprintf("\n");
    iprintf("  Online features removed.\n");
    iprintf("\n\n");
    iprintf("  This is NOT a full game.\n");
    iprintf("  Full Geometry Dash-style\n");
    iprintf("  physics + levels would\n");
    iprintf("  require a complete native\n");
    iprintf("  rewrite (months of work).\n");
    iprintf("\n\n");
    iprintf("  Press START to exit.\n");
}

int main(void) {
    // Initialize the DS
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    // Init console on top screen
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);

    showTitleScreen();

    // Simple main loop
    while (1) {
        swiWaitForVBlank();
        scanKeys();
        u16 keys = keysDown();

        if (keys & KEY_START) {
            break;
        }
    }

    return 0;
}
