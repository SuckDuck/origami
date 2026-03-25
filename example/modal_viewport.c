#include <stddef.h>
#include <stdbool.h>
#include <raylib.h>
#include "origami.h"

static const char *commands[] = {
    NULL
};

static const char **GetCmds(){
    return commands;
}

static void RenderOverlay(OG_Viewport *v){
    DrawTextEx(
        OG.defaultFont,
        "Hello World!\n\n" 
        "this is a modal viewport!\n"
        "You will not be able to interact\n"
        "with any other viewport\n"
        "Until you close this one!\n\n"
        "Press Esc over a viewport to close it",
        (Vector2){0,0},
        16,2,WHITE
    );
}

void ModalViewport(){
    OG_InitViewport(
        "Modal Viewport", 
        (Rectangle){0,0,400,200}, 
        1.0f, 1.0f, 
        (OG_PanelsDimensions){0,0,0,0},
        false, false, true, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        &RenderOverlay, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        NULL, 
        &GetCmds, 
        NULL
    );
}