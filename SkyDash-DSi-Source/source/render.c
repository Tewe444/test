#include "render.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SW 256
#define SH 192
#define PLAYER_SCREEN_X 58.0f
#define C(r,g,b) ((u16)(RGB15((r),(g),(b)) | BIT(15)))
#define COL_BG0 C(2, 4, 15)
#define COL_BG1 C(4, 9, 25)
#define COL_GRID C(5, 15, 34)
#define COL_CYAN C(5, 29, 31)
#define COL_WHITE C(31, 31, 31)
#define COL_DARK C(1, 3, 10)

static u16 *topBuffer;
static u16 *bottomBuffer;

static void pixel(u16 *dst, int x, int y, u16 color) {
    if ((unsigned)x < SW && (unsigned)y < SH) dst[y * 256 + x] = color;
}
static void rect(u16 *dst, int x, int y, int w, int h, u16 color) {
    if (w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > SW ? SW : x + w, y1 = y + h > SH ? SH : y + h;
    for (int yy = y0; yy < y1; ++yy)
        for (int xx = x0; xx < x1; ++xx) dst[yy * 256 + xx] = color;
}
static void frameRect(u16 *dst, int x, int y, int w, int h, u16 color) {
    rect(dst, x, y, w, 2, color); rect(dst, x, y + h - 2, w, 2, color);
    rect(dst, x, y, 2, h, color); rect(dst, x + w - 2, y, 2, h, color);
}
static void clear(u16 *dst, u16 color) {
    for (int y = 0; y < SH; ++y)
        for (int x = 0; x < SW; ++x) dst[y * 256 + x] = color;
}

static const u8 glyphs[36][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {15,16,16,16,16,16,15},
    {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {15,16,16,23,17,17,15}, {17,17,17,31,17,17,17}, {31,4,4,4,4,4,31},
    {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}
};

static void drawChar(u16 *dst, int x, int y, char ch, u16 color, int scale) {
    int index = -1;
    if (ch >= 'A' && ch <= 'Z') index = ch - 'A';
    else if (ch >= '0' && ch <= '9') index = 26 + ch - '0';
    if (index >= 0) {
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (glyphs[index][row] & (1 << (4 - col))) rect(dst, x + col * scale, y + row * scale, scale, scale, color);
    } else if (ch == ':') {
        rect(dst,x+2*scale,y+2*scale,scale,scale,color); rect(dst,x+2*scale,y+5*scale,scale,scale,color);
    } else if (ch == '.') rect(dst,x+2*scale,y+6*scale,scale,scale,color);
    else if (ch == '-') rect(dst,x+1*scale,y+3*scale,3*scale,scale,color);
    else if (ch == '%') {
        pixel(dst,x,y,color); pixel(dst,x+4*scale,y+6*scale,color);
        rect(dst,x+3*scale,y,scale,7*scale,color);
    } else if (ch == '!') {
        rect(dst,x+2*scale,y,scale,5*scale,color); rect(dst,x+2*scale,y+6*scale,scale,scale,color);
    } else if (ch == '/') {
        for(int i=0;i<7;i++) pixel(dst,x+4-i/2,y+i,color);
    } else if (ch == '>') {
        for(int i=0;i<4;i++){pixel(dst,x+i,y+3-i,color);pixel(dst,x+i,y+3+i,color);}
    } else if (ch == '<') {
        for(int i=0;i<4;i++){pixel(dst,x+3-i,y+3-i,color);pixel(dst,x+3-i,y+3+i,color);}
    }
}
static void text(u16 *dst, int x, int y, const char *s, u16 color, int scale) {
    int start = x;
    while (*s) {
        if (*s == '\n') { y += 9 * scale; x = start; }
        else { drawChar(dst,x,y,*s,color,scale); x += 6 * scale; }
        ++s;
    }
}
static void centeredText(u16 *dst, int y, const char *s, u16 color, int scale) {
    int len = (int)strlen(s);
    text(dst, (SW - len * 6 * scale + scale) / 2, y, s, color, scale);
}

static void background(u16 *dst, u16 accent, unsigned tick) {
    for (int y = 0; y < SH; ++y) {
        int r = 2 + y / 35, g = 4 + y / 29, b = 15 + y / 13;
        if (r > 8) r = 8;
        if (g > 11) g = 11;
        if (b > 27) b = 27;
        u16 c = C(r,g,b);
        for (int x = 0; x < SW; ++x) dst[y*256+x] = c;
    }

    /* Distant geometric skyline and subtle parallax blocks. */
    for (int i = 0; i < 11; ++i) {
        int bw = 13 + (i % 4) * 5;
        int bh = 17 + (i % 5) * 7;
        int bx = (i * 29 - (int)(tick / 4) * (i % 2 + 1)) % (SW + 28);
        if (bx < -bw) bx += SW + 28;
        rect(dst, bx, 151 - bh, bw, bh, C(2,7,20));
        rect(dst, bx + 3, 151 - bh + 5, 2, 3, C(5,13,26));
        rect(dst, bx + 8, 151 - bh + 5, 2, 3, C(5,13,26));
        rect(dst, bx + 3, 151 - bh + 12, 2, 3, C(5,13,26));
    }

    for (int x = 0; x < SW; x += 32)
        rect(dst, (x + (tick/3)%32) % SW, 0, 1, SH, COL_GRID);
    for (int y = 24; y < SH; y += 24)
        rect(dst, 0, y, SW, 1, COL_GRID);

    /* Floating pixel stars. */
    for (int i = 0; i < 20; ++i) {
        int x = (i * 53 + (int)(tick * (i % 3 + 1))) % SW;
        int y = 14 + (i * 31) % 116;
        pixel(dst, x, y, (i % 3 == 0) ? accent : C(10,18,26));
        if ((i % 4) == 0) pixel(dst, x + 1, y, C(14,22,30));
    }
}

static void line(u16 *dst, int x0, int y0, int x1, int y1, u16 color) {
    int dx = x1-x0, dy = y1-y0;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (steps == 0) { pixel(dst,x0,y0,color); return; }
    for (int i=0;i<=steps;i++) pixel(dst,x0+dx*i/steps,y0+dy*i/steps,color);
}
static void triangle(u16 *dst, int x0,int y0,int x1,int y1,int x2,int y2,u16 color) {
    int minY=y0<y1?y0:y1, maxY=y0>y1?y0:y1;
    if(y2<minY)minY=y2;
    if(y2>maxY)maxY=y2;
    if(minY<0)minY=0;
    if(maxY>=SH)maxY=SH-1;
    for(int y=minY;y<=maxY;y++) {
        int xs[3], n=0; int xa[3]={x0,x1,x2}, ya[3]={y0,y1,y2};
        for(int e=0;e<3;e++) {
            int j=(e+1)%3;
            if ((y>=ya[e] && y<ya[j]) || (y>=ya[j] && y<ya[e])) {
                xs[n++]=xa[e]+(y-ya[e])*(xa[j]-xa[e])/(ya[j]-ya[e]);
            }
        }
        if(n>=2) { int a=xs[0]<xs[1]?xs[0]:xs[1], b=xs[0]>xs[1]?xs[0]:xs[1]; rect(dst,a,y,b-a+1,1,color); }
    }
    line(dst,x0,y0,x1,y1,C(31,31,31)); line(dst,x1,y1,x2,y2,C(31,31,31)); line(dst,x2,y2,x0,y0,C(31,31,31));
}
static void drawCube(u16 *dst, int cx, int cy, float degrees, u16 fill) {
    const float rad = degrees * 0.0174532925f;
    const float cs = cosf(rad), sn = sinf(rad), half = 8.0f;
    int px[4], py[4];
    for(int i=0;i<4;i++) {
        float vx=(i==0||i==3)?-half:half;
        float vy=(i<2)?-half:half;
        px[i]=(int)(cx+vx*cs-vy*sn); py[i]=(int)(cy+vx*sn+vy*cs);
    }
    /* Convex polygon fill by scanline. */
    int minY=py[0],maxY=py[0];
    for(int i=1;i<4;i++){if(py[i]<minY)minY=py[i];if(py[i]>maxY)maxY=py[i];}
    if(minY<0)minY=0;
    if(maxY>=SH)maxY=SH-1;
    for(int y=minY;y<=maxY;y++) {
        int xs[4],n=0;
        for(int i=0;i<4;i++){int j=(i+1)&3;if((y>=py[i]&&y<py[j])||(y>=py[j]&&y<py[i]))xs[n++]=px[i]+(y-py[i])*(px[j]-px[i])/(py[j]-py[i]);}
        if(n>=2){int a=xs[0],b=xs[1];if(a>b){int t=a;a=b;b=t;}rect(dst,a,y,b-a+1,1,fill);}
    }
    for(int i=0;i<4;i++) line(dst,px[i],py[i],px[(i+1)&3],py[(i+1)&3],COL_WHITE);
    /* Face plate and eyes give the Sky Games cube a recognizable look. */
    rect(dst,cx-4,cy-4,8,2,C(13,30,31));
    rect(dst,cx-5,cy-1,3,4,COL_DARK);
    rect(dst,cx+2,cy-1,3,4,COL_DARK);
    rect(dst,cx-2,cy+4,4,1,C(1,4,12));
}
static void drawObject(u16 *dst, const LevelObject *obj, int sx) {
    const int floor = 151;
    if (obj->type == OBJ_BLOCK) {
        rect(dst,sx,floor-22,22,22,C(6,20,29));
        frameRect(dst,sx,floor-22,22,22,C(10,30,31));
        rect(dst,sx+3,floor-19,16,3,C(10,28,31));
        rect(dst,sx+4,floor-12,3,8,C(3,11,22));
        rect(dst,sx+9,floor-12,3,8,C(3,11,22));
        return;
    }
    if (obj->type == OBJ_STOP_BLOCK) {
        /* S-block marker: visible in the editor-like level data; no dash orb yet. */
        rect(dst,sx,floor-22,22,22,C(24,20,3));
        frameRect(dst,sx,floor-22,22,22,C(31,29,8));
        text(dst,sx+8,floor-16,"S",COL_WHITE,1);
        return;
    }
    if (obj->type == OBJ_DOUBLE_SPIKE) {
        triangle(dst,sx,floor,sx+19,floor-22,sx+38,floor,C(5,22,30));
        triangle(dst,sx+19,floor,sx+38,floor-22,sx+57,floor,C(7,18,29));
        line(dst,sx+19,floor-21,sx+19,floor-3,C(13,28,31));
        line(dst,sx+38,floor-21,sx+38,floor-3,C(13,28,31));
    } else {
        triangle(dst,sx,floor,sx+10,floor-22,sx+20,floor,C(5,22,30));
        line(dst,sx+10,floor-21,sx+10,floor-4,C(13,28,31));
    }
}
static void drawGround(u16 *dst, float distance, u16 accent) {
    rect(dst,0,151,SW,41,C(2,5,14));
    rect(dst,0,151,SW,2,accent);
    int shift=(int)distance%24;
    for(int x=-shift;x<SW;x+=24){
        frameRect(dst,x,158,22,22,C(5,12,25));
        rect(dst,x+2,160,18,1,C(6,17,27));
    }
}
static void drawProgress(u16 *dst, float progress, u16 accent) {
    rect(dst,16,10,224,5,C(1,2,8));
    frameRect(dst,15,9,226,7,C(13,20,28));
    int w=(int)(222.0f*progress); if(w>0)rect(dst,17,11,w,3,accent);
}

void renderInit(void) {
    videoSetMode(MODE_5_2D | DISPLAY_BG2_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG2_ACTIVE);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    int bgTop=bgInit(2,BgType_Bmp16,BgSize_B16_256x256,0,0);
    int bgBottom=bgInitSub(2,BgType_Bmp16,BgSize_B16_256x256,0,0);
    topBuffer=(u16*)bgGetGfxPtr(bgTop);
    bottomBuffer=(u16*)bgGetGfxPtr(bgBottom);
    clear(topBuffer,COL_BG0); clear(bottomBuffer,COL_BG0);
}

static void renderTitle(unsigned tick) {
    background(topBuffer,C(3,30,25),tick);
    rect(topBuffer,26,47,204,96,C(2,7,19)); frameRect(topBuffer,26,47,204,96,C(6,25,31));
    drawCube(topBuffer,128,83,(float)(tick%90),C(4,22,31));
    centeredText(topBuffer,111,"SKY DASH",COL_WHITE,2);
    centeredText(topBuffer,139,"BY SKY GAMES",C(8,27,31),1);

    background(bottomBuffer,C(8,23,31),tick/2);
    centeredText(bottomBuffer,28,"WELCOME TO SKY GAMES",COL_WHITE,1);
    centeredText(bottomBuffer,64,"TAP OR PRESS A",C(9,30,28),2);
    centeredText(bottomBuffer,88,"TO START",COL_WHITE,2);
    centeredText(bottomBuffer,137,"OFFLINE DSI HOMEBREW",C(9,17,26),1);
}
static void renderSelect(const Game *g, unsigned tick) {
    char label[32];
    const LevelDef *level = &gLevels[g->selectedLevel];

    background(topBuffer, level->accent, tick);
    snprintf(label, sizeof(label), "LEVEL %02u / %02u",
             g->selectedLevel + 1, SKY_LEVEL_COUNT);
    centeredText(topBuffer, 17, "SELECT LEVEL", COL_WHITE, 2);
    centeredText(topBuffer, 43, label, level->accent, 1);
    rect(topBuffer, 22, 62, 212, 76, C(2,7,18));
    frameRect(topBuffer, 22, 62, 212, 76, level->accent);
    centeredText(topBuffer, 76, level->name, COL_WHITE, 2);
    centeredText(topBuffer, 103, level->difficulty, C(12,28,30), 1);
    snprintf(label, sizeof(label), "SONG: %s", level->songName);
    centeredText(topBuffer, 126, label, C(10,25,29), 1);
    centeredText(topBuffer, 163, "LEFT / RIGHT  OR  L / R", C(11,20,27), 1);

    background(bottomBuffer, level->accent, tick/2);
    centeredText(bottomBuffer, 17, "SKY GAMES", COL_WHITE, 1);
    centeredText(bottomBuffer, 42, level->name, COL_WHITE, 2);
    snprintf(label, sizeof(label), "BEST %u%%", g->bestProgress[g->selectedLevel]);
    centeredText(bottomBuffer, 76, label, C(11,30,28), 1);
    snprintf(label, sizeof(label), "TRACK %02u", g->selectedLevel + 1);
    centeredText(bottomBuffer, 99, label, C(10,22,30), 1);
    rect(bottomBuffer, 14, 124, 68, 34, C(2,7,18));
    rect(bottomBuffer, 94, 124, 68, 34, C(2,7,18));
    rect(bottomBuffer, 174, 124, 68, 34, C(2,7,18));
    frameRect(bottomBuffer, 14, 124, 68, 34, level->accent);
    frameRect(bottomBuffer, 94, 124, 68, 34, level->accent);
    frameRect(bottomBuffer, 174, 124, 68, 34, level->accent);
    centeredText(bottomBuffer, 137, "< PREV", COL_WHITE, 1);
    centeredText(bottomBuffer, 137, "A PLAY", COL_WHITE, 1);
    centeredText(bottomBuffer, 137, "NEXT >", COL_WHITE, 1);
    centeredText(bottomBuffer, 174, "B BACK  |  OFFLINE", C(9,18,25), 1);
}
static void renderPlaying(const Game *g, unsigned tick) {
    const LevelDef *level=&gLevels[g->selectedLevel];
    background(topBuffer,level->accent,tick);
    drawGround(topBuffer,g->distance,level->accent);
    for(unsigned i=0;i<level->objectCount;i++){
        int sx=(int)((float)level->objects[i].x-g->distance);
        if(sx>-60&&sx<SW+60)drawObject(topBuffer,&level->objects[i],sx);
    }
    drawCube(topBuffer,(int)PLAYER_SCREEN_X+8,(int)g->playerY+8,g->cubeAngle,C(6,24,29));
    drawProgress(topBuffer,gameProgress(g),level->accent);

    background(bottomBuffer,level->accent,tick/2);
    centeredText(bottomBuffer,20,level->name,COL_WHITE,2);
    char p[32];snprintf(p,sizeof(p),"PROGRESS %u%%",(unsigned)(gameProgress(g)*100.0f));
    centeredText(bottomBuffer,62,p,C(10,30,29),1);
    snprintf(p,sizeof(p),"DEATHS %u",g->deaths); centeredText(bottomBuffer,87,p,C(17,19,28),1);
    centeredText(bottomBuffer,108,level->songName,C(9,25,29),1);
    centeredText(bottomBuffer,126,"A / B / X / Y  JUMP",COL_WHITE,1);
    centeredText(bottomBuffer,151,"START  PAUSE",C(11,20,28),1);
    centeredText(bottomBuffer,174,"TOUCH SCREEN TO JUMP",C(7,25,28),1);
}
static void renderOverlay(const Game *g, unsigned tick) {
    u16 accent=gLevels[g->selectedLevel].accent;
    background(topBuffer,accent,tick);
    rect(topBuffer,24,52,208,90,C(1,5,14)); frameRect(topBuffer,24,52,208,90,accent);
    if(g->scene==SCENE_PAUSED){centeredText(topBuffer,73,"PAUSED",COL_WHITE,2);centeredText(topBuffer,108,"A CONTINUE",C(10,29,31),1);}
    else if(g->scene==SCENE_FAILED){centeredText(topBuffer,69,"CRASHED",COL_WHITE,2);centeredText(topBuffer,101,"TRY AGAIN",C(10,29,31),1);}
    else {centeredText(topBuffer,68,"LEVEL CLEAR",COL_WHITE,2);centeredText(topBuffer,103,"NICE RUN",C(10,29,31),1);}

    background(bottomBuffer,accent,tick/2);
    char p[32]; snprintf(p,sizeof(p),"BEST %u%%",g->bestProgress[g->selectedLevel]);
    centeredText(bottomBuffer,43,p,C(10,30,29),2);
    if(g->scene==SCENE_PAUSED){centeredText(bottomBuffer,99,"A  RESUME",COL_WHITE,1);centeredText(bottomBuffer,126,"B  LEVEL SELECT",C(14,20,26),1);}
    else if(g->scene==SCENE_FAILED){centeredText(bottomBuffer,99,"A  RETRY",COL_WHITE,1);centeredText(bottomBuffer,126,"B  LEVEL SELECT",C(14,20,26),1);}
    else {centeredText(bottomBuffer,99,"A  NEXT LEVEL",COL_WHITE,1);centeredText(bottomBuffer,126,"B  LEVEL SELECT",C(14,20,26),1);}
}

void renderFrame(const Game *game) {
    static unsigned tick=0; ++tick;
    switch(game->scene){
        case SCENE_TITLE: renderTitle(tick); break;
        case SCENE_LEVEL_SELECT: renderSelect(game,tick); break;
        case SCENE_PLAYING: renderPlaying(game,tick); break;
        default: renderOverlay(game,tick); break;
    }
    swiWaitForVBlank();
}
