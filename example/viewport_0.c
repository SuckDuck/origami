#include <raylib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "microui.h"
#include "origami.h"

static const char *commands[] = {
    "ADD",
    NULL
};

static Vector2 ballPosition;

/* <== Utils ===================================================> */

void DrawGrid2D(int w, int h, int gridSize, Color c){
    int maxX = (w/2 / gridSize + 1) * gridSize;
    int maxY = (h/2 / gridSize + 1) * gridSize;

    for (int x = -maxX; x <= maxX; x += gridSize)
        DrawLine(x, -maxY, x, maxY, c);
    
    for (int y = -maxY; y <= maxY; y += gridSize)
        DrawLine(-maxX, y, maxX, y, c);
}

/* <== Callbacks ===============================================> */

static void Init(OG_Viewport *v){
    v->leftPanel.resizable = true;
    v->rightPanel.resizable = true;
    v->topPanel.resizable = true;
    v->bottomPanel.resizable = true;
    v->camera.zoom = 0.7;
}

static void Update(OG_Viewport *v){
    OG_ViewportUpdatePan(v);
    OG_ViewportUpdateZoom(v);
    ballPosition = OG_GetMouseViewportPosition(v);
}

static void OnResize(OG_Viewport *v){
    OG_PushLog("Viewport -> %s  New Size -> w:%f h:%f", v->title, v->size.width, v->size.height);
}

static void Render(OG_Viewport *v){
    DrawGrid2D(v->size.width, v->size.height*-1, 50, (Color){80,80,80,255});
    Vector2 textSize = MeasureTextEx(OG.defaultFont, "Hello World!", 32, 0.1f);
    DrawTextEx(OG.defaultFont, "Hello World!", (Vector2){-textSize.x/2,-textSize.y/2}, 32, 0.1f, WHITE);
    DrawCircle(ballPosition.x, ballPosition.y, 10/v->camera.zoom, RED);
}

static void RenderOverlay(OG_Viewport *v){}
static void RenderUnderlay(OG_Viewport *v){}

static void RightPanel(OG_Viewport *v, mu_Context *mu){
    mu_Container *c = mu_get_current_container(mu);
    int hs = (c->rect.h - mu->style->spacing*6 - 2) / 5;
    
    mu_layout_row(mu, 1, (const int[]){-1}, hs);
    char *words[] = {"WAVE","SAND", "HILL", "ROAD", "PATH"};
    for (int i=0; i<5; i++)
        if (mu_button(mu, words[i]))  OG_PushLog("%s", words[i]);
}

static void LeftPanel(OG_Viewport *v, mu_Context *mu){
    mu_Container *c = mu_get_current_container(mu);
    int hs = (c->rect.h - mu->style->spacing*6 - 2) / 5;
    
    mu_layout_row(mu, 1, (const int[]){-1}, hs);
    char *words[] = {"SKY","SUN", "MOON", "STAR", "FIRE"};
    for (int i=0; i<5; i++)
        if (mu_button(mu, words[i]))  OG_PushLog("%s", words[i]);
}

static void TopPanel(OG_Viewport *v, mu_Context *mu){
    mu_Container *c = mu_get_current_container(mu);
    int ws = (c->rect.w - mu->style->spacing*5 - 2) / 4;

    int height = c->rect.h - mu->style->spacing*2 - 2;
    mu_layout_row(mu, 4, (const int[]){ws,ws,ws,-1}, height);
    char *words[] = {"WIND","RAIN", "ROCK", "TREE"};
    for (int i=0; i<4; i++)
        if (mu_button(mu, words[i]))  OG_PushLog("%s", words[i]);
}

static void BottomPanel(OG_Viewport *v, mu_Context *mu){
    mu_Container *c = mu_get_current_container(mu);
    int ws = (c->rect.w - mu->style->spacing*5 - 2) / 4;

    int height = c->rect.h - mu->style->spacing*2 - 2;
    mu_layout_row(mu, 4, (const int[]){ws,ws,ws,-1}, height);
    char *words[] = {"LEAF","DOOR", "WALL", "LIGHT"};
    for (int i=0; i<4; i++)
        if (mu_button(mu, words[i]))  OG_PushLog("%s", words[i]);
}

static const char **GetCmds(){
    return commands;
}

static void ExecCmd(OG_Viewport *v, int argc, char **argv){
    if (strcmp(argv[0], "ADD") == 0){
        int rs = 0;
        for (int i=1; i<argc; i++){
            rs += atoi(argv[i]);
        }
        OG_PushLog("ADD RESULT = %i", rs);
    }
}

/* <== Register Function =======================================> */

void Viewport0(){
    OG_InitViewport(
        "viewport_0", 
        (Rectangle){0,0,300,300}, 
        0.3f, 5.0f, 
        (OG_PanelsDimensions){100,100,100,100}, 
        false, true, false, 
        &Init, 
        &Update, 
        &OnResize, 
        &Render, 
        &RenderOverlay, 
        &RenderUnderlay, 
        &RightPanel, 
        &LeftPanel, 
        &TopPanel, 
        &BottomPanel, 
        &GetCmds, 
        &ExecCmd
    );
}