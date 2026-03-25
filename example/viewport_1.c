#include <raylib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "microui.h"
#include "origami.h"

static const char *commands[] = {
    NULL
};

Color ballColor = {61,194,72,255};

/* <== Utils ===================================================> */

extern void DrawGrid2D(int w, int h, int gridSize, Color c);

static int uint8_slider(mu_Context *ctx, unsigned char *value, int low, int high) {
  static float tmp;
  mu_push_id(ctx, &value, sizeof(value));
  tmp = *value;
  int res = mu_slider_ex(ctx, &tmp, low, high, 0, "%.0f", MU_OPT_ALIGNCENTER);
  *value = tmp;
  mu_pop_id(ctx);
  return res;
}

/* <== Callbacks ===============================================> */

static void Init(OG_Viewport *v){
    v->rightPanel.resizable = true;
    v->camera.zoom = 0.7;
}

static void Update(OG_Viewport *v){
    OG_ViewportUpdatePan(v);
    OG_ViewportUpdateZoom(v);
}

static void OnResize(OG_Viewport *v){
    OG_PushLog("Viewport -> %s  New Size -> w:%f h:%f", v->title, v->size.width, v->size.height);
}

static void Render(OG_Viewport *v){
    DrawGrid2D(v->size.width, v->size.height*-1, 50, (Color){100,100,100,255});
    DrawCircle(0,0,50,ballColor);
}

static void RenderOverlay(OG_Viewport *v){
    DrawTextEx(
        OG.defaultFont, 
        "*Mouse middle clk to pan \n*Mouse wheel to zoom",
        (Vector2){0,0}, 
        16, 
        2, 
        WHITE
    );
}

static void RenderUnderlay(OG_Viewport *v){
    DrawTextEx(
        OG.defaultFont, 
        "Hello world! \nfrom an underlay!",
        (Vector2){0,(v->size.height*-1)-16*2}, 
        16, 
        2, 
        YELLOW
    );
}

static void RightPanel(OG_Viewport *v, mu_Context *mu){
    if (mu_header_ex(mu, "Ball Color:", MU_OPT_EXPANDED)){
        mu_layout_row(mu, 2, (const int[]){20, -1}, 0);
        mu_label(mu, "R:"); uint8_slider(mu, &ballColor.r, 0, 255);
        mu_label(mu, "G:"); uint8_slider(mu, &ballColor.g, 0, 255);
        mu_label(mu, "B:"); uint8_slider(mu, &ballColor.b, 0, 255);
        mu_label(mu, "A:"); uint8_slider(mu, &ballColor.a, 0, 255);
    }
}

static const char **GetCmds(){
    return commands;
}

static void ExecCmd(OG_Viewport *v, int argc, char **argv){}

/* <== Register Function =======================================> */

void Viewport1(){
    OG_InitViewport(
        "viewport_1", 
        (Rectangle){50,50,300,300}, 
        0.3f, 5.0f, 
        (OG_PanelsDimensions){0,200,0,0}, 
        false, true, false, 
        &Init, 
        &Update, 
        &OnResize, 
        &Render, 
        &RenderOverlay, 
        &RenderUnderlay, 
        &RightPanel, 
        NULL,
        NULL,
        NULL,
        &GetCmds, 
        &ExecCmd
    );
}