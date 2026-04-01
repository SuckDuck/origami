#include <linux/limits.h>
#include <raylib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "origami.h"
#include "microui.h"
#include "origami_filedialog.h"

typedef struct FileEntrie {
    char *path;
    char *filename;
    bool isDir;
} FileEntrie;

typedef struct FileList {
    unsigned int count;
    FileEntrie *entries;
} FileList;

static const char *commands[] = {
    NULL
};

static FileList pwdFiles;
static FileEntrie *selectedFile;
static char filename[PATH_MAX];
static float contentSize = 0.0f;
static float scroll = 0.0f;

static OG_FileDialogMode currentMode;
static bool (*Ok)(char*);

static FileList InitFileList(char *dir){
    FilePathList f = LoadDirectoryFiles(dir);
    
    FileList fl;
    fl.count = f.count;
    fl.entries = calloc(f.count, sizeof(FileEntrie));

    for (int i=0; i<f.count; i++){
        FileEntrie *e = &fl.entries[i];
        e->isDir = false;

        // +2 because of \0 and / in case of path being a dir
        int len = strlen(f.paths[i])+2;
        e->path = calloc(len, sizeof(char));
        strcpy(e->path, f.paths[i]);
        e->filename = (char*) GetFileName(e->path);
        if (!IsPathFile(e->path)){
            e->isDir = true;
            e->path[len-2] = '/';
        }
    }

    UnloadDirectoryFiles(f);
    return fl;
}

static void FreeFileList(FileList fl){
    for (int i=0; i<fl.count; i++){
        FileEntrie *e = &fl.entries[i];
        free(e->path);
    }
        
    free(fl.entries);
    fl.count = 0;
}

static void Init(OG_Viewport *v){
    pwdFiles = InitFileList(".");
}

static void Update(OG_Viewport *v){
    // Scroll logic
    contentSize = (OG.defaultFontSize+2) * pwdFiles.count;
    if (contentSize <= v->size.height*-1)
        scroll = 0;
    
    else {
        scroll += OG_GetMouseWheelMove(v) * OG_SCROLL_SPEED;
        if (scroll > 0) scroll = 0;
        if (scroll < v->size.height*-1 - contentSize){
            scroll = v->size.height*-1 - contentSize;
        }
    }

    // Select entrie
    float verticalOffset = 0;
    for (int i=0; i<pwdFiles.count; i++){
        FileEntrie *e = &pwdFiles.entries[i];
        Rectangle r = {
            0, scroll+verticalOffset, 
            v->size.width, 
            OG.defaultFontSize+2
        };

        if (OG_IsMouseButtonPressed(v, MOUSE_LEFT_BUTTON)){
            if (CheckCollisionPointRec(OG_GetMouseOverlayPosition(v), r)){
                if (selectedFile == e){
                    if (e->isDir){
                        if (ChangeDirectory(e->path)){
                            selectedFile = NULL;
                            scroll = 0;
                            FreeFileList(pwdFiles);
                            pwdFiles = InitFileList(".");
                        }

                        else{
                            OG_PushLog("Dir could not be changed");
                        }
                    }
                    
                    else{
                        if (Ok != NULL && Ok(filename))
                            OG_ToggleViewportByName(v->title);
                            
                    }
                }
                    
                else {
                    selectedFile = e;
                    strcpy(filename, e->filename);
                }

            }
        }

        verticalOffset += OG.defaultFontSize+2;
    }

    // a little workaround to force 
    // the update of the scrollbar :)
    // *go to TopPanel*
    v->renderAlways = false;
    v->updateAlways = false;
}

static const char **GetCmds(){
    return commands;
}

static void RenderOverlay(OG_Viewport *v){
    float verticalOffset = 0.0f;
    for (int i=0; i<pwdFiles.count; i++){
        FileEntrie *e = &pwdFiles.entries[i];
        DrawTextEx(
            OG.defaultFont, 
            e->filename, 
            (Vector2){0, scroll + verticalOffset},
            OG.defaultFontSize, 2, selectedFile == e ? OG_HIGHLIGHT_C:OG_TEXT_C
        );

        verticalOffset += OG.defaultFontSize+2;
    }

    // RENDER SCROLL BAR
    if (contentSize > v->size.height*-1){
        DrawRectangle( //bg
            v->size.width - (OG.defaultFontSize+2), 0,
            (OG.defaultFontSize+2), v->size.height*-1,
            *(Color*) &v->ctx.style->colors[MU_COLOR_SCROLLBASE]
        );

        float scrollPercent = scroll*-1/contentSize;
        DrawRectangle( //thumb
            v->size.width - (OG.defaultFontSize+2), v->size.height*-1*scrollPercent, 
            (OG.defaultFontSize+2), 
            (v->size.height*-1/contentSize)*(v->size.height*-1),
            *(Color*) &v->ctx.style->colors[MU_COLOR_SCROLLTHUMB]
        );
    }

}

static void TopPanel(OG_Viewport *v, mu_Context *ctx){
    mu_layout_row(ctx, 2, (const int[]){-40, -1}, 25);
    mu_textbox_ex(ctx, (char*) GetWorkingDirectory(), PATH_MAX, MU_OPT_NOINTERACT);
    if(mu_button(ctx, "Up")){
        if (ChangeDirectory("..")){
            scroll = 0;
            selectedFile = NULL;
            FreeFileList(pwdFiles);
            pwdFiles = InitFileList(".");
        }

        // a little workaround to force 
        // the update of the scrollbar :)
        // *go to Update*
        v->renderAlways = true;
        v->updateAlways = true;
    }
}

static void BottomPanel(OG_Viewport *v, mu_Context *ctx){
    
    mu_layout_row(ctx, 4, (const int[]){80, -150, 70, -1}, 25);
    
    if (currentMode != OG_FD_MODE_SELECT_DIR){
        mu_label(ctx, "FileName:");
        if (mu_textbox(ctx, filename, PATH_MAX) && MU_RES_CHANGE){
            selectedFile = NULL;
        }
    }
    
    else{
        mu_Rect r = mu_layout_next(ctx);
        mu_draw_rect(ctx, r, ctx->style->colors[MU_COLOR_WINDOWBG]);

        r = mu_layout_next(ctx);
        mu_draw_rect(ctx, r, ctx->style->colors[MU_COLOR_WINDOWBG]);
    }
    
    if (currentMode == OG_FD_MODE_SELECT_DIR){
        if (selectedFile && selectedFile->isDir){
            if (mu_button(ctx, "Ok") && Ok && Ok(filename))
                OG_ToggleViewportByName(v->title);
        }
        
        else {
            mu_Rect r = mu_layout_next(ctx);
            mu_draw_rect(ctx, r, ctx->style->colors[MU_COLOR_WINDOWBG]);
        }
    }
    
    else if (mu_button(ctx, "Ok")){
        if (selectedFile && selectedFile->isDir){
            if (ChangeDirectory(selectedFile->path)){
                selectedFile = NULL;
                scroll = 0;
                FreeFileList(pwdFiles);
                pwdFiles = InitFileList(".");
            }
        }
        
        else if (Ok != NULL && Ok(filename))
            OG_ToggleViewportByName(v->title);
    }

    if (mu_button(ctx, "Cancel")){
        if (selectedFile)
            selectedFile = NULL;
        else OG_ToggleViewportByName(v->title);
    }

}

void OG_FileDialog(){
    OG_Viewport *v = OG_InitViewport(
        "FileDialog", 
        (Rectangle){0,0,500,300}, 
        1.0f, 1.0f, 
        (OG_PanelsDimensions){}, 
        true, false, true, 
        &Init, 
        &Update, 
        NULL, 
        NULL, 
        &RenderOverlay, 
        NULL, 
        NULL, 
        NULL, 
        &TopPanel, 
        &BottomPanel, 
        &GetCmds, 
        NULL
    );

    v->bottomPanel.size = 38;
    v->topPanel.size = 38;
}

void OG_OpenFileDialog(bool (*ok_callback)(char*), OG_FileDialogMode mode){
    Ok = ok_callback;
    currentMode = mode;
    selectedFile = NULL;
    OG_ToggleViewportByName("FileDialog");
}