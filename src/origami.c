#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <raylib.h>
#include <rlgl.h>
#include "origami.h"
#include "DejaVuSansMono-Bold.h"
#include "icons.h"
#include "microui.h"

OG_Context OG;
extern void OG_FileDialog();
extern void OG_InputDialog();
extern void OG_ContextMenu();

/* <== Utils ===================================================> */

static int text_width(mu_Font font, const char *str, int len){
    return MeasureTextEx(*(Font*)font,str,OG.defaultFontSize,2).x;
}

static int text_height(mu_Font font){
    return OG.defaultFontSize;
}

static RenderTexture2D LoadCustomRenderTexture(int width, int height){
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(); // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(NULL, width, height, PIXELFORMAT_UNCOMPRESSED_R8G8B8, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
        target.texture.mipmaps = 1;

        // Create depth renderbuffer/texture
        target.depth.id = rlLoadTextureDepth(width, height, true);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19; //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach color texture and depth renderbuffer/texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

static bool IsPointOnRect(Vector2 p, Rectangle r){
    return p.x > r.x && p.x < r.x+r.width && p.y > r.y && p.y < r.y+r.height;
}

static void GetPanelsHandles(OG_Viewport *v, Rectangle *L, Rectangle *R, Rectangle *T, Rectangle *B){
    if (v->LeftPanel && L){
        *L = (Rectangle){
            v->pos.x + v->leftPanel.size - OG_VIEWPORT_PANEL_HANDLE_S,
            v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H),
            OG_VIEWPORT_PANEL_HANDLE_S*2,
            (v->size.height*-1)+v->topPanel.size+v->bottomPanel.size
        };
    }

    if (v->RightPanel && R){
        *R = (Rectangle){
            v->pos.x + v->leftPanel.size + v->size.width - OG_VIEWPORT_PANEL_HANDLE_S,
            v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H),
            OG_VIEWPORT_PANEL_HANDLE_S*2,
            (v->size.height*-1)+v->topPanel.size+v->bottomPanel.size
        };
    }

    if (v->TopPanel && T){
        *T = (Rectangle){
            v->pos.x + v->leftPanel.size,
            v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + v->topPanel.size - OG_VIEWPORT_PANEL_HANDLE_S,
            v->size.width,
            OG_VIEWPORT_PANEL_HANDLE_S*2
        };
    }

    if (v->BottomPanel && B){
        *B = (Rectangle){
            v->pos.x + v->leftPanel.size,
            v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + v->topPanel.size + (v->size.height*-1) - OG_VIEWPORT_PANEL_HANDLE_S,
            v->size.width,
            OG_VIEWPORT_PANEL_HANDLE_S*2
        };
    }
}

static Color InvertColor(Color color){
    return (Color){
        255 - color.r,
        255 - color.g,
        255 - color.b,
        255
    };
}

static Rectangle GetViewportRect(OG_Viewport *v){
    Rectangle viewportArea = (Rectangle){
        v->pos.x,v->pos.y,
        v->size.width + v->rightPanel.size + v->leftPanel.size,
        (v->size.height*-1) + v->topPanel.size + v->bottomPanel.size + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H)
    };

    return viewportArea;
}

static bool IsViewportFullyVisible(OG_Viewport *v){
    if (!v) return false;
    if (v->hidden) return false;

    Rectangle vRect = GetViewportRect(v);
    for (OG_Viewport *vv = v->next; vv != NULL; vv = vv->next){ // for each viewport on front v
        if (vv->hidden) continue;
        Rectangle vvRect = GetViewportRect(vv);
        if (CheckCollisionRecs(vRect, vvRect))
            return false;
    }

    return true;
}

/* <== Logs Section ============================================> */

void OG_PushLog(char *format, ...){
    va_list args;
    
    // PUSH THE LOG TO BE PRINTED IN SCREEN
    if (OG.logsQ < 256){
        va_start(args, format);
        OG.logs[OG.logsQ] = (char*) calloc(256, sizeof(char));
        vsnprintf(OG.logs[OG.logsQ++], 256, format, args);
        va_end(args);
    }
    
    // PUSH THE LOG TO BE PRINTED IN STDOUT
    va_start(args, format);
    int logLen = strlen(format);
    char logFormat[logLen+2];
    strcpy(logFormat,format);
    logFormat[logLen] = '\n';
    logFormat[logLen + 1] = '\0';
    vprintf(logFormat,args);
    va_end(args);

    if (OG.logsQ == 1)
        OG.logsTimer = 0;
}

void OG_PushLogSimple(char *log){
    OG_PushLog("%s", log);
}

static void PopLog(){
    if (OG.logsQ <= 0) return; 
    free(OG.logs[0]);
    for (int i=0; i<OG.logsQ-1; i++){
        OG.logs[i] = OG.logs[i+1];
    }
    OG.logsQ--;
    OG.logsTimer = 0.0;
}


/* <== UI Section ==============================================> */

static void UpdateViewportUIInput(OG_Viewport *v){
    Vector2 mousePosition = GetMousePosition();
    mu_input_mousemove(&v->ctx, mousePosition.x, mousePosition.y);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) mu_input_mousedown(&v->ctx, mousePosition.x,mousePosition.y,MU_MOUSE_LEFT);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) mu_input_mouseup(&v->ctx,mousePosition.x,mousePosition.y,MU_MOUSE_LEFT);
    mu_input_scroll(&v->ctx, 0, GetMouseWheelMove()*OG_SCROLL_SPEED*-1);
    if (IsKeyPressed(KEY_LEFT_SHIFT)) mu_input_keydown(&v->ctx, MU_KEY_SHIFT);
    if (IsKeyReleased(KEY_LEFT_SHIFT)) mu_input_keyup(&v->ctx, MU_KEY_SHIFT); 
    if (IsKeyPressed(KEY_ENTER)) mu_input_keydown(&v->ctx, MU_KEY_RETURN);
    if (IsKeyReleased(KEY_ENTER)) mu_input_keyup(&v->ctx, MU_KEY_RETURN);    
    if (IsKeyPressed(KEY_BACKSPACE)) mu_input_keydown(&v->ctx, MU_KEY_BACKSPACE);
    if (IsKeyReleased(KEY_BACKSPACE)) mu_input_keyup(&v->ctx, MU_KEY_BACKSPACE);

    char pressedChar[2] = {0};
    while (true) {
        *pressedChar = GetCharPressed();
        if (*pressedChar == 0) break;
        mu_input_text(&v->ctx, pressedChar);
    }
}

static void CleanViewportUIInput(OG_Viewport *v){
    mu_input_mousemove(&v->ctx, 0, 0);
    mu_input_mouseup(&v->ctx, 0, 0, MU_MOUSE_LEFT);
    mu_input_mouseup(&v->ctx, 0, 0, MU_MOUSE_RIGHT);
    mu_input_scroll(&v->ctx, 0, 0);
}

static void ProcessViewportUI(OG_Viewport *v){
    mu_Context *ctx = &v->ctx;
    mu_begin(ctx);
    
    // RIGHT PANEL
    if (v->RightPanel != NULL){
        mu_Rect rect = mu_rect(
            v->pos.x+v->size.width + v->leftPanel.size + 2, 
            v->pos.y+(v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H), 
            v->rightPanel.size-1, 
            (v->size.height*-1)+v->topPanel.size+v->bottomPanel.size);

        mu_begin_window_ex(ctx, "rightPanel", rect, MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE);
        mu_get_current_container(ctx)->rect = rect;
        v->RightPanel(v,&v->ctx);
        mu_end_window(ctx);
    }

    // LEFT PANEL
    if (v->LeftPanel != NULL){
        mu_Rect rect = mu_rect(
            v->pos.x, 
            v->pos.y+(v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H), 
            v->leftPanel.size, 
            (v->size.height*-1)+v->topPanel.size+v->bottomPanel.size );

        mu_begin_window_ex(ctx, "leftPanel", rect, MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE);
        mu_get_current_container(ctx)->rect = rect;
        v->LeftPanel(v,&v->ctx);
        mu_end_window(ctx);
    }

    // TOP PANEL
    if (v->TopPanel != NULL){
        mu_Rect rect = mu_rect(
            v->pos.x + v->leftPanel.size+1, 
            v->pos.y+(v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H)-1, 
            v->size.width, 
            v->topPanel.size );

        mu_begin_window_ex(ctx, "topPanel", rect, MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE);
        mu_get_current_container(ctx)->rect = rect;
        v->TopPanel(v,&v->ctx);
        mu_end_window(ctx);
    }

    // BOTTOM PANEL
    if (v->BottomPanel != NULL){
        mu_Rect rect = mu_rect(
            v->pos.x + v->leftPanel.size+1, 
            v->pos.y + (v->size.height*-1) + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + v->topPanel.size +1, 
            v->size.width, 
            v->bottomPanel.size-1 );

        mu_begin_window_ex(ctx, "bottomPanel", rect, MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE);
        mu_get_current_container(ctx)->rect = rect;
        v->BottomPanel(v,&v->ctx);
        mu_end_window(ctx);
    }

    mu_end(&v->ctx);
}


/* <== Render Section ==========================================> */

static void RenderViewport(OG_Viewport *v){
    BeginTextureMode(v->renderTexture);
    ClearBackground(OG_VIEWPORT_BG_C);
    if (v->RenderUnderlay != NULL) v->RenderUnderlay(v);
    BeginMode2D(v->camera);
    if (v->Render != NULL) v->Render(v);
    EndMode2D();
    if (v->RenderOverlay != NULL) v->RenderOverlay(v);
    EndTextureMode();
}

static void DrawViewport(OG_Viewport *v){
    Vector2 pos = (Vector2){
        v->pos.x + v->leftPanel.size+1,
        v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + v->topPanel.size
    };

    DrawTextureRec(v->renderTexture.texture,v->size,pos,WHITE);

    // DRAW BORDER
    Rectangle border = (Rectangle){
        v->pos.x, v->pos.y,
        v->size.width+v->rightPanel.size+v->leftPanel.size + 1,
        (v->size.height*-1) + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + v->topPanel.size + v->bottomPanel.size
    };

    DrawRectangleLinesEx(
        border,
        OG_VIEWPORT_OUTLINE_T,
        v->isModal ? OG_MODAL_VIEWPORT_C : (v == OG.viewports.tail ? OG_VIEWPORT_TITLE_C : OG_VIEWPORT_OUTLINE_C));
    
    // DRAW TITLEBAR
    DrawRectangle(
        v->pos.x,v->pos.y, 
        v->size.width + v->rightPanel.size + v->leftPanel.size, 
        (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H), 
        v->isModal ? OG_MODAL_VIEWPORT_C : OG_VIEWPORT_TITLE_C );
    
    if (!v->noTitleBar){
        DrawTextEx(
            OG.defaultFont, 
            v->header ? v->header : v->title, 
            (Vector2){ v->pos.x+OG_VIEWPORT_OUTLINE_T, v->pos.y+OG_VIEWPORT_OUTLINE_T }, 
            (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H), 
            1, 
            OG_TEXT_C );
    }

    // DRAW RESIZE VIEWPORT GUIDELINE
    if (OG.state == OG_STATE_RESIZING_VIEWPORT && OG.targetViewport == v){
        Vector2 mousePos = GetMousePosition();
        Rectangle rect = (Rectangle){
            v->pos.x,
            v->pos.y,
            mousePos.x - v->pos.x,
            mousePos.y - v->pos.y
        };

        float minW = OG_VIEWPORT_MIN_W + (v->LeftPanel ? v->leftPanel.size:0) + (v->RightPanel ? v->rightPanel.size:0);
        if (rect.width < minW) rect.width = minW;
            
        float minH = OG_VIEWPORT_MIN_H + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + (v->TopPanel ? v->topPanel.size:0) + (v->BottomPanel ? v->bottomPanel.size:0);
        if (rect.height < minH) rect.height = minH;
        
        DrawRectangleLinesEx(
            rect,
            OG_VIEWPORT_OUTLINE_T,
            InvertColor(OG_BG_C)
        );
    }

    // DRAW RESIZE PANEL GUIDELINE
    if (OG.state == OG_STATE_RESIZING_PANEL && OG.targetViewport == v){
        Vector2 mousePos = GetMousePosition();
        float w = v->size.width + (v->RightPanel ? v->rightPanel.size:0) + (v->LeftPanel ? v->leftPanel.size:0);
        float h = (v->size.height*-1) + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + (v->TopPanel ? v->topPanel.size:0) + (v->BottomPanel ? v->bottomPanel.size:0);

        switch (OG.targetPanelType){
            case OG_RIGHT_PANEL:
                DrawLineEx(
                    (Vector2){v->pos.x + w - OG.targetPanelSizePreview, v->pos.y},
                    (Vector2){v->pos.x + w - OG.targetPanelSizePreview, v->pos.y+h},
                    OG_VIEWPORT_OUTLINE_T, 
                    InvertColor(OG_BG_C)
                );
            break;
            

            case OG_LEFT_PANEL:
                DrawLineEx(
                    (Vector2){v->pos.x + OG.targetPanelSizePreview, v->pos.y},
                    (Vector2){v->pos.x + OG.targetPanelSizePreview, v->pos.y+h},
                    OG_VIEWPORT_OUTLINE_T, 
                    InvertColor(OG_BG_C)
                );
            break;
            
            case OG_TOP_PANEL:
                DrawLineEx(
                    (Vector2){v->pos.x + (v->LeftPanel ? v->leftPanel.size:0), v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + OG.targetPanelSizePreview},
                    (Vector2){v->pos.x + (v->LeftPanel ? v->leftPanel.size:0) + v->size.width, v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + OG.targetPanelSizePreview},
                    OG_VIEWPORT_OUTLINE_T, 
                    InvertColor(OG_BG_C)
                );
            break;

            case OG_BOTTOM_PANEL:
                DrawLineEx(
                    (Vector2){v->pos.x + (v->LeftPanel ? v->leftPanel.size:0), v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + (v->TopPanel ? v->topPanel.size:0) + (v->size.height*-1) + (v->BottomPanel ? v->bottomPanel.size:0) - OG.targetPanelSizePreview},
                    (Vector2){v->pos.x + (v->LeftPanel ? v->leftPanel.size:0) + v->size.width, v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + (v->TopPanel ? v->topPanel.size:0) + (v->size.height*-1) + (v->BottomPanel ? v->bottomPanel.size:0) - OG.targetPanelSizePreview},
                    OG_VIEWPORT_OUTLINE_T, 
                    InvertColor(OG_BG_C)
                );
            break;

        }
    }

}

static void DrawViewportUI(OG_Viewport *v){
    mu_Command *cmd = NULL;
    int radius;
    while (mu_next_command(&v->ctx, &cmd)) {
        switch(cmd->type){
            case MU_COMMAND_TEXT: {
                DrawTextEx(
                    *(Font*)(cmd->text.font), 
                    cmd->text.str, 
                    (Vector2){cmd->text.pos.x,cmd->text.pos.y},
                    OG.defaultFontSize,
                    2,
                    *(Color*)&cmd->text.color
                );
                break;
            }

            case MU_COMMAND_RECT: {
                DrawRectangle(
                    cmd->rect.rect.x, 
                    cmd->rect.rect.y, 
                    cmd->rect.rect.w, 
                    cmd->rect.rect.h, 
                    *(Color*)&cmd->rect.color
                );
                break;
            }

            case MU_COMMAND_ICON: {
                Texture2D icons = OG.icons;
                int iconsWidth = icons.width/OG_ICONS_Q;
                DrawTexturePro(
                    OG.icons,
                    (Rectangle){iconsWidth*(cmd->icon.id-1),0,iconsWidth,icons.height}, 
                    (Rectangle){cmd->icon.rect.x, cmd->icon.rect.y, cmd->icon.rect.w, cmd->icon.rect.h}, 
                    (Vector2){0,0}, 
                    0.0, 
                    OG_TEXT_C
                );
                break;
            }
            
            case MU_COMMAND_CLIP: {
                mu_Rect r = cmd->clip.rect;    
                if (r.x == 0 && r.y == 0){
                    EndScissorMode();
                    break;
                }
                
                BeginScissorMode(r.x, r.y, r.w, r.h);
                break;
            }

        }
    }
}

static void DrawCommandBar(){
    if (OG.state != OG_STATE_ON_COMMAND_BAR) return;
    int screenWidth = GetScreenWidth();
    DrawRectangle(0, 0, screenWidth, OG_VIEWPORT_TITLE_H, OG_CMD_BAR_C);
    if (OG.selectedHint == 0)
        DrawRectangle(0, 0, MeasureTextEx(OG.defaultFont, OG.cmdBuf, OG.defaultFontSize, 2).x, OG_VIEWPORT_TITLE_H, OG_CMD_BAR_VIEWPORT_C);
    DrawTextEx(OG.defaultFont, OG.cmdBuf, (Vector2){0,0}, OG.defaultFontSize, 2, OG_TEXT_C);
    
    int hintsOffset = screenWidth/2;
    for (int i=1; i<OG.hintsQ; i++){
        int hintWidth = MeasureTextEx(OG.defaultFont,OG.hintsBuf[i],OG.defaultFontSize,2).x; 
        if (i == OG.selectedHint)
            DrawRectangle(hintsOffset, 0, hintWidth, OG_VIEWPORT_TITLE_H, OG_CMD_BAR_VIEWPORT_C);
        DrawTextEx(OG.defaultFont, OG.hintsBuf[i], (Vector2){hintsOffset,0}, OG.defaultFontSize, 2, OG_TEXT_C);
        hintsOffset += hintWidth + 20;
    }
}

static void DrawLogs(){
    int logsYOffset = 0;
    for (int i=OG.logsQ-1; i>=0; i--){
        logsYOffset += OG.defaultFontSize + 2;/* line spacing */
        Vector2 logPosition = (Vector2){0,GetScreenHeight()-logsYOffset};
        DrawTextEx(OG.defaultFont, OG.logs[i], logPosition, OG.defaultFontSize, 2, OG_LOGS_C);
    }
}

/* <== State Machine ===========================================> */

static void IdleState(){
    Vector2 mousePosition = GetMousePosition();
    if (OG.viewportJustSwitched && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
        OG.viewportJustSwitched = false;
    }

    OG_ChangeCursor(NULL, MOUSE_CURSOR_DEFAULT);

    for (int i=0; i<5; i++){
        OG_Viewport *v = OG.viewportsToShow[i];
        if (v != NULL){
            CleanViewportUIInput(v);
            ProcessViewportUI(v);
            if (v->hidden) OG_ToggleViewport(v);
            else OG_SetViewportOnTop(v);
            OG.viewportsToShow[i] = NULL;
            return;
        }
    }

    if (IsKeyDown(OG_MODKEY)){
        
        // COMMAND BAR TOGGLE
        if (IsKeyPressed(OG_COMMAND_BAR_KEY)){
            memset(OG.cmdBuf, '\0', 256);
            OG.cmdCursor = 0;
            OG.selectedHint = 0;
            OG.state = OG_STATE_ON_COMMAND_BAR;
            return;
        }

    }
    
    // VIEWPORTS FOCUS
    if (!OG_MouseInViewport(OG.viewports.tail,false,false,false) && !OG.viewports.tail->isModal ){
        for (OG_Viewport *v = OG.viewports.tail->prev; v != NULL; v = v->prev){ // for each viewport except the last-one
            if (v->hidden || !OG_MouseInViewport(v, false, false,false)) continue;
            if (IsViewportFullyVisible(v) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) OG.viewportJustSwitched = true;
                OG_SetViewportOnTop(v);
                break;
            }
        }
    }

    if (OG.viewports.tail->hidden) return;
    OG_Viewport *v = OG.viewports.tail;

    // VIEWPORT MOVEMENT
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        if (OG_MouseInViewport(v, true, false, false)){
            OG.state = OG_STATE_MOVING_VIEWPORT;
            OG.targetViewport = v;
            OG.grabOffset = (Vector2){mousePosition.x-OG.targetViewport->pos.x,mousePosition.y-OG.targetViewport->pos.y};
            return;
        }
    }

    // VIEWPORT RESIZE
    if (v->resizable && OG_MouseInViewport(v, false, true, false)){
        
        #if __linux__
            OG_ChangeCursor(NULL, MOUSE_CURSOR_RESIZE_ALL);
        #else
            OG_ChangeCursor(NULL, MOUSE_CURSOR_RESIZE_NWSE);
        #endif
        
    
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            OG.state = OG_STATE_RESIZING_VIEWPORT;
            OG.targetViewport = v;
            OG.grabOffset = (Vector2){
                mousePosition.x-OG.targetViewport->pos.x,
                mousePosition.y-OG.targetViewport->pos.y };
            return;
        }
    }

    // PANELS RESIZE
    Rectangle leftPanelHandle, rightPanelHandle, topPanelHandle, bottomPanelHandle;
    GetPanelsHandles(v, &leftPanelHandle, &rightPanelHandle, &topPanelHandle, &bottomPanelHandle);

    if (v->LeftPanel != NULL && v->leftPanel.resizable && IsPointOnRect(mousePosition, leftPanelHandle)){
        OG_ChangeCursor(NULL, MOUSE_CURSOR_RESIZE_EW);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            OG.targetPanelSize = &v->leftPanel.size;
            OG.targetPanelType = OG_LEFT_PANEL;
            OG.grabOffset = v->pos;
            OG.state = OG_STATE_RESIZING_PANEL;
        }
    }

    else if (v->RightPanel != NULL && v->rightPanel.resizable && IsPointOnRect(mousePosition, rightPanelHandle) && !OG_MouseInViewport(v, false, true, false)){
        OG_ChangeCursor(NULL, MOUSE_CURSOR_RESIZE_EW);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            OG.targetPanelSize = &v->rightPanel.size;
            OG.targetPanelType = OG_RIGHT_PANEL;
            OG.grabOffset = (Vector2){v->pos.x + v->leftPanel.size + v->size.width + v->rightPanel.size,0};
            OG.state = OG_STATE_RESIZING_PANEL;
        }
    }

    else if (v->TopPanel != NULL && v->topPanel.resizable && IsPointOnRect(mousePosition, topPanelHandle)){
        OG_ChangeCursor(NULL, MOUSE_CURSOR_RESIZE_NS);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            OG.targetPanelSize = &v->topPanel.size;
            OG.targetPanelType = OG_TOP_PANEL;
            OG.grabOffset = (Vector2){0,v->pos.y+(v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H)};
            OG.state = OG_STATE_RESIZING_PANEL;
        }
    }

    else if (v->BottomPanel != NULL && v->bottomPanel.resizable && IsPointOnRect(mousePosition, bottomPanelHandle)){
        OG_ChangeCursor(NULL, MOUSE_CURSOR_RESIZE_NS);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            OG.targetPanelSize = &v->bottomPanel.size;
            OG.targetPanelType = OG_BOTTOM_PANEL;
            OG.grabOffset = (Vector2){0,v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + v->topPanel.size + (v->size.height*-1) + v->bottomPanel.size};
            OG.state = OG_STATE_RESIZING_PANEL;
        }
    }

    // VIEWPORT EXIT
    if (IsKeyPressed(KEY_ESCAPE)){
        OG_ToggleViewport(v);
        return;
    }

    // VIEWPORT UPDATE
    if (OG_MouseInViewport(v, false, false, true) && v->Update != NULL && !v->updateAlways ){
        v->Update(v);
    }

    UpdateViewportUIInput(v);
    ProcessViewportUI(v);
}

static void MovingViewportState(){
    Vector2 mousePosition = GetMousePosition();
    OG.targetViewport->pos = (Vector2){mousePosition.x-OG.grabOffset.x,mousePosition.y-OG.grabOffset.y};
    
    /* Weird workaround: prevents the UI position from desyncing with the viewport */
    CleanViewportUIInput(OG.targetViewport);
    ProcessViewportUI(OG.targetViewport);
    ProcessViewportUI(OG.targetViewport);

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
        OG.state = OG_STATE_IDLE;
        return;
    }
}

static void ResizingViewportState(){
    Vector2 mousePosition = GetMousePosition();
    OG_Viewport *v = OG.targetViewport;
    float w = mousePosition.x - v->pos.x - v->rightPanel.size - v->leftPanel.size;
    float h = mousePosition.y - v->pos.y - v->topPanel.size - v->bottomPanel.size - (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H);
    
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
        OG_ResizeViewport(v, w, h);
        if (v->OnResize != NULL)
            v->OnResize(v);
        OG.state = OG_STATE_IDLE;
        return;
    }

}

static void ResizingPanelState(){
    int newSize, newViewportSize, totalNewSize, delta;
    switch (OG.targetPanelType){
        case OG_LEFT_PANEL:
        case OG_RIGHT_PANEL:
            OG_ChangeCursor(NULL, MOUSE_CURSOR_RESIZE_EW);

            if (OG.targetPanelType == OG_LEFT_PANEL) newSize = GetMousePosition().x - OG.grabOffset.x;
            else newSize = OG.grabOffset.x - GetMousePosition().x;
            if (newSize < OG_MIN_PANEL_SIZE) newSize = OG_MIN_PANEL_SIZE;
            
            delta = newSize - *OG.targetPanelSize;
            newViewportSize = OG.viewports.tail->size.width-delta;
            totalNewSize = newViewportSize + newSize;

            if (newViewportSize < OG_VIEWPORT_MIN_W){
                newViewportSize = OG_VIEWPORT_MIN_W;
                newSize = totalNewSize - newViewportSize;
            }

            OG.targetPanelSizePreview = newSize;

        break;
        
        case OG_TOP_PANEL:
        case OG_BOTTOM_PANEL:
            OG_ChangeCursor(NULL, MOUSE_CURSOR_RESIZE_NS);

            if (OG.targetPanelType == OG_TOP_PANEL) newSize = GetMousePosition().y - OG.grabOffset.y;
            else newSize = OG.grabOffset.y - GetMousePosition().y;
            if (newSize < OG_MIN_PANEL_SIZE) newSize = OG_MIN_PANEL_SIZE;
            
            delta = newSize - *OG.targetPanelSize;
            newViewportSize = (OG.viewports.tail->size.height*-1)-delta;
            totalNewSize = newViewportSize + newSize;

            if (newViewportSize < OG_VIEWPORT_MIN_H){
                newViewportSize = OG_VIEWPORT_MIN_H;
                newSize = totalNewSize - newViewportSize;
            }

            OG.targetPanelSizePreview = newSize;

        break;
    }
    
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
        switch (OG.targetPanelType){
            case OG_LEFT_PANEL:
            case OG_RIGHT_PANEL:
                *OG.targetPanelSize = newSize;
                OG_ResizeViewport(OG.viewports.tail, newViewportSize, -1);
                
            break;

            case OG_TOP_PANEL:
            case OG_BOTTOM_PANEL:
                *OG.targetPanelSize = newSize;
                OG_ResizeViewport(OG.viewports.tail, -1, newViewportSize);
                
            break;
        }

        if (OG.viewports.tail->OnResize != NULL)
            OG.viewports.tail->OnResize(OG.viewports.tail);
        
        OG.targetPanelSize = NULL;
        OG.state = OG_STATE_IDLE;
        return;
    }
}

static void OnCommandBarState(){
    // DELETE CHARACTERS
    if (OG.cmdBuf[0] != '\0' && IsKeyPressed(KEY_BACKSPACE)){
        OG.cmdBuf[OG.cmdCursor-1] = '\0';
        OG.cmdCursor--;
    }

    // CANCEL COMMAND
    if (IsKeyPressed(KEY_ESCAPE)){
        OG.state = OG_STATE_IDLE;
        return;
    }

    // ACCEPT COMMAND
    if (IsKeyPressed(KEY_ENTER)){
        OG.state = OG_STATE_IDLE;
        if (strcmp(OG.hintsBuf[OG.selectedHint], "fps") == 0){
            OG.drawFps = !OG.drawFps;
            return;
        }
        
        for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
            if (strcmp(OG.hintsBuf[OG.selectedHint],v->title) == 0){
                if (v->hidden) OG_ToggleViewport(v);
                else OG_SetViewportOnTop(v);
                return;
            }
        }
        
        OG_Viewport *top = OG.viewports.tail;
        if (!top->hidden){
            char *argv[32];
            int argc = 0;
            while (true){
                argv[argc] = strtok(argc > 0 ? NULL:OG.hintsBuf[OG.selectedHint]," ");
                if (argv[argc] == NULL) break;
                argc++;
            }
            top->ExecCmd(top,argc,argv);
        }
        
        return;
    }

    // AUTOCOMPLETE COMMAND (TAB)
    if (IsKeyPressed(KEY_TAB) && OG.selectedHint > 0){
        memset(OG.cmdBuf,'\0', sizeof(char)*256);
        strcpy(OG.cmdBuf,OG.hintsBuf[OG.selectedHint]);
        OG.selectedHint = 0;
        OG.cmdCursor = strlen(OG.cmdBuf);
    }

    // ADD CHARACTERS
    char pressedChar;
    while (true) {
        pressedChar = GetCharPressed();
        if (pressedChar == 0) break;
        OG.cmdBuf[OG.cmdCursor++] = pressedChar;
    }

    // HINTS
    memset(OG.hintsBuf+1, '\0', 256);
    OG.hintsQ = 1;
    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        if (!v->hideCmd && strstr(v->title, OG.cmdBuf) != NULL)
            OG.hintsBuf[OG.hintsQ++] = v->title;
    }

    if (!OG.viewports.tail->hidden){ //focus window hints
        OG_Viewport *v = OG.viewports.tail;
        const char **vCommands = v->GetCmds();
        int o=0;
        while (true){
            if (vCommands[o] == NULL) break;
            if (strstr(vCommands[o], OG.cmdBuf) != NULL)
                OG.hintsBuf[OG.hintsQ++] = vCommands[o];
            o++;
        }
    }

    if (IsKeyPressed(KEY_RIGHT)) OG.selectedHint++;
    if (IsKeyPressed(KEY_LEFT)) OG.selectedHint--;
    if (OG.selectedHint >= OG.hintsQ) OG.selectedHint = OG.hintsQ-1;
    if (OG.selectedHint < 0) OG.selectedHint = 0;
}

/* <== Public API ==============================================> */

void OG_ResizeViewport(OG_Viewport *v, int w, int h){
    if (w == -1) w = v->size.width;
    else if (w < OG_VIEWPORT_MIN_W) w = OG_VIEWPORT_MIN_W;
    if (h == -1) h = v->size.height;
    else if (h < OG_VIEWPORT_MIN_H) h = OG_VIEWPORT_MIN_H;
    
    if (w >= OG_VIEWPORT_MIN_W) v->size.width = w;
    if (h >= OG_VIEWPORT_MIN_H) v->size.height = h*-1;
    
    if (!v->disableOffsetUpdate){
        v->camera.offset = (Vector2){
            v->size.width/2,
            (v->size.height*-1)/2
        };
    }

    UnloadRenderTexture(v->renderTexture);
    v->renderTexture = LoadCustomRenderTexture(v->size.width, v->size.height*-1);
    
    /* Weird workaround: prevents UI panels from jittering when resized  */
    CleanViewportUIInput(v);
    ProcessViewportUI(v);
    ProcessViewportUI(v);
}

void OG_SetViewportOnTop(OG_Viewport *v){
    OG.targetViewport = v;
    if (v == OG.viewports.tail) return;
    if (OG.modalViewport != NULL){
        if (OG.modalViewport != v){
            OG_PushLog("Cant't set %s on top, a modal viewport is currently open", v->title);
            return;    
        }
    }
    
    if (v->prev != NULL) v->prev->next = v->next;
    else OG.viewports.head = v->next;
        
    v->next->prev = v->prev;
    OG.viewports.tail->next = v;
    v->prev = OG.viewports.tail;
    v->next = NULL;
    OG.viewports.tail = v;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        OG.viewportJustSwitched = true;
}

// this function sucks, I can do it better
bool OG_MouseInViewport(OG_Viewport* v, bool titleBar, bool resizeHandle, bool onlyViewport){
    Vector2 mousePosition = GetMousePosition();
    
    if (titleBar){
        Rectangle title = (Rectangle){
            v->pos.x, 
            v->pos.y, 
            v->size.width+v->rightPanel.size + v->leftPanel.size, 
            (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H)
        };
        
        if (IsPointOnRect(mousePosition, title)) return true;
        return false;
    }
    
    if (resizeHandle){
        Rectangle corner = (Rectangle){
            v->pos.x+v->size.width+v->rightPanel.size+v->leftPanel.size-OG_VIEWPORT_CORNER_S,
            v->pos.y+(v->size.height*-1)+v->topPanel.size+v->bottomPanel.size+(v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H)-OG_VIEWPORT_CORNER_S,
            OG_VIEWPORT_CORNER_S,OG_VIEWPORT_CORNER_S
        };
        if (IsPointOnRect(mousePosition, corner)) return true;
        return false;
    }

    if (onlyViewport){
        Rectangle onlyViewportArea = (Rectangle){
            v->pos.x+v->leftPanel.size,
            v->pos.y+(v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H)+v->topPanel.size,
            v->size.width,
            (v->size.height*-1)
        };
        if (IsPointOnRect(mousePosition, onlyViewportArea)) return true;
        return false;
    }

    Rectangle viewportArea = GetViewportRect(v);
    if (IsPointOnRect(mousePosition, viewportArea)) return true;
    return false;
}

float OG_GetMouseWheelMove(OG_Viewport *v){
    if (OG.viewports.tail != v) return 0.0f;
    if (!OG_MouseInViewport(v, false, false, true)) return 0.0f;
    return GetMouseWheelMove();
}

Vector2 OG_GetMouseOverlayPosition(OG_Viewport* v){
    Vector2 mousePosition = GetMousePosition();
    return (Vector2){
        mousePosition.x - (v->pos.x + OG_VIEWPORT_OUTLINE_T + v->leftPanel.size),
        mousePosition.y - (v->pos.y + (v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H) + v->topPanel.size)
    };
}

Vector2 OG_GetMouseViewportPosition(OG_Viewport* v){
    Vector2 mousePosition = GetMousePosition();
    return (Vector2){
        (mousePosition.x - (v->pos.x+v->leftPanel.size) - v->camera.offset.x)/v->camera.zoom + v->camera.target.x,
        (mousePosition.y - (v->pos.y+v->topPanel.size+(v->noTitleBar ? 0:OG_VIEWPORT_TITLE_H)) - v->camera.offset.y)/v->camera.zoom + v->camera.target.y
    };
}

bool OG_IsMouseButtonPressed(OG_Viewport *v, int button){
    if (OG.viewportJustSwitched) return false;
    if (OG.state != OG_STATE_IDLE) return false;
    if (OG.modalViewport && v != OG.modalViewport) return false;
    if (v != OG.viewports.tail) return false;
    if (!OG_MouseInViewport(v, false, false, true))
        return false;
    return IsMouseButtonPressed(button);
}

bool OG_IsMouseButtonReleased(OG_Viewport *v, int button){
    if (OG.viewportJustSwitched) return false;
    if (OG.state != OG_STATE_IDLE) return false;
    if (OG.modalViewport && v != OG.modalViewport) return false;
    if (v != OG.viewports.tail) return false;
    if (!OG_MouseInViewport(v, false, false, true))
        return false;
    return IsMouseButtonReleased(button);
}

void OG_ViewportUpdateZoom(OG_Viewport* v){
    if (OG.viewports.tail != v) return;
    if (!OG_MouseInViewport(v, false, false, true)) return;
    v->camera.zoom += GetMouseWheelMove() * OG_ZOOM_SPEED;
    if (v->camera.zoom < v->minZoom) v->camera.zoom = v->minZoom;
    if (v->camera.zoom > v->maxZoom) v->camera.zoom = v->maxZoom;
}

int OG_ViewportUpdatePan(OG_Viewport* v){
    if (OG.viewports.tail != v) return 1;
    if (!OG_MouseInViewport(v, false, false, true)) return 1;
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 d = GetMouseDelta();
        v->camera.target = (Vector2){v->camera.target.x - (d.x / v->camera.zoom),v->camera.target.y - (d.y / v->camera.zoom)};
        return 0;
    }
    return 1;
}

OG_Viewport *OG_GetViewportByName(char *name){
    if (!name) return NULL;

    OG_Viewport *v = OG.viewports.head;
    while (v != NULL) {
        if (v->title && strcmp(v->title, name) == 0)
            return v;
        v = v->next;
    }

    return NULL;
}

void OG_ToggleViewport(OG_Viewport *v){
    if (OG.modalViewport != NULL){
        if (v != OG.modalViewport && !v->isModal){
            OG_PushLog("Cant't toggle %s, a modal viewport is currently open", v->title);
            return;
        }

        OG.modalViewport = NULL;
    }
    
    v->hidden = !v->hidden;
    if (!v->hidden){
        if (v->isModal) OG.modalViewport = v;
        OG_ResizeViewport(v, -1, -1);
        OG_SetViewportOnTop(v);
    }
    else if (v == OG.viewports.tail){
        while (true){
            if (v == NULL) return;
            if (!v->hidden){
                OG_SetViewportOnTop(v);
                return;
            }
            
            v = v->prev;    
        }
    }
}

void OG_ToggleViewportByName(char *name){
    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        if (strcmp(name, v->title) == 0){
            OG_ToggleViewport(v);
            return;
        }
    }
}

void OG_OpenViewportByName(char *name){
    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        if (strcmp(name, v->title) == 0){
            for (int i=0; i<5; i++){
                if (OG.viewportsToShow[i] == NULL){
                    OG.viewportsToShow[i] = v;
                    return;
                }
            }
        }
    }
}

void OG_CloseViewportByName(char *name){
    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        if (strcmp(name, v->title) == 0){
            if (!v->hidden) OG_ToggleViewport(v);
            return;
        }
    }
}

void OG_ChangeCursor(OG_Viewport *v, MouseCursor c){
    if (v != NULL && v != OG.viewports.tail) return;
    
    if (v != NULL){
        if (!OG_MouseInViewport(v, false, false, true))
            return;
    }
    
    OG.nextCursor = c;
}

OG_Viewport *OG_InitViewport(char* title, 
                    Rectangle rect,
                    float minZoom, float maxZoom,
                    OG_PanelsDimensions panelsDimensions,
                    
                    // Flags
                    bool hideCmd,
                    bool resizable,
                    bool isModal,

                    // Main Callbacks
                    void (*Init)(OG_Viewport*), 
                    void (*Update)(OG_Viewport*), 
                    void (*OnResize)(OG_Viewport*),
                    
                    // Render callbacks
                    void (*Render)(OG_Viewport*), 
                    void (*RenderOverlay)(OG_Viewport*),
                    void (*RenderUnderlay)(OG_Viewport*),

                    // UI Callbacks
                    void (*RightPanel)(OG_Viewport*, mu_Context*), 
                    void (*LeftPanel)(OG_Viewport*, mu_Context*), 
                    void (*TopPanel)(OG_Viewport*, mu_Context*), 
                    void (*BottomPanel)(OG_Viewport*, mu_Context*), 
                    
                    // Command callbacks
                    const char** (GetCmds)(),
                    void (*ExecCmd)(OG_Viewport*, int argc, char **argv)
                    ){
    OG_Viewport *v = calloc(1, sizeof(OG_Viewport));
    
    // Meta
    v->title = title;
    v->size = (Rectangle){0,0,rect.width,rect.height};
    v->pos = (Vector2){rect.x,rect.y};
    v->minZoom = minZoom;
    v->maxZoom = maxZoom;
    v->hidden = true;
    

    // Camera
    v->camera.target = (Vector2){0,0};
    v->camera.offset = (Vector2){rect.width/2,rect.height/2};
    v->camera.rotation = 0.0f;
    v->camera.zoom = 1.0f;
    v->renderTexture = LoadCustomRenderTexture(rect.width,rect.height);
    

    // Flags
    v->hideCmd = hideCmd;
    v->resizable = resizable;
    v->isModal = isModal;
    

    // Main callbacks
    v->Init = Init;
    v->Update = Update;
    v->OnResize = OnResize;


    // Render callbacks
    v->Render = Render;
    v->RenderOverlay = RenderOverlay;
    v->RenderUnderlay = RenderUnderlay;


    // UI callbacks
    v->RightPanel = RightPanel;
    v->LeftPanel = LeftPanel;
    v->TopPanel = TopPanel;
    v->BottomPanel = BottomPanel;


    // Command callbacks
    v->GetCmds = GetCmds;
    v->ExecCmd = ExecCmd;


    // Set panels dimensions
    if (LeftPanel != NULL) v->leftPanel.size = panelsDimensions.left > OG_MIN_PANEL_SIZE ? panelsDimensions.left:OG_MIN_PANEL_SIZE;
    if (RightPanel != NULL) v->rightPanel.size = panelsDimensions.right > OG_MIN_PANEL_SIZE ? panelsDimensions.right:OG_MIN_PANEL_SIZE;
    if (TopPanel != NULL) v->topPanel.size = panelsDimensions.top > OG_MIN_PANEL_SIZE ? panelsDimensions.top:OG_MIN_PANEL_SIZE;
    if (BottomPanel != NULL) v->bottomPanel.size = panelsDimensions.bottom > OG_MIN_PANEL_SIZE ? panelsDimensions.bottom:OG_MIN_PANEL_SIZE;


    // LINK THE LIST
    if (OG.viewports.tail == NULL){
        OG.viewports.head = v;
    }

    else{
        OG.viewports.tail->next = v;
        v->prev = OG.viewports.tail;
    }

    OG.viewports.tail = v;
}

int OG_Init(char* title, int fps){
    OG_FileDialog();
    OG_InputDialog();
    OG_ContextMenu();
    
    //Raylib init
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(100, 100, title); // width and height cannot be 0 because of wasm
    MaximizeWindow();
    SetExitKey(0);
    SetTargetFPS(fps);

    const char *chars =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    " !\"#$%&'()*+,-./:;<=>∧⋀⌃?@[\\]^_`{|}~θ°"
    "│─└├"
    "⬅⮌←⟵↑";

    int count = 0;
    int *codepoints = LoadCodepoints(chars, &count);

    OG.defaultFont = LoadFontFromMemory(
        ".ttf",
        DejaVuSansMono_Bold_ttf,
        DejaVuSansMono_Bold_ttf_len,
        32,
        codepoints,
        count
    );

    UnloadCodepoints(codepoints);
    OG.defaultFontSize = OG_DEFAULT_FONT_SIZE;

    Image iconsImage = LoadImageFromMemory(".png", icons_png, icons_png_len);
    OG.icons = LoadTextureFromImage(iconsImage);
    UnloadImage(iconsImage);

    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        mu_init(&v->ctx);
        v->ctx.text_width = text_width;
        v->ctx.text_height = text_height;
        v->ctx.style->title_height = OG_VIEWPORT_TITLE_H;
        v->ctx.style->font = (void*) &(OG.defaultFont);
        v->ctx.style->colors[MU_COLOR_WINDOWBG]  = *(mu_Color*) &OG_VIEWPORT_BG_C;
        v->ctx.style->colors[MU_COLOR_TITLEBG]   = *(mu_Color*) &OG_VIEWPORT_TITLE_C;
        v->ctx.style->colors[MU_COLOR_TITLETEXT] = *(mu_Color*) &OG_TEXT_C;
        v->ctx.style->colors[MU_COLOR_BORDER]    = *(mu_Color*) &OG_VIEWPORT_OUTLINE_C;
        
        if (v->Init != NULL){
            v->Init(v);
        }
    }

    OG.hintsBuf[0] = OG.cmdBuf;

    return 0;
}

bool OG_UpdateFrame(){
    switch (OG.state){
        case OG_STATE_IDLE:               IdleState();              break;
        case OG_STATE_MOVING_VIEWPORT:    MovingViewportState();    break;
        case OG_STATE_RESIZING_VIEWPORT:  ResizingViewportState();  break;
        case OG_STATE_RESIZING_PANEL:     ResizingPanelState();     break;
        case OG_STATE_ON_COMMAND_BAR:     OnCommandBarState();      break;
    }

    // EXECUTE THE 'ALWAYS' LOGIC
    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        if (v->updateAlways) v->Update(v);
    }

    // EXECUTE LOGS LOGIC
    if (OG.logsQ > 0){
        OG.logsTimer += GetFrameTime();
        if (OG.logsTimer >= 4 /*seconds*/){
            PopLog();
        }
    }
    
    // SET CURSOR PER FRAME
    if (OG.nextCursor != OG.currentCursor){
        OG.currentCursor = OG.nextCursor;
        SetMouseCursor(OG.currentCursor);
    }

    return WindowShouldClose();
}

bool OG_RenderFrame(){
    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        if (v == OG.viewports.tail || v->renderAlways) RenderViewport(v);
    }
    
    BeginDrawing();
    ClearBackground(OG_BG_C);
    
    // FOR EACH VIEWPORT
    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        if (v->hidden) continue;
        DrawViewportUI(v);
        DrawViewport(v);
    }

    DrawCommandBar();
    DrawLogs();
    if (OG.drawFps) 
        DrawFPS(10,10);
    EndDrawing();

    return WindowShouldClose();
}

bool OG_Update(){
    OG_UpdateFrame();
    OG_RenderFrame();
    return WindowShouldClose();
}

void OG_Free(){
    CloseWindow();

    // Free resources
    UnloadFont(OG.defaultFont);
    UnloadTexture(OG.icons);
    
    // Free render textures
    for (OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        UnloadRenderTexture(v->renderTexture);
    }

    // Free logs
    for (int i = 0; i < OG.logsQ; ++i) {
        if (OG.logs[i]) free(OG.logs[i]);
    }
    OG.logsQ = 0;

    // Free viewports linked list
    for(OG_Viewport *v = OG.viewports.head; v != NULL; v = v->next){
        if (v->prev != NULL)
            free(v->prev);
    }
    
    free(OG.viewports.tail);
    OG.viewports.head = NULL;
    OG.viewports.tail = NULL;
}