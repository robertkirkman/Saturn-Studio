#ifndef SaturnImGuiChroma
#define SaturnImGuiChroma

#include "SDL2/SDL.h"

struct ImageRef {
    bool loaded;
    float scale;
    int x;
    int y;
    int width;
    int height;
};

extern bool use_imageref;
extern struct ImageRef imageref;

#ifdef __cplusplus

#include <string>
#include "saturn/libs/imgui/imgui_internal.h"

extern std::string imageref_filename;

extern bool renderFloor;
extern int currentChromaArea;
extern ImVec4 uiChromaColor;

extern "C" {
#endif
    void schroma_imgui_init(void);
    void schroma_imgui_update(void);
#ifdef __cplusplus
}
#endif

#endif