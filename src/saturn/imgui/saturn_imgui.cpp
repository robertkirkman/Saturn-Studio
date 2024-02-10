#include "saturn_imgui.h"

#include <ios>
#include <string>
#include <iostream>
#include <algorithm>
#include <map>
#include <fstream>

#include "game/area.h"
#include "saturn/imgui/saturn_imgui_file_browser.h"
#include "saturn/imgui/saturn_imgui_dynos.h"
#include "saturn/imgui/saturn_imgui_cc_editor.h"
#include "saturn/imgui/saturn_imgui_machinima.h"
#include "saturn/imgui/saturn_imgui_settings.h"
#include "saturn/imgui/saturn_imgui_chroma.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/libs/imgui/imgui_neo_sequencer.h"
#include "saturn/saturn.h"
#include "saturn/saturn_actors.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_textures.h"
#include "saturn/discord/saturn_discord.h"
#include "pc/controller/controller_keyboard.h"
#include "data/dynos.cpp.h"
#include "icons/IconsForkAwesome.h"
#include "icons/IconsFontAwesome5.h"
#include "saturn/filesystem/saturn_projectfile.h"
#include "saturn/saturn_json.h"
#include "saturn/saturn_video_renderer.h"

#include <SDL2/SDL.h>

#ifdef __MINGW32__
# define FOR_WINDOWS 1
#else
# define FOR_WINDOWS 0
#endif

#if FOR_WINDOWS || defined(OSX_BUILD)
# define GLEW_STATIC
# include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#ifdef USE_GLES
# include <SDL2/SDL_opengles2.h>
# define RAPI_NAME "OpenGL ES"
#else
# include <SDL2/SDL_opengl.h>
# define RAPI_NAME "OpenGL"
#endif

#if FOR_WINDOWS
#define PLATFORM "Windows"
#define PLATFORM_ICON ICON_FK_WINDOWS
#elif defined(OSX_BUILD)
#define PLATFORM "Mac OS"
#define PLATFORM_ICON ICON_FK_APPLE
#else
#define PLATFORM "Linux"
#define PLATFORM_ICON ICON_FK_LINUX
#endif

extern "C" {
#include "pc/gfx/gfx_pc.h"
#include "pc/configfile.h"
#include "game/mario.h"
#include "game/game_init.h"
#include "game/camera.h"
#include "game/level_update.h"
#include "engine/level_script.h"
#include "game/object_list_processor.h"
#include "pc/pngutils.h"
}

using namespace std;

SDL_Window* window = nullptr;

Array<PackData *> &iDynosPacks = DynOS_Gfx_GetPacks();

// Variables

int currentMenu = 0;
bool showStatusBars = true;

bool windowStats = true;
bool windowCcEditor;
bool windowAnimPlayer;
bool windowSettings;
bool windowChromaKey;

bool chromaRequireReload;

int windowStartHeight;

bool has_copy_camera;
bool copy_relative = true;

bool paste_forever;

int copiedKeyframeIndex = -1;

bool k_context_popout_open = false;
Keyframe k_context_popout_keyframe = Keyframe();
ImVec2 k_context_popout_pos = ImVec2(0, 0);

bool was_camera_frozen = false;

bool splash_finished = false;

std::string editor_theme = "moon";
std::vector<std::pair<std::string, std::string>> theme_list = {};

float game_viewport[4] = { 0, 0, -1, -1 };

bool request_mario_tab = false;

#include "saturn/saturn_timelines.h"

// Bundled Components

void imgui_bundled_tooltip(const char* text) {
    if (!configEditorShowTips) return;

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(450.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void imgui_bundled_help_marker(const char* desc) {
    if (!configEditorShowTips) {
        ImGui::TextDisabled("");
        return;
    }

    ImGui::TextDisabled(ICON_FA_QUESTION_CIRCLE);
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void imgui_bundled_space(float size, const char* title = NULL, const char* help_marker = NULL) {
    ImGui::Dummy(ImVec2(0, size/2));
    ImGui::Separator();
    if (title == NULL) {
        ImGui::Dummy(ImVec2(0, size/2));
    } else {
        ImGui::Dummy(ImVec2(0, size/4));
        ImGui::Text(title);
        if (help_marker != NULL) {
            ImGui::SameLine(); imgui_bundled_help_marker(help_marker);
        }
        ImGui::Dummy(ImVec2(0, size/4));
    }
}

void imgui_bundled_window_reset(const char* windowTitle, int width, int height, int x, int y) {
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::Selectable(ICON_FK_UNDO " Reset Window Pos")) {
            ImGui::SetWindowSize(ImGui::FindWindowByName(windowTitle), ImVec2(width, height));
            ImGui::SetWindowPos(ImGui::FindWindowByName(windowTitle), ImVec2(x, y));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

ImGuiWindowFlags imgui_bundled_window_corner(int corner, int width, int height, float alpha) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
    if (corner != -1) {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        if (width != 0) ImGui::SetNextWindowSize(ImVec2(width, height));
        window_flags |= ImGuiWindowFlags_NoMove;
        ImGui::SetNextWindowBgAlpha(alpha);
    }

    return window_flags;
}

#define JSON_VEC4(json, entry, dest) if (json.isMember(entry)) dest = ImVec4(json[entry][0].asFloat(), json[entry][1].asFloat(), json[entry][2].asFloat(), json[entry][3].asFloat())
#define JSON_BOOL(json, entry, dest) if (json.isMember(entry)) dest = json[entry].asBool()
#define JSON_FLOAT(json, entry, dest) if (json.isMember(entry)) dest = json[entry].asFloat()
#define JSON_VEC2(json, entry, dest) if (json.isMember(entry)) dest = ImVec2(json[entry][0].asFloat(), json[entry][1].asFloat())
#define JSON_DIR(json, entry, dest) if (json.isMember(entry)) dest = to_dir(json[entry].asString())

ImGuiDir to_dir(std::string dir) {
    if (dir == "up") return ImGuiDir_Up;
    if (dir == "left") return ImGuiDir_Left;
    if (dir == "down") return ImGuiDir_Down;
    if (dir == "right") return ImGuiDir_Right;
    return ImGuiDir_None;
}

void imgui_custom_theme(std::string theme_name) {
    std::filesystem::path path = std::filesystem::path("dynos/themes/" + theme_name + ".json");
    if (!std::filesystem::exists(path)) return;
    std::ifstream file = std::ifstream(path);
    Json::Value json;
    json << file;
    ImGuiStyle* style = &ImGui::GetStyle();
    style->ResetStyle();
    ImVec4* colors = style->Colors;
    if (json.isMember("colors")) {
        Json::Value colorsJson = json["colors"];
        JSON_VEC4(colorsJson, "text", colors[ImGuiCol_Text]);
        JSON_VEC4(colorsJson, "text_disabled", colors[ImGuiCol_TextDisabled]);
        JSON_VEC4(colorsJson, "window_bg", colors[ImGuiCol_WindowBg]);
        JSON_VEC4(colorsJson, "child_bg", colors[ImGuiCol_ChildBg]);
        JSON_VEC4(colorsJson, "popup_bg", colors[ImGuiCol_PopupBg]);
        JSON_VEC4(colorsJson, "border", colors[ImGuiCol_Border]);
        JSON_VEC4(colorsJson, "border_shadow", colors[ImGuiCol_BorderShadow]);
        JSON_VEC4(colorsJson, "frame_bg", colors[ImGuiCol_FrameBg]);
        JSON_VEC4(colorsJson, "frame_bg_hovered", colors[ImGuiCol_FrameBgHovered]);
        JSON_VEC4(colorsJson, "frame_bg_active", colors[ImGuiCol_FrameBgActive]);
        JSON_VEC4(colorsJson, "title_bg", colors[ImGuiCol_TitleBg]);
        JSON_VEC4(colorsJson, "title_bg_active", colors[ImGuiCol_TitleBgActive]);
        JSON_VEC4(colorsJson, "title_bg_collapsed", colors[ImGuiCol_TitleBgCollapsed]);
        JSON_VEC4(colorsJson, "menu_bar_bg", colors[ImGuiCol_MenuBarBg]);
        JSON_VEC4(colorsJson, "scrollbar_bg", colors[ImGuiCol_ScrollbarBg]);
        JSON_VEC4(colorsJson, "scrollbar_grab", colors[ImGuiCol_ScrollbarGrab]);
        JSON_VEC4(colorsJson, "scrollbar_grab_hovered", colors[ImGuiCol_ScrollbarGrabHovered]);
        JSON_VEC4(colorsJson, "scrollbar_grab_active", colors[ImGuiCol_ScrollbarGrabActive]);
        JSON_VEC4(colorsJson, "check_mark", colors[ImGuiCol_CheckMark]);
        JSON_VEC4(colorsJson, "slider_grab", colors[ImGuiCol_SliderGrab]);
        JSON_VEC4(colorsJson, "slider_grab_active", colors[ImGuiCol_SliderGrabActive]);
        JSON_VEC4(colorsJson, "button", colors[ImGuiCol_Button]);
        JSON_VEC4(colorsJson, "button_hovered", colors[ImGuiCol_ButtonHovered]);
        JSON_VEC4(colorsJson, "button_active", colors[ImGuiCol_ButtonActive]);
        JSON_VEC4(colorsJson, "header", colors[ImGuiCol_Header]);
        JSON_VEC4(colorsJson, "header_hovered", colors[ImGuiCol_HeaderHovered]);
        JSON_VEC4(colorsJson, "header_active", colors[ImGuiCol_HeaderActive]);
        JSON_VEC4(colorsJson, "separator", colors[ImGuiCol_Separator]);
        JSON_VEC4(colorsJson, "separator_hovered", colors[ImGuiCol_SeparatorHovered]);
        JSON_VEC4(colorsJson, "separator_active", colors[ImGuiCol_SeparatorActive]);
        JSON_VEC4(colorsJson, "resize_grip", colors[ImGuiCol_ResizeGrip]);
        JSON_VEC4(colorsJson, "resize_grip_hovered", colors[ImGuiCol_ResizeGripHovered]);
        JSON_VEC4(colorsJson, "resize_grip_active", colors[ImGuiCol_ResizeGripActive]);
        JSON_VEC4(colorsJson, "tab", colors[ImGuiCol_Tab]);
        JSON_VEC4(colorsJson, "tab_hovered", colors[ImGuiCol_TabHovered]);
        JSON_VEC4(colorsJson, "tab_active", colors[ImGuiCol_TabActive]);
        JSON_VEC4(colorsJson, "tab_unfocused", colors[ImGuiCol_TabUnfocused]);
        JSON_VEC4(colorsJson, "tab_unfocused_active", colors[ImGuiCol_TabUnfocusedActive]);
        JSON_VEC4(colorsJson, "plot_lines", colors[ImGuiCol_PlotLines]);
        JSON_VEC4(colorsJson, "plot_lines_hovered", colors[ImGuiCol_PlotLinesHovered]);
        JSON_VEC4(colorsJson, "plot_histogram", colors[ImGuiCol_PlotHistogram]);
        JSON_VEC4(colorsJson, "plot_histogram_hovered", colors[ImGuiCol_PlotHistogramHovered]);
        JSON_VEC4(colorsJson, "table_header_bg", colors[ImGuiCol_TableHeaderBg]);
        JSON_VEC4(colorsJson, "table_border_strong", colors[ImGuiCol_TableBorderStrong]);
        JSON_VEC4(colorsJson, "table_border_light", colors[ImGuiCol_TableBorderLight]);
        JSON_VEC4(colorsJson, "table_row_bg", colors[ImGuiCol_TableRowBg]);
        JSON_VEC4(colorsJson, "table_row_bg_alt", colors[ImGuiCol_TableRowBgAlt]);
        JSON_VEC4(colorsJson, "text_selected_bg", colors[ImGuiCol_TextSelectedBg]);
        JSON_VEC4(colorsJson, "drag_drop_target", colors[ImGuiCol_DragDropTarget]);
        JSON_VEC4(colorsJson, "nav_highlight", colors[ImGuiCol_NavHighlight]);
        JSON_VEC4(colorsJson, "nav_windowing_highlight", colors[ImGuiCol_NavWindowingHighlight]);
        JSON_VEC4(colorsJson, "nav_windowing_dim_bg", colors[ImGuiCol_NavWindowingDimBg]);
        JSON_VEC4(colorsJson, "modal_window_dim_bg", colors[ImGuiCol_ModalWindowDimBg]);
    }
    if (json.isMember("style")) {
        Json::Value styleJson = json["style"];
        JSON_BOOL(styleJson, "anti_aliased_lines", style->AntiAliasedLines);
        JSON_BOOL(styleJson, "anti_aliased_lines_use_tex", style->AntiAliasedLinesUseTex);
        JSON_BOOL(styleJson, "anti_aliased_fill", style->AntiAliasedFill);
        JSON_FLOAT(styleJson, "alpha", style->Alpha);
        JSON_FLOAT(styleJson, "disabled_alpha", style->DisabledAlpha);
        JSON_FLOAT(styleJson, "window_rounding", style->WindowRounding);
        JSON_FLOAT(styleJson, "window_border_size", style->WindowBorderSize);
        JSON_FLOAT(styleJson, "child_rounding", style->ChildRounding);
        JSON_FLOAT(styleJson, "child_border_size", style->ChildBorderSize);
        JSON_FLOAT(styleJson, "popup_rounding", style->PopupRounding);
        JSON_FLOAT(styleJson, "popup_border_size", style->PopupBorderSize);
        JSON_FLOAT(styleJson, "frame_rounding", style->FrameRounding);
        JSON_FLOAT(styleJson, "frame_border_size", style->FrameBorderSize);
        JSON_FLOAT(styleJson, "indent_spacing", style->IndentSpacing);
        JSON_FLOAT(styleJson, "columns_min_spacing", style->ColumnsMinSpacing);
        JSON_FLOAT(styleJson, "scrollbar_size", style->ScrollbarSize);
        JSON_FLOAT(styleJson, "scrollbar_rounding", style->ScrollbarRounding);
        JSON_FLOAT(styleJson, "grab_min_size", style->GrabMinSize);
        JSON_FLOAT(styleJson, "grab_rounding", style->GrabRounding);
        JSON_FLOAT(styleJson, "log_slider_deadzone", style->LogSliderDeadzone);
        JSON_FLOAT(styleJson, "tab_rounding", style->TabRounding);
        JSON_FLOAT(styleJson, "tab_border_size", style->TabBorderSize);
        JSON_FLOAT(styleJson, "tab_min_width_for_close_button", style->TabMinWidthForCloseButton);
        JSON_FLOAT(styleJson, "mouse_cursor_scale", style->MouseCursorScale);
        JSON_FLOAT(styleJson, "curve_tessellation_tol", style->CurveTessellationTol);
        JSON_FLOAT(styleJson, "circle_tessellation_max_error", style->CircleTessellationMaxError);
        JSON_VEC2(styleJson, "window_padding", style->WindowPadding);
        JSON_VEC2(styleJson, "window_min_size", style->WindowMinSize);
        JSON_VEC2(styleJson, "window_title_align", style->WindowTitleAlign);
        JSON_VEC2(styleJson, "frame_padding", style->FramePadding);
        JSON_VEC2(styleJson, "item_spacing", style->ItemSpacing);
        JSON_VEC2(styleJson, "item_inner_spacing", style->ItemInnerSpacing);
        JSON_VEC2(styleJson, "cell_padding", style->CellPadding);
        JSON_VEC2(styleJson, "touch_extra_padding", style->TouchExtraPadding);
        JSON_VEC2(styleJson, "button_text_align", style->ButtonTextAlign);
        JSON_VEC2(styleJson, "selectable_text_align", style->SelectableTextAlign);
        JSON_VEC2(styleJson, "display_window_padding", style->DisplayWindowPadding);
        JSON_VEC2(styleJson, "display_safe_area_padding", style->DisplaySafeAreaPadding);
        JSON_DIR(styleJson, "color_button_position", style->ColorButtonPosition);
        JSON_DIR(styleJson, "window_menu_button_position", style->WindowMenuButtonPosition);
    }
}

#undef JSON_VEC4
#undef JSON_BOOL
#undef JSON_FLOAT
#undef JSON_VEC2
#undef JSON_DIR

bool initFont = true;

void imgui_update_theme() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle* style = &ImGui::GetStyle();

    float SCALE = 1.f;

    if (initFont) {
        initFont = false;

        ImFontConfig defaultConfig;
        defaultConfig.SizePixels = 13.0f * SCALE;
        io.Fonts->AddFontDefault(&defaultConfig);

        ImFontConfig symbolConfig;
        symbolConfig.MergeMode = true;
        symbolConfig.SizePixels = 13.0f * SCALE;
        symbolConfig.GlyphMinAdvanceX = 13.0f * SCALE; // Use if you want to make the icon monospaced
        static const ImWchar icon_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
        io.Fonts->AddFontFromFileTTF("fonts/forkawesome-webfont.ttf", symbolConfig.SizePixels, &symbolConfig, icon_ranges);
    }

    // backwards compatibility with older theme settings
    if (configEditorTheme == 0) editor_theme = "legacy";
    else if (configEditorTheme == 1) editor_theme = "moon";
    else if (configEditorTheme == 2) editor_theme = "halflife";
    else if (configEditorTheme == 3) editor_theme = "moviemaker";
    else if (configEditorTheme == 4) editor_theme = "dear";
    else {
        for (const auto& entry : std::filesystem::directory_iterator("dynos/themes")) {
            std::filesystem::path path = entry.path();
            if (path.extension().string() != ".json") continue;
            std::string name = path.filename().string();
            name = name.substr(0, name.length() - 5);
            if (string_hash(name.c_str(), 0, name.length()) == configEditorThemeJson) {
                editor_theme = name;
                break;
            }
        }
    }
    std::cout << "Using theme: " << editor_theme << std::endl;
    configEditorTheme = -1;
    imgui_custom_theme(editor_theme);

    style->ScaleAllSizes(SCALE);
}

int selected_video_format = 0;
int videores[] = { 1920, 1080 };
bool capturing_video = false;
bool transparency_enabled = true;
bool stop_capture = false;
bool video_antialias = true;
int video_timer = 0; // used for 1 frame delays on video captures
                     // without this, its always 1 frame behind

const float ANTIALIAS_MODIFIER = 1.f;

bool saturn_imgui_get_viewport(int* width, int* height) {
    int w, h;
    if (width == nullptr) width = &w;
    if (height == nullptr) height = &h;
    if (capturing_video) {
        *width = videores[0];
        *height = videores[1];
        return true;
    }
    if (game_viewport[2] != -1 && game_viewport[3] != -1) {
        *width  = game_viewport[2] * (configWindow.enable_antialias * ANTIALIAS_MODIFIER + 1);
        *height = game_viewport[3] * (configWindow.enable_antialias * ANTIALIAS_MODIFIER + 1);
        return true;
    }
    SDL_GetWindowSize(window, width, height);
    return false;
}

void* framebuffer;

#define VIDEO_FRAME_DELAY 2

void saturn_capture_screenshot() {
    if (!capturing_video) return;
    if (video_timer-- > 0) return;
    capturing_video = false;
    int fb_size = (int)videores[0] * (int)videores[1] * 4;
    unsigned char* image = (unsigned char*)malloc(fb_size);
    unsigned char* flipped = (unsigned char*)malloc(fb_size);
    glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)framebuffer);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);
    for (int y = 0; y < videores[1]; y++) {
        for (int x = 0; x < videores[0]; x++) {
            int i = (y * videores[0] + x) * 4;
            int j = ((videores[1] - y - 1) * videores[0] + x) * 4;
            int r = 0, g = 0, b = 0, a = 0;
            if (video_antialias) {
                int pixels = 0;
                for (int X = x - 1; X <= x + 1; X++) {
                    for (int Y = y - 1; Y <= y + 1; Y++) {
                        if (X < 0 || Y < 0 || X >= videores[0] || Y >= videores[1]) continue;
                        int I = (Y * videores[0] + X) * 4;
                        r += image[I + 0];
                        g += image[I + 1];
                        b += image[I + 2];
                        a += image[I + 3];
                        pixels++;
                    }
                }
                r /= pixels; g /= pixels; b /= pixels; a /= pixels;
            }
            else {
                r = image[i + 0];
                g = image[i + 1];
                b = image[i + 2];
                a = image[i + 3];
            }
            flipped[j + 0] = r;
            flipped[j + 1] = g;
            flipped[j + 2] = b;
            flipped[j + 3] = a;
        }
    }
    if (keyframe_playing) {
        capturing_video = true;
        video_renderer_render(flipped);
        if (stop_capture) {
            capturing_video = false;
            stop_capture = false;
            video_renderer_finalize();
            keyframe_playing = false;
        }
    }
    else pngutils_write_png("screenshot.png", (int)videores[0], (int)videores[1], 4, flipped, 0);
    free(image);
    free(flipped);
}

bool saturn_imgui_is_capturing_video() {
    return capturing_video;
}

void saturn_imgui_stop_capture() {
    stop_capture = true;
}

bool saturn_imgui_is_capturing_transparent_video() {
    return capturing_video && transparency_enabled && (video_renderer_flags & VIDEO_RENDERER_FLAGS_TRANSPARECY);
}

void saturn_imgui_set_frame_buffer(void* fb, bool do_capture) {
    framebuffer = fb;
    if (do_capture) saturn_capture_screenshot();
}

// Set up ImGui

bool imgui_config_exists = false;

ImGuiID saturn_imgui_setup_dockspace() {
    ImGuiViewport* viewport = ImGui::GetWindowViewport();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(viewport->Size);
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_MenuBar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Dockspace", NULL, windowFlags);
    ImGui::PopStyleVar();
    ImGuiID dockspace = ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
    return dockspace;
}

void saturn_imgui_create_dockspace_layout(ImGuiID dockspace) {
    if (imgui_config_exists) return;
    imgui_config_exists = true;
    ImGuiID left, right, up, down;
    ImGui::DockBuilderRemoveNode(dockspace);
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace, ImGui::GetWindowViewport()->Size);
    ImGui::DockBuilderSplitNode(dockspace, ImGuiDir_Up, 0.7f, &up, &down);
    ImGui::DockBuilderSplitNode(up, ImGuiDir_Left, 0.25f, &left, &right);
    ImGui::DockBuilderDockWindow("Machinima", left);
    ImGui::DockBuilderDockWindow("Settings", left);
    ImGui::DockBuilderDockWindow("Game", right);
    ImGui::DockBuilderDockWindow("Timeline###kf_timeline", down);
}

void saturn_imgui_init_backend(SDL_Window * sdl_window, SDL_GLContext ctx) {
    window = sdl_window;

    imgui_config_exists = std::filesystem::exists("imgui.ini");

    const char* glsl_version = "#version 120";
    ImGuiContext* imgui = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui);
    ImGuiIO& io = ImGui::GetIO();
    io.WantSetMousePos = false;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    imgui_update_theme();

    ImGui_ImplSDL2_InitForOpenGL(window, ctx);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, configWindowState?"1":"0");
}

void saturn_load_themes() {
    for (const auto& entry : std::filesystem::directory_iterator("dynos/themes")) {
        std::filesystem::path path = entry.path();
        if (path.extension().string() != ".json") continue;
        std::string id = path.filename().string();
        id = id.substr(0, id.length() - 5);
        std::ifstream stream = std::ifstream(path);
        Json::Value json;
        try {
            json << stream;
        }
        catch (std::runtime_error err) {
            std::cout << "Error parsing theme " << id << std::endl;
            continue;
        }
        if (!json.isMember("name")) continue;
        std::cout << "Found theme " << json["name"].asString() << " (" << id << ")" << std::endl;
        theme_list.push_back({ id, json["name"].asString() });
    }
}

bool mario_menu_do_open = false;
int game_focus_timer = 0;
int mario_menu_index = -1;

bool is_focused_on_game() {
    return game_focus_timer++ >= 3;
}

void saturn_imgui_open_mario_menu(int index) {
    mario_menu_do_open = true;
    mario_menu_index = index;
}

bool is_ffmpeg_installed() {
    std::string path = std::string(getenv("PATH"));
#ifdef _WIN32
    char delimiter = ';';
    std::string suffix = ".exe";
#else
    char delimiter = ':';
    std::string suffix = "";
#endif
    std::vector<std::string> paths = {};
    std::string word = "";
    for (int i = 0; i < path.length(); i++) {
        if (path[i] == delimiter) {
            paths.push_back(word);
            word = "";
        }
        else word += path[i];
    }
    paths.push_back(word);
    for (std::string p : paths) {
        std::filesystem::path fsPath = std::filesystem::path(p) / ("ffmpeg" + suffix);
        if (std::filesystem::exists(fsPath)) return true;
    }
    return false;
}

bool ffmpeg_installed = true;

void saturn_imgui_init() {
    saturn_load_themes();
    sdynos_imgui_init();
    smachinima_imgui_init();
    ssettings_imgui_init();
    
    saturn_load_project_list();

    ffmpeg_installed = is_ffmpeg_installed();
    saturn_set_video_renderer(selected_video_format);
}

void saturn_imgui_handle_events(SDL_Event * event) {
    if (saturn_imgui_is_capturing_video()) return;
    ImGui_ImplSDL2_ProcessEvent(event);
    switch (event->type){
        case SDL_KEYDOWN:
            if(event->key.keysym.sym == SDLK_F3) {
                saturn_play_keyframe();
            }
            if(event->key.keysym.sym == SDLK_F4) {
                limit_fps = !limit_fps;
                configWindow.fps_changed = true;
            }

            // this is for YOU, baihsense
            // here's your crash key
            //    - bup
            if(event->key.keysym.sym == SDLK_SCROLLLOCK) {
                imgui_update_theme();
            }
            if(event->key.keysym.sym == SDLK_F9) {
                DynOS_Gfx_GetPacks().Clear();
                DynOS_Opt_Init();
                //model_details = "" + std::to_string(DynOS_Gfx_GetPacks().Count()) + " model pack";
                //if (DynOS_Gfx_GetPacks().Count() != 1) model_details += "s";
                splash_finished = false;

                if (gCurrLevelNum > 3 || !mario_exists)
                    DynOS_ReturnToMainMenu();
            }

            if(event->key.keysym.sym == SDLK_F6) {
                k_popout_open = !k_popout_open;
            }
        
        break;
    }
    smachinima_imgui_controls(event);
}

void saturn_keyframe_sort(std::vector<Keyframe>* keyframes) {
    for (int i = 0; i < keyframes->size(); i++) {
        for (int j = i + 1; j < keyframes->size(); j++) {
            if ((*keyframes)[i].position < (*keyframes)[j].position) continue;
            Keyframe temp = (*keyframes)[i];
            (*keyframes)[i] = (*keyframes)[j];
            (*keyframes)[j] = temp;
        }
    }
}

int startFrame = 0;
int endFrame = 0;
int endFrameText = 0;

void saturn_keyframe_window() {
    std::string windowLabel = "Timeline###kf_timeline";
    ImGui::Begin(windowLabel.c_str());
    if (ImGui::BeginPopupContextItem("Keyframe Menu Popup")) {
        k_context_popout_open = false;
        vector<Keyframe>* keyframes = &k_frame_keys[k_context_popout_keyframe.timelineID].second;
        int curve = -1;
        int index = -1;
        for (int i = 0; i < keyframes->size(); i++) {
            if ((*keyframes)[i].position == k_context_popout_keyframe.position) index = i;
        }
        bool isFirst = k_context_popout_keyframe.position == 0;
        if (isFirst) ImGui::BeginDisabled();
        bool doDelete = false;
        bool doCopy = false;
        if (ImGui::MenuItem("Delete")) {
            doDelete = true;
        }
        if (ImGui::MenuItem("Move to Pointer")) {
            doDelete = true;
            doCopy = true;
        }
        if (isFirst) ImGui::EndDisabled();
        if (ImGui::MenuItem("Copy to Pointer")) {
            doCopy = true;
        }
        ImGui::Separator();
        bool forceWait = k_frame_keys[k_context_popout_keyframe.timelineID].first.behavior != KFBEH_DEFAULT;
        if (forceWait) ImGui::BeginDisabled();
        for (int i = 0; i < IM_ARRAYSIZE(curveNames); i++) {
            if (ImGui::MenuItem(curveNames[i].c_str(), NULL, k_context_popout_keyframe.curve == InterpolationCurve(i))) {
                curve = i;
            }
        }
        if (forceWait) ImGui::EndDisabled();
        std::string timeline = k_context_popout_keyframe.timelineID;
        keyframes = &k_frame_keys[timeline].second;
        if (curve != -1) {
            (*keyframes)[index].curve = InterpolationCurve(curve);
            k_context_popout_open = false;
            k_previous_frame = -1;
        }
        if (doCopy) {
            int hoverKeyframeIndex = -1;
            for (int i = 0; i < keyframes->size(); i++) {
                if ((*keyframes)[i].position == k_current_frame) hoverKeyframeIndex = i;
            }
            Keyframe copy = Keyframe();
            copy.position = k_current_frame;
            copy.curve = (*keyframes)[index].curve;
            copy.value = (*keyframes)[index].value;
            copy.timelineID = (*keyframes)[index].timelineID;
            if (hoverKeyframeIndex == -1) keyframes->push_back(copy);
            else (*keyframes)[hoverKeyframeIndex] = copy;
            k_context_popout_open = false;
            k_previous_frame = -1;
        }
        if (doDelete) {
            keyframes->erase(keyframes->begin() + index);
            k_context_popout_open = false;
            k_previous_frame = -1;
        }
        saturn_keyframe_sort(keyframes);
        ImGui::EndPopup();
    }

    if (k_current_frame < 0) k_current_frame = 0;

    bool keyframe_prev_playing = keyframe_playing;
    if (keyframe_playing) { if (ImGui::Button(ICON_FK_STOP))          keyframe_playing = false; }
    else                  { if (ImGui::Button(ICON_FK_PLAY))          keyframe_playing = true;  }
                                ImGui::SameLine();
                            if (ImGui::Button(ICON_FK_FAST_BACKWARD)) k_current_frame = startFrame = 0;
                                ImGui::SameLine();
                                ImGui::Checkbox("Loop###k_t_loop", &k_loop);
                                ImGui::SameLine();
                                ImGui::PushItemWidth(48);
                                ImGui::InputInt("Frame", &k_current_frame, 0);
                                ImGui::PopItemWidth();
    if (!keyframe_playing && keyframe_prev_playing) {
        for (auto& entry : k_frame_keys) {
            saturn_keyframe_apply(entry.first, k_current_frame);
        }
    }

    ImVec2 window_size = ImGui::GetWindowSize();
            
    // Scrolling
    int scroll = keyframe_playing
        ? (k_current_frame - startFrame) >= (endFrame - startFrame) / 2 ?
            1 : 0
        : ImGui::IsWindowHovered() ?
            (int)(ImGui::GetIO().MouseWheel * -4) : 0;
    startFrame += scroll;
    if (startFrame < 0) startFrame = 0;
    endFrame = endFrameText = startFrame + window_size.x / 6;

    #define SEQUENCER { \
        ImGui::Dummy(ImVec2(0, 0)); \
        ImGui::SameLine(0.01); /* 0 doesnt work */ \
        if (ImGui::BeginNeoSequencer("Sequencer###k_sequencer", (uint32_t*)&k_current_frame, (uint32_t*)&startFrame, (uint32_t*)&endFrame, ImVec2(window_size.x, window_size.y), ImGuiNeoSequencerFlags_HideZoom)) { \
            for (auto& entry : k_frame_keys) { \
                if (entry.second.first.marioIndex != mario_index) continue; \
                std::string name = entry.second.first.name; \
                if (ImGui::BeginNeoTimeline(name.c_str(), entry.second.second)) ImGui::EndNeoTimeLine(); \
            } \
            ImGui::EndNeoSequencer(); \
        } \
    }

    int mario_index = -1;

    if (ImGui::BeginTabBar("###sequencers")) {
        if (ImGui::BeginTabItem("Environment")) {
            SEQUENCER;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Mario", nullptr, ImGuiTabItemFlags_SetSelected * request_mario_tab)) {
            mario_index = mario_menu_index;
            if (!saturn_get_actor(mario_index)) mario_index = mario_menu_index = -1;
            if (mario_index == -1) ImGui::Text("No Mario is selected");
            else SEQUENCER;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    request_mario_tab = false;

    // Keyframes
    if (!keyframe_playing) {
        for (auto& entry : k_frame_keys) {
            KeyframeTimeline timeline = entry.second.first;
            std::vector<Keyframe>* keyframes = &entry.second.second;

            if (k_previous_frame == k_current_frame && !saturn_keyframe_matches(entry.first, k_current_frame)) {
                // We create a keyframe here
                int keyframeIndex = 0;
                for (int i = 0; i < keyframes->size(); i++) {
                    if (k_current_frame >= (*keyframes)[i].position) keyframeIndex = i;
                }
                bool create_new = keyframes->size() == 0;
                InterpolationCurve curve = InterpolationCurve::WAIT;
                if (!create_new) {
                    create_new = (*keyframes)[keyframeIndex].position != k_current_frame;
                    curve = (*keyframes)[keyframeIndex].curve;
                }
                if (create_new) saturn_create_keyframe(entry.first, curve);
                else {
                    Keyframe* keyframe = &(*keyframes)[keyframeIndex];
                    void* ptr = saturn_keyframe_get_timeline_ptr(timeline);
                    if (timeline.type == KFTYPE_BOOL) (*keyframe).value[0] = *(bool*)ptr;
                    if (timeline.type == KFTYPE_FLOAT || timeline.type == KFTYPE_COLORF) {
                        float* values = (float*)ptr;
                        for (int i = 0; i < timeline.numValues; i++) {
                            (*keyframe).value[i] = *values;
                            values++;
                        }
                    }
                    if (timeline.type == KFTYPE_ANIM) {
                        AnimationState* anim_state = (AnimationState*)ptr;
                        (*keyframe).value[0] = anim_state->custom;
                        (*keyframe).value[1] = anim_state->id;
                    }
                    if (timeline.type == KFTYPE_EXPRESSION) {
                        Model* model = (Model*)ptr;
                        for (int i = 0; i < model->Expressions.size(); i++) {
                            (*keyframe).value[i] = model->Expressions[i].CurrentIndex;
                        }
                    }
                    if (timeline.type == KFTYPE_COLOR) {
                        int* values = (int*)ptr;
                        for (int i = 0; i < 6; i++) {
                            (*keyframe).value[i] = *values;
                            values++;
                        }
                    }
                    if (timeline.type == KFTYPE_SWITCH) {
                        (*keyframe).value[0] = *(int*)ptr;
                    }
                    if (timeline.behavior != KFBEH_DEFAULT) (*keyframe).curve = InterpolationCurve::WAIT;
                }
            }
            else saturn_keyframe_apply(entry.first, k_current_frame);
        }
    }

    if (k_context_popout_open) ImGui::OpenPopup("Keyframe Menu Popup");

    ImGui::End();

    // Auto focus (use controls without clicking window first)
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None) && saturn_disable_sm64_input()) {
        ImGui::SetWindowFocus(windowLabel.c_str());
    }

    k_popout_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_None);
    if (k_popout_focused) {
        for (auto& entry : k_frame_keys) {
            if (entry.second.first.name.find(", Main") != string::npos || entry.second.first.name.find(", Shade") != string::npos) {
                // Apply color keyframes
                //apply_cc_from_editor();
            }
        }
    }

    k_previous_frame = k_current_frame;
}

char saturnProjectFilename[257] = "Project";
int current_project_id;

void saturn_imgui_update() {
    if (!splash_finished) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();
    
    windowStartHeight = (showStatusBars) ? 48 : 30;

    camera_savestate_mult = 1.f;

    ImGuiID dockspace = saturn_imgui_setup_dockspace();
    if (ImGui::Begin("Machinima")) {
        windowCcEditor = false;

        if (ImGui::BeginMenu("Open Project")) {
            saturn_file_browser_filter_extension("spj");
            saturn_file_browser_scan_directory("dynos/projects");
            if (saturn_file_browser_show("project")) {
                std::string str = saturn_file_browser_get_selected().string();
                str = str.substr(0, str.length() - 4); // trim the extension
                if (str.length() <= 256) memcpy(saturnProjectFilename, str.c_str(), str.length() + 1);
                saturnProjectFilename[256] = 0;
            }
            ImGui::PushItemWidth(125);
            ImGui::InputText(".spj###project_file_input", saturnProjectFilename, 256);
            ImGui::PopItemWidth();
            if (ImGui::Button(ICON_FA_FILE " Open###project_file_open")) {
                saturn_load_project((char*)(std::string(saturnProjectFilename) + ".spj").c_str());
            }
            ImGui::SameLine(70);
            bool in_custom_level = gCurrLevelNum == LEVEL_SA && gCurrAreaIndex == 3;
            if (in_custom_level) ImGui::BeginDisabled();
            if (ImGui::Button(ICON_FA_SAVE " Save###project_file_save")) {
                saturn_save_project((char*)(std::string(saturnProjectFilename) + ".spj").c_str());
                saturn_load_project_list();
            }
            if (in_custom_level) ImGui::EndDisabled();
            ImGui::SameLine();
            imgui_bundled_help_marker("NOTE: Project files are currently EXPERIMENTAL and prone to crashing!");
            if (in_custom_level) ImGui::Text("Saving in a custom\nlevel isn't supported");
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(ICON_FA_UNDO " Load Autosaved")) {
            saturn_load_project("autosave.spj");
        }

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.f, 0.f, 1.f));
        if (ImGui::Selectable(ICON_FK_TRASH " Delete all Marios")) {
            int actors = saturn_actor_sizeof();
            for (int i = 0; i < actors; i++) {
                saturn_remove_actor(0);
            }
        }
        ImGui::PopStyleColor();

        if (ImGui::CollapsingHeader("Camera")) {
            windowCcEditor = false;

            if (camera_frozen) {
                saturn_keyframe_popout_next_line({ "k_c_camera_pos0", "k_c_camera_pos1", "k_c_camera_pos2", "k_c_camera_yaw", "k_c_camera_pitch", "k_c_camera_roll" });

                if (ImGui::BeginMenu("Options###camera_options")) {
                    camera_savestate_mult = 0.f;
                    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
                    ImGui::BeginChild("###model_metadata", ImVec2(200, 90), true, ImGuiWindowFlags_NoScrollbar);
                    ImGui::TextDisabled("pos %.f, %.f, %.f", gCamera->pos[0], gCamera->pos[1], gCamera->pos[2]);
                    ImGui::TextDisabled("foc %.f, %.f, %.f", gCamera->focus[0], gCamera->focus[1], gCamera->focus[2]);
                    if (ImGui::Button(ICON_FK_FILES_O " Copy###copy_camera")) {
                        saturn_copy_camera(copy_relative);
                        if (copy_relative) saturn_paste_camera();
                        has_copy_camera = 1;
                    } ImGui::SameLine();
                    if (!has_copy_camera) ImGui::BeginDisabled();
                    if (ImGui::Button(ICON_FK_CLIPBOARD " Paste###paste_camera")) {
                        if (has_copy_camera) saturn_paste_camera();
                    }
                    /*ImGui::Checkbox("Loop###camera_paste_forever", &paste_forever);
                    if (paste_forever) {
                        saturn_paste_camera();
                    }*/
                    if (!has_copy_camera) ImGui::EndDisabled();
                    ImGui::Checkbox("Relative to Mario###camera_copy_relative", &copy_relative);

                    ImGui::EndChild();
                    ImGui::PopStyleVar();

                    ImGui::Text(ICON_FK_VIDEO_CAMERA " Speed");
                    ImGui::PushItemWidth(150);
                    ImGui::SliderFloat("Move", &camVelSpeed, 0.0f, 2.0f);
                    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { camVelSpeed = 1.f; }
                    ImGui::SliderFloat("Rotate", &camVelRSpeed, 0.0f, 2.0f);
                    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { camVelRSpeed = 1.f; }
                    ImGui::PopItemWidth();
                    ImGui::EndMenu();
                }
            }
            ImGui::Separator();
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("FOV", &camera_fov, 0.0f, 100.0f);
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { camera_fov = 50.f; }
            imgui_bundled_tooltip("Controls the FOV of the in-game camera; Default is 50, or 40 in Project64.");
            saturn_keyframe_popout("k_fov");
            ImGui::SameLine(200); ImGui::TextDisabled("N/M");
            ImGui::PopItemWidth();
            ImGui::Checkbox("Smooth###fov_smooth", &camera_fov_smooth);

            ImGui::Separator();
            ImGui::PushItemWidth(100);
            ImGui::SliderFloat("Follow", &camera_focus, 0.0f, 1.0f);
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { camera_focus = 1.f; }
            saturn_keyframe_popout("k_focus");
            ImGui::PopItemWidth();
        }

        /*if (ImGui::CollapsingHeader("Appearance")) {
            sdynos_imgui_menu();
        }*/

        if (ImGui::CollapsingHeader("Options")) {
            imgui_machinima_quick_options();
        }

        if (ImGui::CollapsingHeader("Autochroma")) {
            if (gCurrLevelNum != LEVEL_SA) {
                if (ImGui::Checkbox("Enabled", &autoChroma)) schroma_imgui_init();
                ImGui::Separator();
            }
            if (!autoChroma && gCurrLevelNum != LEVEL_SA) ImGui::BeginDisabled();
            schroma_imgui_update();
            if (!autoChroma && gCurrLevelNum != LEVEL_SA) ImGui::EndDisabled();
        }

        if (ImGui::CollapsingHeader("Recording")) {
            if (!ffmpeg_installed) {
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
                if (ImGui::BeginChild("###no_ffmpeg", ImVec2(0, 48), true, ImGuiWindowFlags_NoScrollbar)) {
                    ImGui::Text("FFmpeg isn't installed, so some");
                    ImGui::Text("video formats aren't supported.");
                    ImGui::EndChild();
                }
                ImGui::PopStyleVar();
            }
            if (ImGui::BeginCombo("###res_preset", "Resolution Preset")) {
                if (ImGui::Selectable("240p 4:3 (N64)")) { videores[0] =  320; videores[1] =  240; }
                if (ImGui::Selectable("720p 16:9"))      { videores[0] = 1280; videores[1] =  720; }
                if (ImGui::Selectable("1080p 16:9"))     { videores[0] = 1920; videores[1] = 1080; }
                if (ImGui::Selectable("4K 16:9"))        { videores[0] = 3840; videores[1] = 2160; }
                if (ImGui::Selectable("8K 16:9"))        { videores[0] = 7680; videores[1] = 4320; }
                ImGui::EndCombo();
            }
            ImGui::InputInt2("Resolution", videores);
            std::vector<std::pair<int, std::string>> video_formats = video_renderer_get_formats();
            if (ImGui::BeginCombo("Video Format", video_formats[selected_video_format].second.c_str())) {
                for (int i = 0; i < video_formats.size(); i++) {
                    bool ffmpeg_required = video_formats[i].first & VIDEO_RENDERER_FLAGS_FFMPEG;
                    bool is_selected = selected_video_format == i;
                    if (!ffmpeg_installed && ffmpeg_required) ImGui::BeginDisabled();
                    if (ImGui::Selectable(video_formats[i].second.c_str(), is_selected)) {
                        selected_video_format = i;
                        saturn_set_video_renderer(i);
                    }
                    if (is_selected) ImGui::SetItemDefaultFocus();
                    if (!ffmpeg_installed && ffmpeg_required) ImGui::EndDisabled();
                }
                ImGui::EndCombo();
            }
            ImGui::Checkbox("Anti-aliasing", &video_antialias);
            bool transparency_supported = video_renderer_flags & VIDEO_RENDERER_FLAGS_TRANSPARECY;
            bool transparency_checkbox = transparency_enabled && transparency_supported;
            if (!transparency_supported) ImGui::BeginDisabled();
            if (ImGui::Checkbox("Transparency", &transparency_checkbox)) transparency_enabled = !transparency_enabled;
            if (!transparency_supported) ImGui::EndDisabled();
            if (!transparency_supported) {
                ImGui::Text(ICON_FK_EXCLAMATION_TRIANGLE " This video format doesn't");
                ImGui::Text("support transparency");
            }
            ImGui::Separator();
            if (ImGui::Button("Capture Screenshot (.png)")) {
                capturing_video = true;
                keyframe_playing = false;
                video_timer = VIDEO_FRAME_DELAY;
            }
            ImGui::SameLine();
            if (ImGui::Button("Render Video")) {
                capturing_video = true;
                video_timer = VIDEO_FRAME_DELAY;
                saturn_play_keyframe();
                video_renderer_init(videores[0], videores[1]);
            }
        }
    } ImGui::End();

    if (ImGui::Begin("Settings")) {
        ssettings_imgui_update();
    } ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Game")) {
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();
        window_pos.y += 20;
        window_size.y -= 20;
        game_viewport[0] = window_pos.x;
        game_viewport[1] = window_pos.y;
        game_viewport[2] = window_size.x;
        game_viewport[3] = window_size.y;
        if (ImGui::IsWindowHovered()) mouse_state.scrollwheel += ImGui::GetIO().MouseWheel;
        ImGui::Image(framebuffer, window_size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        ImGui::PopStyleVar();
        if (ImGui::BeginPopup("Mario Menu")) {
            mario_menu_do_open = false;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.f, 0.f, 1.f));
            if (ImGui::MenuItem(ICON_FK_TRASH_O " Remove")) {
                saturn_remove_actor(mario_menu_index);
            }
            ImGui::PopStyleColor();
            if (ImGui::MenuItem(ICON_FK_EYE " Look at")) {
                MarioActor* actor = saturn_get_actor(mario_menu_index);
                Vec3f mpos;
                float dist;
                s16 pitch, yaw;
                vec3f_set(mpos, actor->x, actor->y + 100, actor->z);
                vec3f_set_dist_and_angle(mpos, freezecamPos, 200, 0, actor->angle);
                vec3f_get_dist_and_angle(freezecamPos, mpos, &dist, &pitch, &yaw);
                freezecamPitch = pitch;
                freezecamYaw = yaw;
            }
            sdynos_imgui_menu(mario_menu_index);
            ImGui::EndPopup();
        }
        if (mario_menu_do_open) ImGui::OpenPopup("Mario Menu");
        if (!ImGui::IsWindowHovered()) game_focus_timer = 0;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    } ImGui::End();
    ImGui::PopStyleVar();

    saturn_keyframe_window();

    ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();

    if (showStatusBars) {
        if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags)) {
            if (ImGui::BeginMenuBar()) {
                ImGui::Text(PLATFORM_ICON);
                if (configFps60) ImGui::TextDisabled("%.1f FPS (%.3f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
                else ImGui::TextDisabled("%.1f FPS (%.3f ms/frame)", ImGui::GetIO().Framerate / 2, 1000.0f / (ImGui::GetIO().Framerate / 2));
                ImGui::Text("     ");
#ifdef GIT_BRANCH
#ifdef GIT_HASH
                ImGui::TextDisabled(ICON_FK_GITHUB " " GIT_BRANCH " " GIT_HASH);
#endif
#endif
                if (gCurrLevelNum == LEVEL_SA && gCurrAreaIndex == 3) {
                    ImGui::SameLine(ImGui::GetWindowWidth() - 280);
                    ImGui::Text("Saving in custom level isn't supported");
                }
                else {
                    ImGui::SameLine(ImGui::GetWindowWidth() - 140);
                    ImGui::Text("Autosaving in %ds", autosaveDelay / 30);
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();
        }
    }

    /*if (windowStats) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGuiWindowFlags stats_flags = imgui_bundled_window_corner(2, 0, 0, 0.64f);
        ImGui::Begin("Stats", &windowStats, stats_flags);

#ifdef DISCORDRPC
        if (has_discord_init && gCurUser.username != "") {
            if (gCurUser.username == NULL || gCurUser.username == "") {
                ImGui::Text(ICON_FK_DISCORD " Loading...");
            } else {
                std::string disc = gCurUser.discriminator;
                if (disc == "0") ImGui::Text(ICON_FK_DISCORD " @%s", gCurUser.username);
                else ImGui::Text(ICON_FK_DISCORD " %s#%s", gCurUser.username, gCurUser.discriminator);
            }
            ImGui::Separator();
        }
#endif
        ImGui::Text(ICON_FK_FOLDER_OPEN " %i model pack(s)", model_list.size());
        ImGui::Text(ICON_FK_FILE_TEXT " %i color code(s)", color_code_list.size());
        ImGui::TextDisabled(ICON_FK_PICTURE_O " %i textures loaded", preloaded_textures_count);

        ImGui::End();
        ImGui::PopStyleColor();
    }
    if (windowCcEditor && support_color_codes && current_model.ColorCodeSupport) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::Begin("Color Code Editor", &windowCcEditor);
        OpenCCEditor();
        ImGui::End();
        ImGui::PopStyleColor();

#ifdef DISCORDRPC
        discord_state = "In-Game // Editing a CC";
#endif
    }
    if (windowAnimPlayer && mario_exists) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::Begin("Animation Mixtape", &windowAnimPlayer);
        imgui_machinima_animation_player();
        ImGui::End();
        ImGui::PopStyleColor();
    }*/

    //ImGui::ShowDemoWindow();

    saturn_imgui_create_dockspace_layout(dockspace);

    is_cc_editing = windowCcEditor & support_color_codes & current_model.ColorCodeSupport;

    ImGui::Render();
    GLint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glUseProgram(0);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glUseProgram(last_program);

    /*if (was_camera_frozen && !camera_frozen && saturn_timeline_exists("k_c_camera_pos0")) {
        k_frame_keys.erase("k_c_camera_pos0");
        k_frame_keys.erase("k_c_camera_pos1");
        k_frame_keys.erase("k_c_camera_pos2");
        k_frame_keys.erase("k_c_camera_yaw");
        k_frame_keys.erase("k_c_camera_pitch");
        k_frame_keys.erase("k_c_camera_roll");
    }*/
    was_camera_frozen = camera_frozen;

    //if (!is_gameshark_open) apply_cc_from_editor();
}

/*
    ======== SATURN ========
    Advanced Keyframe Engine
    ========================
*/

std::string saturn_keyframe_get_mario_timeline_id(std::string base, int mario) {
    if (mario == -1) return base;
    char data[base.size() + 8];
    sprintf(data, "%s%c%07d", base.c_str(), '_', mario);
    return data;
}

void saturn_keyframe_context_popout(Keyframe keyframe) {
    if (keyframe_playing || k_context_popout_open) return;
    k_context_popout_open = true;
    k_context_popout_pos = ImGui::GetMousePos();
    k_context_popout_keyframe = keyframe;
}

void saturn_keyframe_show_kf_content(Keyframe keyframe) {
    ImGui::BeginTooltip();
    KeyframeTimeline timeline = k_frame_keys[keyframe.timelineID].first;
    if (timeline.type == KFTYPE_BOOL) {
        bool checked = keyframe.value[0] >= 1;
        ImGui::Checkbox("###kf_content_checkbox", &checked);
    }
    if (timeline.type == KFTYPE_FLOAT) {
        char buf[64];
        std::string fmt = timeline.precision >= 0 ? "%d" : ("%." + std::to_string(-timeline.precision) + "f");
        snprintf(buf, 64, fmt.c_str(), keyframe.value[0]);
        buf[63] = 0;
        ImGui::Text(buf);
    }
    if (timeline.type == KFTYPE_ANIM) {
        std::string anim_name;
        bool anim_custom = keyframe.value[0] >= 1;
        int anim_id = keyframe.value[1];
        if (anim_custom) anim_name = canim_array[anim_id];
        else anim_name = saturn_animations_list[anim_id];
        ImGui::Text(anim_name.c_str());
    }
    if (timeline.type == KFTYPE_EXPRESSION) {
        Model* model = (Model*)saturn_keyframe_get_timeline_ptr(timeline);
        for (int i = 0; i < keyframe.value.size(); i++) {
            if (model->Expressions[i].Textures.size() == 2 && (
                model->Expressions[i].Textures[0].FileName.find("default") != std::string::npos ||
                model->Expressions[i].Textures[1].FileName.find("default") != std::string::npos
            )) {
                int   select_index = (model->Expressions[i].Textures[0].FileName.find("default") != std::string::npos) ? 0 : 1;
                int deselect_index = (model->Expressions[i].Textures[0].FileName.find("default") != std::string::npos) ? 1 : 0;
                bool is_selected = (model->Expressions[i].CurrentIndex == select_index);
                ImGui::Checkbox((model->Expressions[i].Name + "###kf_content_expr").c_str(), &is_selected);
            }
            else {
                ImGui::Text((model->Expressions[i].Textures[model->Expressions[i].CurrentIndex].FileName + " - " + model->Expressions[i].Name).c_str());
            }
        }
    }
    if (timeline.type == KFTYPE_COLOR) {
        ImVec4 colorm;
        colorm.x = keyframe.value[0] / 255.f;
        colorm.y = keyframe.value[2] / 255.f;
        colorm.z = keyframe.value[4] / 255.f;
        colorm.w = 1;
        ImVec4 colors;
        colors.x = keyframe.value[1] / 255.f;
        colors.y = keyframe.value[3] / 255.f;
        colors.z = keyframe.value[5] / 255.f;
        colors.w = 1;
        ImGui::ColorEdit4("###kfprev_maincol",  (float*)&colorm, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit4("###kfprev_shadecol", (float*)&colors, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions);
    }
    if (timeline.type == KFTYPE_COLORF) {
        ImVec4 color;
        color.x = keyframe.value[0];
        color.y = keyframe.value[1];
        color.z = keyframe.value[2];
        color.w = 1;
        ImGui::ColorEdit4("###kfprev_color", (float*)&color, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions);
    }
    if (timeline.type == KFTYPE_SWITCH) {
        if (kf_switch_names.find(keyframe.timelineID) == kf_switch_names.end()) ImGui::Text("u forgor");
        else ImGui::Text(kf_switch_names[keyframe.timelineID][(int)keyframe.value[0]].c_str());
    }
    ImVec2 window_pos = ImGui::GetMousePos();
    ImVec2 window_size = ImGui::GetWindowSize();
    window_pos.x -= window_size.x - 4;
    window_pos.y += 4;
    ImGui::SetWindowPos(window_pos);
    ImGui::EndTooltip();
}

void saturn_create_keyframe(std::string id, InterpolationCurve curve) {
    Keyframe keyframe = Keyframe();
    keyframe.position = k_current_frame;
    keyframe.curve = curve;
    keyframe.timelineID = id;
    KeyframeTimeline timeline = k_frame_keys[id].first;
    void* ptr = saturn_keyframe_get_timeline_ptr(timeline);
    if (timeline.type == KFTYPE_BOOL) keyframe.value.push_back(*(bool*)ptr);
    if (timeline.type == KFTYPE_FLOAT || timeline.type == KFTYPE_COLORF) {
        float* values = (float*)ptr;
        for (int i = 0; i < timeline.numValues; i++) {
            keyframe.value.push_back(*values);
            values++;
        }
    }
    if (timeline.type == KFTYPE_ANIM) {
        AnimationState* anim_state = (AnimationState*)ptr;
        keyframe.value.push_back(anim_state->custom);
        keyframe.value.push_back(anim_state->id);
    }
    if (timeline.type == KFTYPE_EXPRESSION) {
        Model* model = (Model*)ptr;
        for (int i = 0; i < model->Expressions.size(); i++) {
            keyframe.value.push_back(model->Expressions[i].CurrentIndex);
        }
    }
    if (timeline.type == KFTYPE_COLOR) {
        int* values = (int*)ptr;
        for (int i = 0; i < 6; i++) {
            keyframe.value.push_back(*values);
            values++;
        }
    }
    if (timeline.type == KFTYPE_SWITCH) {
        keyframe.value.push_back(*(int*)ptr);
    }
    keyframe.curve = curve;
    k_frame_keys[id].second.push_back(keyframe);
    saturn_keyframe_sort(&k_frame_keys[id].second);
}

void saturn_keyframe_popout(std::string id) {
    ImGui::SameLine();
    saturn_keyframe_popout_next_line(id);
}

void saturn_keyframe_popout_next_line(std::string id) {
    saturn_keyframe_popout_next_line(std::vector<std::string>{ id });
}

void saturn_keyframe_popout(std::vector<std::string> ids) {
    ImGui::SameLine();
    saturn_keyframe_popout_next_line(ids);
}

void saturn_keyframe_popout_next_line(std::vector<std::string> ids) {
    if (ids.size() == 0) return;
    if (timelineDataTable.find(ids[0]) == timelineDataTable.end()) return;
    auto timeline_metadata = timelineDataTable[ids[0]];
    bool contains = saturn_timeline_exists(saturn_keyframe_get_mario_timeline_id(ids[0], get<6>(timeline_metadata) ? mario_menu_index : -1).c_str());
    std::string button_label = std::string(contains ? ICON_FK_TRASH : ICON_FK_LINK) + "###kb_" + ids[0];
    bool event_button = false;
    if (ImGui::Button(button_label.c_str())) {
        k_popout_open = true;
        for (std::string id : ids) {
            if (contains) {
                timeline_metadata = timelineDataTable[id];
                int index = get<6>(timeline_metadata) ? mario_menu_index : -1;
                k_frame_keys.erase(saturn_keyframe_get_mario_timeline_id(id, index));
            }
            else {
                auto [ptr, type, behavior, name, precision, num_values, is_mario] = timelineDataTable[id];
                KeyframeTimeline timeline = KeyframeTimeline();
                timeline.dest = ptr;
                timeline.type = type;
                timeline.behavior = behavior;
                timeline.name = name;
                timeline.precision = precision;
                timeline.numValues = num_values;
                timeline.marioIndex = is_mario ? mario_menu_index : -1;
                k_current_frame = 0;
                startFrame = 0;
                std::string timeline_id = saturn_keyframe_get_mario_timeline_id(id, is_mario ? mario_menu_index : -1);
                k_frame_keys.insert({ timeline_id, { timeline, {} } });
                saturn_create_keyframe(timeline_id, behavior == KFBEH_FORCE_WAIT ? InterpolationCurve::WAIT : InterpolationCurve::LINEAR);
            }
        }
    }
    for (std::string id : ids) {
        timeline_metadata = timelineDataTable[id];
        std::string timeline_id = saturn_keyframe_get_mario_timeline_id(id, get<6>(timeline_metadata) ? mario_menu_index : -1);
    }
    imgui_bundled_tooltip(contains ? "Remove" : "Animate");
}

bool saturn_disable_sm64_input() {
    return ImGui::GetIO().WantTextInput;
}

template <typename T>
void saturn_keyframe_popout(const T &edit_value, s32 data_type, string value_name, string id) {
    /*string buttonLabel = ICON_FK_LINK "###kb_" + id;
    string windowLabel = "Timeline###kw_" + id;

#ifdef DISCORDRPC
    discord_state = "In-Game // Keyframing";
#endif

    ImGui::SameLine();
    if (ImGui::Button(buttonLabel.c_str())) {
        if (active_key_float_value != edit_value) {
            keyframe_playing = false;
            k_last_placed_frame = 0;
            k_frame_keys = {0};
            k_v_float_keys = {0.f};

            k_c_pos1_keys = {0.f};
            k_c_pos2_keys = {0.f};
            k_c_foc0_keys = {0.f};
            k_c_foc1_keys = {0.f};
            k_c_foc2_keys = {0.f};
            k_c_rot0_keys = {0.f};
            k_c_rot1_keys = {0.f};
        }

        k_popout_open = true;
        active_key_float_value = edit_value;
        active_data_type = data_type;
    }

    if (k_popout_open && active_key_float_value == edit_value) {
        if (windowSettings) windowSettings = false;

        ImGuiWindowFlags timeline_flags = imgui_bundled_window_corner(1, 0, 0, 0.64f);
        ImGui::Begin(windowLabel.c_str(), &k_popout_open, timeline_flags);
        if (!keyframe_playing) {
            if (ImGui::Button(ICON_FK_PLAY " Play###k_t_play")) {
                saturn_play_keyframe();
            }
            ImGui::SameLine();
            ImGui::Checkbox("Loop###k_t_loop", &k_loop);
            ImGui::SameLine(180);
            if (ImGui::Button(ICON_FK_TRASH_O " Clear All###k_t_clear")) {
                k_last_placed_frame = 0;
                k_frame_keys = {0};
                k_v_float_keys = {0.f};
                k_v_int_keys = {0};
                
                k_c_pos1_keys = {0.f};
                k_c_pos2_keys = {0.f};
                k_c_foc0_keys = {0.f};
                k_c_foc1_keys = {0.f};
                k_c_foc2_keys = {0.f};
                k_c_rot0_keys = {0.f};
                k_c_rot1_keys = {0.f};
            }
        } else {
            if (ImGui::Button(ICON_FK_STOP " Stop###k_t_stop")) {
                keyframe_playing = false;
            }
            ImGui::SameLine();
            ImGui::Checkbox("Loop###k_t_loop", &k_loop);
        }

        ImGui::Separator();

        ImGui::PushItemWidth(35);
        if (ImGui::InputInt("Frames", &endFrameText, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (endFrameText >= 60) {
                endFrame = (uint32_t)endFrameText;
            } else {
                endFrame = 60;
                endFrameText = 60;
            }
        }
        ImGui::PopItemWidth();
                
        // Popout
        if (ImGui::BeginNeoSequencer("Sequencer###k_sequencer", (uint32_t*)&k_current_frame, &startFrame, &endFrame, ImVec2(endFrame * 6, 0), ImGuiNeoSequencerFlags_HideZoom)) {
            if (ImGui::BeginNeoTimeline(value_name.c_str(), k_frame_keys)) { ImGui::EndNeoTimeLine(); }
            if (data_type == KEY_CAMERA) if(ImGui::BeginNeoTimeline("Rotation", k_frame_keys)) { ImGui::EndNeoTimeLine(); }
            ImGui::EndNeoSequencer();
        }

        // UI Controls
        if (!keyframe_playing) {
            if (k_current_frame != 0) {
                if (std::find(k_frame_keys.begin(), k_frame_keys.end(), k_current_frame) != k_frame_keys.end()) {
                    // We are hovering over a keyframe
                    auto it = std::find(k_frame_keys.begin(), k_frame_keys.end(), k_current_frame);
                    if (it != k_frame_keys.end()) {
                        int key_index = it - k_frame_keys.begin();
                        if (ImGui::Button(ICON_FK_MINUS_SQUARE " Delete Keyframe###k_d_frame")) {
                            // Delete Keyframe
                            k_frame_keys.erase(k_frame_keys.begin() + key_index);
                            if (data_type == KEY_FLOAT || data_type == KEY_CAMERA) k_v_float_keys.erase(k_v_float_keys.begin() + key_index);
                            if (data_type == KEY_INT) k_v_int_keys.erase(k_v_int_keys.begin() + key_index);

                            if (data_type == KEY_CAMERA) {
                                k_c_pos1_keys.erase(k_c_pos1_keys.begin() + key_index);
                                k_c_pos2_keys.erase(k_c_pos2_keys.begin() + key_index);
                                k_c_foc0_keys.erase(k_c_foc0_keys.begin() + key_index);
                                k_c_foc1_keys.erase(k_c_foc1_keys.begin() + key_index);
                                k_c_foc2_keys.erase(k_c_foc2_keys.begin() + key_index);
                                k_c_rot0_keys.erase(k_c_rot0_keys.begin() + key_index);
                                k_c_rot1_keys.erase(k_c_rot1_keys.begin() + key_index);
                            }

                            k_last_placed_frame = k_frame_keys[k_frame_keys.size() - 1];
                        } ImGui::SameLine(); ImGui::Text("at %i", (int)k_current_frame);
                    }
                } else {
                    // No keyframe here
                    if (ImGui::Button(ICON_FK_PLUS_SQUARE " Create Keyframe###k_c_frame")) {
                        if (k_last_placed_frame > k_current_frame) {
                            
                        } else {
                            k_frame_keys.push_back(k_current_frame);
                            if (data_type == KEY_FLOAT || data_type == KEY_CAMERA) k_v_float_keys.push_back(*edit_value);
                            if (data_type == KEY_INT) k_v_int_keys.push_back(*edit_value);

                            if (data_type == KEY_CAMERA) {
                                f32 dist;
                                s16 pitch, yaw;
                                vec3f_get_dist_and_angle(gCamera->pos, gCamera->focus, &dist, &pitch, &yaw);
                                k_c_pos1_keys.push_back(gCamera->pos[1]);
                                k_c_pos2_keys.push_back(gCamera->pos[2]);
                                k_c_foc0_keys.push_back(gCamera->focus[0]);
                                k_c_foc1_keys.push_back(gCamera->focus[1]);
                                k_c_foc2_keys.push_back(gCamera->focus[2]);
                                k_c_rot0_keys.push_back(yaw);
                                k_c_rot1_keys.push_back(pitch);
                                std::cout << pitch << std::endl;
                            }
                            k_last_placed_frame = k_current_frame;
                        }
                    } ImGui::SameLine(); ImGui::Text("at %i", (int)k_current_frame);
                }
            } else {
                // On frame 0
                if (data_type == KEY_CAMERA)
                    ImGui::Text("(Setting initial value)");
            }
        }
        ImGui::End();

        // Auto focus (use controls without clicking window first)
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None) && accept_text_input == false) {
            ImGui::SetWindowFocus(windowLabel.c_str());
        }
    }*/
}
