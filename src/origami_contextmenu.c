#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include "microui.h"
#include "origami.h"

#define OPT_HEIGHT 20
#define MAX_HEIGHT 250

static const char *commands[] = {
    NULL
};

static OG_Viewport *this;
static char** options;
static int optionsQ;
static void (*callback)(int);

static void Init(OG_Viewport *v){
    this = v;
    v->ctx.style->spacing = 0;
    v->ctx.style->padding = 0;
    v->ctx.style->colors[MU_COLOR_BORDER] = v->ctx.style->colors[MU_COLOR_BUTTON];
}

static void Update(OG_Viewport *v){
}

static void BottomPanel(OG_Viewport *v, mu_Context *ctx){
    if (options){
        mu_layout_row(ctx, 1, (const int[]){-1}, OPT_HEIGHT);
        for (int i=0; i<optionsQ; i++){
            if (mu_button_ex(ctx, options[i], 0, 0)){
                if (callback) callback(i);
                OG_CloseViewportByName("ContextMenu");
            }
        }
    }

}

static const char **GetCmds(){
    return commands;
}

void OG_ContextMenu(){
    OG_Viewport *v = OG_InitViewport(
        "ContextMenu", 
        (Rectangle){0,0,250, OG_VIEWPORT_MIN_H}, 
        1.0f, 1.0f,
        (OG_PanelsDimensions){.bottom=250}, 
        true, false, true, 
        &Init, 
        &Update, 
        NULL,
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        &BottomPanel, 
        &GetCmds, 
        NULL
    );

    v->size.height = -1;
    v->noTitleBar = true;
}

void OG_OpenContextMenu(void (*_callback)(int), int optQ, ...){
    Vector2 mousePos = GetMousePosition();
    this->pos = mousePos;
    OG_ToggleViewportByName("ContextMenu");

    if (options){
        for (int i=0; i<optionsQ; i++)
            free(options[i]);
        free(options);
        optionsQ = 0;
    }
        
    options = calloc(optQ, sizeof(char*));
    optionsQ = optQ;

    va_list args;
    va_start(args, optQ);
    for (int i = 0; i < optQ; i++) {
        char *opt = va_arg(args, char*);
        options[i] = calloc(strlen(opt)+1, sizeof(char));
        strcpy(options[i], opt);
    }

    va_end(args);
    callback = _callback;
    this->bottomPanel.size = optQ*OPT_HEIGHT+1;
    if (this->bottomPanel.size > MAX_HEIGHT)
        this->bottomPanel.size = MAX_HEIGHT;
}