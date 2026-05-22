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

typedef struct ContextMenu {
    char** options;
    int optionsQ;
    void (*callback)(int);
    void (*callbackStr)(char*);
} ContextMenu;

static const char *commands[] = {
    NULL
};

static OG_Viewport *this;
static ContextMenu menus[32];
static int menusQ = 0;
static bool dontCloseFlag = false;

static void CloseContextMenu(){
    for (int i=0; i<menus[0].optionsQ; i++)
        free(menus[0].options[i]);
    free(menus[0].options);

    for (int i = 1; i < 32; i++) {
        menus[i - 1] = menus[i];
    }

    menusQ--;

    if (menusQ == 0)
        OG_CloseViewportByName("ContextMenu");
}

static void CloseAllContextMenus() {
    for (int m = 0; m < menusQ; m++) {
        for (int i = 0; i < menus[m].optionsQ; i++)
            free(menus[m].options[i]);
        free(menus[m].options);

        menus[m].options = NULL;
        menus[m].optionsQ = 0;
        menus[m].callback = NULL;
        menus[m].callbackStr = NULL;
    }

    menusQ = 0;
    OG_CloseViewportByName("ContextMenu");
}

static void Init(OG_Viewport *v){
    this = v;
    v->ctx.style->spacing = 0;
    v->ctx.style->padding = 0;
    v->ctx.style->colors[MU_COLOR_BORDER] = v->ctx.style->colors[MU_COLOR_BUTTON];
}

static void Update(OG_Viewport *v){
    if (IsKeyPressed(KEY_ESCAPE)){
        CloseAllContextMenus();
    }
    
    if (v->hidden){
        if (menusQ > 0)
            OG_OpenViewportByName("ContextMenu");    
        return;
    }

    if (OG.viewports.tail != v) return;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !OG.viewportJustSwitched && !dontCloseFlag){
        if (!OG_MouseInViewport(v, false, false, false)){
            CloseAllContextMenus();
        }
    }

    dontCloseFlag = false;
}

static void BottomPanel(OG_Viewport *v, mu_Context *ctx){
    if (menus[menusQ-1].options){
        mu_layout_row(ctx, 1, (const int[]){-1}, OPT_HEIGHT);
        for (int i=0; i<menus[menusQ-1].optionsQ; i++){
            if (mu_button_ex(ctx, menus[menusQ-1].options[i], 0, 0)){
                if (menus[menusQ-1].callback) menus[menusQ-1].callback(i);
                else if (menus[menusQ-1].callbackStr) menus[menusQ-1].callbackStr(menus[menusQ-1].options[i]);
                CloseContextMenu();
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

static ContextMenu *OpenContextMenuHelper(void (*_callback)(int), int optQ){
    Vector2 mousePos = GetMousePosition();
    if (menusQ == 0)
        this->pos = mousePos;

    menus[menusQ].options = calloc(optQ, sizeof(char*));
    menus[menusQ].optionsQ = optQ;

    menus[menusQ].callback = _callback;
    menus[menusQ].callbackStr = NULL;
    this->bottomPanel.size = optQ*OPT_HEIGHT+1;
    if (this->bottomPanel.size > MAX_HEIGHT)
        this->bottomPanel.size = MAX_HEIGHT;

    menusQ++;
    dontCloseFlag = true;
    return &menus[menusQ-1];
}

void OG_OpenContextMenu(void (*_callback)(int), char *hint, int optQ, ...){
    this->header = hint;
    this->noTitleBar = hint == NULL;
    
    ContextMenu *cm = OpenContextMenuHelper(_callback, optQ);

    va_list args;
    va_start(args, optQ);
    for (int i = 0; i < optQ; i++) {
        char *opt = va_arg(args, char*);
        cm->options[i] = calloc(strlen(opt)+1, sizeof(char));
        strcpy(cm->options[i], opt);
    }

    va_end(args);
}

void OG_OpenContextMenuV2(void (*_callback)(int), char *hint, int optQ, char **opts){
    this->header = hint;
    this->noTitleBar = hint == NULL;
    
    ContextMenu *cm = OpenContextMenuHelper(_callback, optQ);
    for (int i = 0; i < optQ; i++) {
        char *opt = opts[i];
        cm->options[i] = calloc(strlen(opt)+1, sizeof(char));
        strcpy(cm->options[i], opt);
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
    
    ContextMenu *cm = OpenContextMenuHelper(NULL, Q);
    cm->callbackStr = _callback;
    cm->callback = NULL;

    int ii=0;
    for (int i = 0; i < optQ; i++) {
        char *opt = opts[i];
        if (opt && strcmp(opt, "") != 0){
            cm->options[ii] = calloc(strlen(opt)+1, sizeof(char));
            strcpy(cm->options[ii++], opt);
        }
    }

}