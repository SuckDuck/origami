#include <stdbool.h>
#include "origami.h"

extern void Viewport0();
extern void Viewport1();
extern void ModalViewport();

int main(int argc, char **argv){
    
    Viewport0();
    Viewport1();
    ModalViewport();
    
    OG_Init("origami_example", 60);
    while (true) {
        if (OG_Update()){
            break;
        }
    }

    OG_Free();
    return 0;
}