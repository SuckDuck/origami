#ifndef origami_h
#define origami_h

#define OG_DEFAULT_FONT_SIZE         13
#define OG_VIEWPORT_TITLE_H          20
#define OG_VIEWPORT_CORNER_S         12
#define OG_VIEWPORT_OUTLINE_T         1
#define OG_VIEWPORT_PANEL_HANDLE_S    5
#define OG_VIEWPORT_MIN_W           200
#define OG_VIEWPORT_MIN_H           200
#define OG_MIN_PANEL_SIZE           100
#define OG_ZOOM_SPEED              0.1f
#define OG_SCROLL_SPEED               8
#define OG_ICONS_Q                    7
#define OG_BG_C                    CLITERAL(Color){ 40, 40, 40,255}
#define OG_VIEWPORT_BG_C           CLITERAL(Color){ 55, 55, 55,255}
#define OG_VIEWPORT_TITLE_C        CLITERAL(Color){  0,121,241,255}
#define OG_MODAL_VIEWPORT_C        CLITERAL(Color){241, 60, 60,255}
#define OG_TEXT_C                  CLITERAL(Color){200,200,200,255}
#define OG_VIEWPORT_OUTLINE_C      OG_BG_C
#define OG_CMD_BAR_C               CLITERAL(Color){ 30, 30, 30,255}
#define OG_CMD_BAR_VIEWPORT_C      CLITERAL(Color){ 35, 60, 90,255}
#define OG_LOGS_C                  CLITERAL(Color){255,255,  0,255}
#define OG_MODKEY                  KEY_LEFT_SHIFT
#define OG_COMMAND_BAR_KEY         KEY_ENTER

#include <raylib.h>
#include "microui.h"

typedef struct OG_Panel{
    int size;
    bool resizable;
} OG_Panel;

typedef struct OG_Viewport{
    char* title;
    Rectangle size;
    Vector2 pos;
    mu_Context ctx;
    RenderTexture renderTexture;
    Camera2D camera;

    float minZoom;
    float maxZoom;
    
    bool hidden;
    bool hideCmd;
    bool resizable;
    bool disableOffsetUpdate;
    bool isModal;
    bool updateAlways;
    bool renderAlways;

    OG_Panel rightPanel;
    OG_Panel leftPanel;
    OG_Panel topPanel;
    OG_Panel bottomPanel;

    void (*Init)(struct OG_Viewport*);
    void (*Update)(struct OG_Viewport*);
    void (*OnResize)(struct OG_Viewport*);
    const char** (*GetCmds)();
    void (*ExecCmd)(struct OG_Viewport*, int argc, char **argv);
    void (*RightPanel)(struct OG_Viewport*, mu_Context*);
    void (*LeftPanel)(struct OG_Viewport*, mu_Context*);
    void (*TopPanel)(struct OG_Viewport*, mu_Context*);
    void (*BottomPanel)(struct OG_Viewport*, mu_Context*);
    void (*RenderUnderlay)(struct OG_Viewport*);
    void (*Render)(struct OG_Viewport*);
    void (*RenderOverlay)(struct OG_Viewport*);

    struct OG_Viewport *prev;
    struct OG_Viewport *next;
} OG_Viewport;

typedef struct OG_ViewportLinkedList{
    OG_Viewport *head;
    OG_Viewport *tail;
} OG_ViewportLinkedList;

typedef enum{
    OG_STATE_IDLE,
    OG_STATE_MOVING_VIEWPORT,
    OG_STATE_RESIZING_VIEWPORT,
    OG_STATE_RESIZING_PANEL,
    OG_STATE_ON_COMMAND_BAR
} OG_State;

typedef enum{
    OG_LEFT_PANEL,
    OG_RIGHT_PANEL,
    OG_TOP_PANEL,
    OG_BOTTOM_PANEL
} OG_PanelType;

typedef struct OG_PanelsDimensions{
    float left, right, top, bottom;
} OG_PanelsDimensions;

typedef struct OG_Context {
    OG_State state;
    mu_Context muCtx;
    OG_ViewportLinkedList viewports;
    OG_Viewport *viewportsToShow[5];
    OG_Viewport *targetViewport;
    OG_Viewport *modalViewport;
    Texture icons;
    Vector2 grabOffset;
    int *targetPanelSize;
    int targetPanelSizePreview;
    OG_PanelType targetPanelType;
    
    // Font
    Font defaultFont;
    int defaultFontSize;
    
    // Logs
    int logsQ;
    char *logs[256];
    float logsTimer;

    // Command bar
    char cmdBuf[256];
    char *hintsBuf[256];
    int cmdCursor;
    int hintsQ;
    int selectedHint;

    // Cursor
    MouseCursor currentCursor;
    MouseCursor nextCursor;

    // Flags
    bool drawFps;
    bool viewportJustSwitched;
} OG_Context;

extern OG_Context OG;

void OG_PushLog(char *format, ...);
void OG_PushLogSimple(char *log);
void OG_ResizeViewport(OG_Viewport *v, int w, int h);
void OG_SetViewportOnTop(OG_Viewport *v);
bool OG_MouseInViewport(OG_Viewport* v, bool titleBar, bool resizeHandle, bool onlyViewport);
Vector2 OG_GetMouseOverlayPosition(OG_Viewport* v);
Vector2 OG_GetMouseViewportPosition(OG_Viewport* v);
bool OG_IsMouseButtonPressed(int button);
bool OG_IsMouseButtonReleased(int button);
void OG_ViewportUpdateZoom(OG_Viewport* v);
int OG_ViewportUpdatePan(OG_Viewport* v);
void OG_ToggleViewport(OG_Viewport *v);
void OG_ToggleViewportByName(char *name);
void OG_OpenViewportByName(char *name);
void OG_CloseViewportByName(char *name);
void OG_ChangeCursor(MouseCursor c);

void OG_InitViewport(char* title, 
                    Rectangle rect,
                    float minZoom, float maxZoom,
                    OG_PanelsDimensions panelsDimensions,
                    bool hideCmd,
                    bool resizable,
                    bool isModal,
                    void (*Init)(OG_Viewport*), 
                    void (*Update)(OG_Viewport*), 
                    void (*OnResize)(OG_Viewport*),
                    void (*Render)(OG_Viewport*), 
                    void (*RenderOverlay)(OG_Viewport*),
                    void (*RenderUnderlay)(OG_Viewport*),
                    void (*RightPanel)(OG_Viewport*, mu_Context*), 
                    void (*LeftPanel)(OG_Viewport*, mu_Context*), 
                    void (*TopPanel)(OG_Viewport*, mu_Context*), 
                    void (*BottomPanel)(OG_Viewport*, mu_Context*), 
                    const char** (GetCmds)(),
                    void (*ExecCmd)(OG_Viewport*, int argc, char **argv)
                    );

int OG_Init(char* title, int fps);
bool OG_Update();
void OG_Free();

#endif