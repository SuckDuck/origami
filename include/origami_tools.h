#ifndef ORIGAMI_FD_H
#define ORIGAMI_FD_H

#include <stdbool.h>

typedef enum {
    OG_FD_MODE_SELECT_FILE,
    OG_FD_MODE_SELECT_DIR
} OG_FileDialogMode;

void OG_OpenFileDialog(bool (*ok_callback)(char*), OG_FileDialogMode mode, char *msg);
void OG_OpenInputDialog(void (*ok_callback)(char*));
void OG_OpenContextMenu(void (*callback)(int), int optQ, ...);
void OG_OpenContextMenuV2(void (*_callback)(int), int optQ, char **opts);
void OG_OpenContextMenuV3(void (*_callback)(char*), int optQ, char **opts);

#endif