#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include "microui.h"
#include "origami.h"
#include "origami_tools.h"

#define OPT_HEIGHT 20
#define MAX_HEIGHT 250

static const char *commands[] = {
    NULL
};

static OG_Viewport *this;
static char** options;
static int optionsQ;
static void (*callback)(int);
static void (*callbackStr)(char*);

static void Init(OG_Viewport *v){
    this = v;
    v->ctx.style->spacing = 0;
    v->ctx.style->padding = 0;
    v->ctx.style->colors[MU_COLOR_BORDER] = v->ctx.style->colors[MU_COLOR_BUTTON];
}

static void Update(OG_Viewport *v){
    if (v->hidden) return;
    if (OG.viewports.tail != v) return;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !OG.viewportJustSwitched){
        if (!OG_MouseInViewport(v, false, false, false)){
            OG_CloseViewportByName("ContextMenu");
        }
    }
}

static void BottomPanel(OG_Viewport *v, mu_Context *ctx){
    if (options){
        mu_layout_row(ctx, 1, (const int[]){-1}, OPT_HEIGHT);
        for (int i=0; i<optionsQ; i++){
            if (mu_button_ex(ctx, options[i], 0, 0)){
                if (callback) callback(i);
                else if (callbackStr) callbackStr(options[i]);
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
        NULL, 
        &BottomPanel, 
        &GetCmds, 
        NULL
    );

    v->size.height = -1;
    v->updateAlways = true;
}

static void OpenContextMenuHelper(void (*_callback)(int), int optQ){
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

    callback = _callback;
    callbackStr = NULL;
    this->bottomPanel.size = optQ*OPT_HEIGHT+1;
    if (this->bottomPanel.size > MAX_HEIGHT)
        this->bottomPanel.size = MAX_HEIGHT;

    // Force microUI update to prevent old context menu from appearing 1 frame
    mu_begin(&this->ctx);
    mu_Rect rect = mu_rect(
        this->pos.x + this->leftPanel.size+1, 
        this->pos.y + (this->size.height*-1) + (this->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + this->topPanel.size +1, 
        this->size.width, 
        this->bottomPanel.size-1 
    );

    mu_begin_window_ex(&this->ctx, "bottomPanel", rect, MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE);
    mu_get_current_container(&this->ctx)->rect = rect;
    BottomPanel(this, &this->ctx);
    mu_end_window(&this->ctx);
    mu_end(&this->ctx);
}

void OG_OpenContextMenu(void (*_callback)(int), char *hint, int optQ, ...){
    this->header = hint;
    this->noTitleBar = hint == NULL;
    
    OpenContextMenuHelper(_callback, optQ);

    va_list args;
    va_start(args, optQ);
    for (int i = 0; i < optQ; i++) {
        char *opt = va_arg(args, char*);
        options[i] = calloc(strlen(opt)+1, sizeof(char));
        strcpy(options[i], opt);
    }

    va_end(args);
}

void OG_OpenContextMenuV2(void (*_callback)(int), char *hint, int optQ, char **opts){
    this->header = hint;
    this->noTitleBar = hint == NULL;
    
    OpenContextMenuHelper(_callback, optQ);
    for (int i = 0; i < optQ; i++) {
        char *opt = opts[i];
        options[i] = calloc(strlen(opt)+1, sizeof(char));
        strcpy(options[i], opt);
    }
}

void OG_OpenContextMenuV3(void (*_callback)(char*), char *hint, int optQ, char **opts){
    this->header = hint;
    this->noTitleBar = hint == NULL;
    
    // discard empty options
    int Q=0;
    for (int i = 0; i < optQ; i++) {
        char *opt = opts[i];
        if (opt && strcmp(opt, "") != 0)
            Q++;
    }
    
    OpenContextMenuHelper(NULL, Q);
    callbackStr = _callback;
    callback = NULL;

    int ii=0;
    for (int i = 0; i < optQ; i++) {
        char *opt = opts[i];
        if (opt && strcmp(opt, "") != 0){
            options[ii] = calloc(strlen(opt)+1, sizeof(char));
            strcpy(options[ii++], opt);
        }
    }

}