#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "microui.h"
#include "origami.h"

static const char *commands[] = {
    NULL
};

static OG_Viewport *this;
static char buf[256];
static mu_Id input_id;
static void (*Ok)(char*);

static void Init(OG_Viewport *v){
    this = v;
}

static void Update(OG_Viewport *v){
    if (v->hidden) return;
    if (OG.viewports.tail != v) return;
    v->ctx.focus = input_id;
}

static void TopPanel(OG_Viewport *v, mu_Context *ctx){
    mu_layout_row(ctx, 1, (const int[]){-1}, 15);

    // manual textbox
    input_id = mu_get_id(ctx, "BITCH", 5);
    mu_Rect r = mu_layout_next(ctx);
    if (mu_textbox_raw(ctx, buf, 256, input_id, r, 0) & MU_RES_SUBMIT){
        Ok(buf);
        OG_ToggleViewport(v);
    }

}

static const char **GetCmds(){
    return commands;
}

void OG_InputDialog(){
    OG_Viewport *v = OG_InitViewport(
        "InputDialog", 
        (Rectangle){0,0,250, OG_VIEWPORT_MIN_H}, 
        1.0f, 1.0f, 
        (OG_PanelsDimensions){}, 
        true, false, true, 
        &Init, 
        &Update, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        &TopPanel, 
        NULL, 
        &GetCmds, 
        NULL
    );

    v->topPanel.size = 25;
    v->size.height = -1;
    v->noTitleBar = true;
    v->updateAlways = true;
}

void OG_OpenInputDialog(void (*ok_callback)(char*)){
    memset(buf, 0, 256);
    Ok = ok_callback;
    Vector2 mousePos = GetMousePosition();
    this->pos = mousePos;
    OG_ToggleViewportByName("InputDialog");
}