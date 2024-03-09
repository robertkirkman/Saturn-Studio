#ifndef SaturnImGui
#define SaturnImGui

#include <SDL2/SDL.h>
#include <PR/ultratypes.h>

#ifdef __cplusplus

#include <string>
#include "saturn/saturn.h"

#define UNSTABLE if (configUnstableFeatures)

extern void imgui_update_theme();
extern void imgui_bundled_tooltip(const char*);
extern void imgui_bundled_help_marker(const char*);
extern void imgui_bundled_space(float, const char*, const char*);

enum {
    KEY_FLOAT,
    KEY_INT,
    KEY_BOOL,
    KEY_CAMERA,
    KEY_EXPRESSION,
    KEY_COLOR,
};

extern void saturn_keyframe_popout(std::string id);
extern void saturn_keyframe_popout(std::vector<std::string> id);
extern void saturn_keyframe_popout_next_line(std::string id);
extern void saturn_keyframe_popout_next_line(std::vector<std::string> id);
extern void saturn_keyframe_helper(std::string id, float* value, float max);

extern bool is_focused_on_game();
extern void saturn_imgui_open_mario_menu(int index);

extern std::string saturn_keyframe_get_mario_timeline_id(std::string base, int mario);

template <typename T>
extern void saturn_keyframe_popout(const T &edit_value, s32 data_type, std::string, std::string);

extern void saturn_keyframe_context_popout(Keyframe keyframe);
extern void saturn_keyframe_show_kf_content(Keyframe keyframe);

extern void saturn_keyframe_sort(std::vector<Keyframe>* keyframes);

extern int currentMenu;

extern bool windowCcEditor;
extern bool windowAnimPlayer;
extern bool windowChromaKey;

extern bool chromaRequireReload;

extern SDL_Window* window;

extern int endFrame;
extern int endFrameText;

extern bool splash_finished;

extern std::string editor_theme;
extern std::vector<std::pair<std::string, std::string>> theme_list;

extern bool k_context_popout_open;

extern float game_viewport[4];

extern bool request_mario_tab;

extern fs::path imgui_config_path;

extern "C" {
#endif
    bool saturn_imgui_is_capturing_transparent_video();
    bool saturn_imgui_is_capturing_video();
    void saturn_imgui_stop_capture();
    bool saturn_imgui_get_viewport(int*, int*);
    void saturn_imgui_set_frame_buffer(void* fb, bool do_capture);
    void saturn_imgui_init_backend(SDL_Window *, SDL_GLContext);
    void saturn_imgui_init();
    void saturn_imgui_handle_events(SDL_Event *);
    void saturn_imgui_update(void);
    bool saturn_disable_sm64_input();

    extern SDL_Scancode bind_to_sdl_scancode[512];
#ifdef __cplusplus
}
#endif

#endif