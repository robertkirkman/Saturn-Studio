#include "saturn_imgui_machinima.h"

#include <string>
#include <iostream>
#include <algorithm>
#include <map>
#include <fstream>

#include "game/area.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/saturn.h"
#include "saturn/saturn_actors.h"
#include "saturn/saturn_textures.h"
#include "saturn/saturn_animation_ids.h"
#include "saturn/saturn_animations.h"
#include "saturn/saturn_obj_def.h"
#include "saturn/imgui/saturn_imgui_dynos.h"
#include "saturn_imgui.h"
#include "saturn/imgui/saturn_imgui_file_browser.h"
#include "saturn/imgui/saturn_imgui_chroma.h"
#include "saturn/filesystem/saturn_locationfile.h"
#include "saturn/filesystem/saturn_animfile.h"
#include "pc/controller/controller_keyboard.h"
#include <SDL2/SDL.h>

#include "icons/IconsForkAwesome.h"

#include "data/dynos.cpp.h"

extern "C" {
#include "sm64.h"
#include "pc/gfx/gfx_pc.h"
#include "pc/configfile.h"
#include "pc/cheats.h"
#include "game/mario.h"
#include "game/camera.h"
#include "game/level_update.h"
#include "engine/level_script.h"
#include "game/game_init.h"
#include "src/game/envfx_snow.h"
#include "src/game/interaction.h"
#include "include/behavior_data.h"
#include "game/object_helpers.h"
#include "game/custom_level.h"
}

#include "saturn/saturn_json.h"

using namespace std;

int custom_anim_index = -1;
int current_sanim_index = 7;
std::string current_sanim_name = "RUNNING";
int current_sanim_id = MARIO_ANIM_RUNNING;
std::map<std::pair<int, std::string>, int> current_anim_map = sanim_movement;
std::string anim_preview_name = "RUNNING";
int current_sanim_group_index = 0;
int current_slevel_index = 1;
Vec3f obj_pos;
int obj_rot[3];
int obj_beh_params[4];
int obj_model;
int obj_beh;

float gravity = 1;
int time_freeze_state = 0;

int current_location_index = 0;
char location_name[256];

float custom_level_scale = 100.f;
bool is_custom_level_loaded = false;
std::string custom_level_path;
std::string custom_level_filename;
std::string custom_level_dirname;

s16 levelList[] = { 
    LEVEL_SA, LEVEL_CASTLE_GROUNDS, LEVEL_CASTLE, LEVEL_CASTLE_COURTYARD, LEVEL_BOB, 
    LEVEL_WF, LEVEL_PSS, LEVEL_TOTWC, LEVEL_JRB, LEVEL_CCM,
    LEVEL_BITDW, LEVEL_BBH, LEVEL_HMC, LEVEL_COTMC, LEVEL_LLL,
    LEVEL_SSL, LEVEL_VCUTM, LEVEL_DDD, LEVEL_BITFS, 
    LEVEL_SL, LEVEL_WDW, LEVEL_TTM, LEVEL_THI,
    LEVEL_TTC, LEVEL_WMOTR, LEVEL_RR, LEVEL_BITS
};
int areaList[] = {
    2, 1, 3, 1, 1,
    1, 1, 1, 2, 2,
    1, 1, 1, 1, 2,
    2, 1, 2, 1,
    2, 2, 2, 2,
    1, 1, 1, 1
};

int current_level_sel = 0;
void warp_to(s16 destLevel, s16 destArea = 0x01, s16 destWarpNode = 0x0A) {
    if (!mario_exists)
        return;

    if (destLevel == gCurrLevelNum && destArea == gCurrAreaIndex) {
        if (current_slevel_index < 4)
            return;
            
        DynOS_Warp_ToLevel(gCurrLevelNum, gCurrAreaIndex, gCurrActNum);
    }

    initiate_warp(destLevel, destArea, destWarpNode, 0);
    fade_into_special_warp(0,0);
}

void anim_play_button() {
    if (current_animation.id == -1) return;
    current_anim_frame = 0;
    is_anim_playing = true;
    if (current_animation.custom) {
        saturn_read_mcomp_animation(nullptr, anim_preview_name);
        saturn_play_animation(MarioAnimID(current_animation.id));
        saturn_play_custom_animation();
    } else {
        saturn_play_animation(MarioAnimID(current_animation.id));
    }
}

void saturn_create_object(int model, const BehaviorScript* behavior, float x, float y, float z, s16 pitch, s16 yaw, s16 roll, int behParams) {
    Object* obj = spawn_object(gMarioState->marioObj, model, behavior);
    obj->oPosX = x;
    obj->oPosY = y;
    obj->oPosZ = z;
    obj->oHomeX = x;
    obj->oHomeY = y;
    obj->oHomeZ = z;
    obj->oFaceAnglePitch = pitch;
    obj->oFaceAngleYaw = yaw;
    obj->oFaceAngleRoll = roll;
    obj->oBehParams = behParams;
}

void smachinima_imgui_controls(SDL_Event * event) {
    switch (event->type){
        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_m && !saturn_disable_sm64_input()) {
                if (camera_fov <= 98.0f) camera_fov += 2.f;
            } else if (event->key.keysym.sym == SDLK_n && !saturn_disable_sm64_input()) {
                if (camera_fov >= 2.0f) camera_fov -= 2.f;
            }

            /*if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LSHIFT]) {
                if (!saturn_disable_sm64_input()) {
                    if (event->key.keysym.sym >= SDLK_0 && event->key.keysym.sym <= SDLK_9) {
                        saturn_load_expression_number(event->key.keysym.sym);
                    }
                };
            }*/

        case SDL_MOUSEMOTION:
            SDL_Delay(2);
            camera_view_move_x = event->motion.xrel;
            camera_view_move_y = event->motion.yrel;
        
        break;
    }
}

void warp_to_level(int level, int area, int act = -1) {
    s32 levelID = levelList[level];
    s32 warpnode = 0x0A;

    if (gCurrLevelNum == levelID && gCurrAreaIndex == area) return;

    is_anim_playing = false;
    is_anim_paused = false;
    time_freeze_state = 0;

    if (level != 0) enable_shadows = true;
    else enable_shadows = false;

    switch (level) {
        case 1:
            warpnode = 0x04;
            break;
        case 2:
            if (area == 1) warpnode = 0x1E;
            else warpnode = 0x10;
            break;
        case 3:
            warpnode = 0x0B;
            break;
    }

    DynOS_Warp_ToWarpNode(levelID, area, act, warpnode);

    saturn_clear_actors();
}

int get_saturn_level_id(int level) {
    for (int i = 0; i < IM_ARRAYSIZE(levelList); i++) {
        if (levelList[i] == level) return i;
    }
}

std::vector<std::string> split(std::string input, char character) {
    std::vector<std::string> tokens = {};
    std::string token = "";
    for (int i = 0; i < input.length(); i++) {
        if (input[i] == '\r') continue;
        if (input[i] == character) {
            tokens.push_back(token);
            token = "";
        }
        else token += input[i];
    }
    tokens.push_back(token);
    return tokens;
}
std::vector<std::vector<std::string>> tokenize(std::string input) {
    std::vector<std::vector<std::string>> tokens = {};
    auto lines = split(input, '\n');
    for (auto line : lines) {
        tokens.push_back(split(line, ' '));
    }
    return tokens;
}

int textureIndex = 0;
fs::path customlvl_texdir = fs::path(sys_user_path()) / "res" / "gfx" / "customlevel";
bool custom_level_flip_normals = false;

void parse_materials(char* data, std::map<std::string, fs::path>* materials) {
    auto tokens = tokenize(std::string(data));
    std::string matname = "";
    for (auto line : tokens) {
        if (line[0] == "newmtl") matname = line[1];
        if (line[0] == "map_Kd" && matname != "") {
            std::string path = std::to_string(textureIndex++) + ".png";
            fs::path raw = fs::path(line[1]);
            fs::path src = raw.is_absolute() ? raw : fs::path(custom_level_path).parent_path() / raw;
            fs::path dst = customlvl_texdir / path;
            fs::remove(dst);
            fs::copy_file(src, dst);
            materials->insert({ matname, "customlevel/" + path });
        }
    }
}

void parse_custom_level(char* data) {
    auto tokens = tokenize(std::string(data));
    textureIndex = 0;
    if (fs::exists(customlvl_texdir)) fs::remove_all(customlvl_texdir);
    fs::create_directories(customlvl_texdir);
    custom_level_new();
    std::vector<std::array<float, 3>> vertices = {};
    std::vector<std::array<float, 2>> uv = {};
    std::map<std::string, fs::path> materials = {};
    for (auto line : tokens) {
        if (line.size() == 0) continue;
        if (line[0] == "mtllib") {
            fs::path path = fs::absolute(fs::path(custom_level_dirname) / line[1]);
            if (!fs::exists(path)) continue;
            auto size = fs::file_size(path);
            char* mtldata = (char*)malloc(size);
            std::ifstream file = std::ifstream(path, std::ios::binary);
            file.read(mtldata, size);
            parse_materials(mtldata, &materials);
            free(mtldata);
        }
        if (line[0] == "v") vertices.push_back({ std::stof(line[1]), std::stof(line[2]), std::stof(line[3]) });
        if (line[0] == "vt") uv.push_back({ std::stof(line[1]), std::stof(line[2]) });
        if (line[0] == "usemtl") {
            if (materials.find(line[1]) == materials.end()) continue; 
            custom_level_texture((char*)materials[line[1]].c_str());
        }
        if (line[0] == "f") {
            for (int i = 1; i < line.size(); i++) {
                int idx = i;
                if      (custom_level_flip_normals && idx == 1) idx = 3;
                else if (custom_level_flip_normals && idx == 3) idx = 1;
                auto indexes = split(line[idx], '/');
                int v = std::stoi(indexes[0]) - 1;
                int vt = std::stoi(indexes[1]) - 1;
                custom_level_vertex(vertices[v][0] * custom_level_scale, vertices[v][1] * custom_level_scale, vertices[v][2] * custom_level_scale, uv[vt][0] * 1024, uv[vt][1] * 1024);
            }
            custom_level_face();
        }
    }
    gfx_precache_textures();
    custom_level_finish();
}

void smachinima_imgui_init() {
    Cheats.EnableCheats = true;
    Cheats.GodMode = true;
    Cheats.ExitAnywhere = true;
    current_anim_dir_path = std::string(sys_user_path()) + "/dynos/anims/";
}

bool enabled_acts[6];
int current_warp_area = 1;

void imgui_machinima_quick_options() {
    if (ImGui::MenuItem(ICON_FK_CLOCK_O " Limit FPS",      "F4", limit_fps)) {
        limit_fps = !limit_fps;
        configWindow.fps_changed = true;
    }

    if (mario_exists) {
        ImGui::Separator();

        if (ImGui::BeginCombo("###warp_to_level", saturn_get_stage_name(levelList[current_slevel_index]), ImGuiComboFlags_None)) {
            for (int n = 0; n < IM_ARRAYSIZE(levelList); n++) {
                const bool is_selected = (current_slevel_index == n);

                if (ImGui::Selectable(saturn_get_stage_name(levelList[n]), is_selected)) {
                    current_slevel_index = n;
                    current_warp_area = 1;
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::BeginCombo("###level_area", ("Area " + std::to_string(current_warp_area)).c_str())) {
            for (int i = 1; i <= areaList[current_slevel_index]; i++) {
                bool is_selected = (current_warp_area == i);
                if (ImGui::Selectable(("Area " + std::to_string(i)).c_str())) {
                    current_warp_area = i;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Warp to Level")) {
            autoChroma = false;

            warp_to_level(current_slevel_index, current_warp_area, 1);
            // Erase existing timelines
            k_frame_keys.clear();
        }
        for (int i = 0; i < 6; i++) {
            ImGui::SameLine();
            if (ImGui::Button((std::string(enabled_acts[i] ? ICON_FK_STAR : ICON_FK_STAR_O) + "###act_select_" + std::to_string(i)).c_str())) {
                enabled_acts[i] = !enabled_acts[i];
            }
            imgui_bundled_tooltip((std::string(enabled_acts[i] ? "Disable" : "Enable") + " act " + std::to_string(i + 1) + " objects").c_str());
        }

        auto locations = saturn_get_locations();
        bool do_save = false;
        std::vector<std::string> forRemoval = {};
        if (ImGui::BeginMenu("Locations")) {
            for (auto& entry : *locations) {
                if (ImGui::Button((std::string(ICON_FK_PLAY "###warp_to_location_") + entry.first).c_str())) {
                    gMarioState->pos[0] = entry.second.second[0];
                    gMarioState->pos[1] = entry.second.second[1];
                    gMarioState->pos[2] = entry.second.second[2];
                    gMarioState->faceAngle[1] = entry.second.first;
                }
                imgui_bundled_tooltip("Warp");
                ImGui::SameLine();
                if (ImGui::Button((std::string(ICON_FK_TRASH "###delete_location_") + entry.first).c_str())) {
                    forRemoval.push_back(entry.first);
                    do_save = true;
                }
                imgui_bundled_tooltip("Remove");
                ImGui::SameLine();
                ImGui::Text(entry.first.c_str());
            }
            if (ImGui::Button(ICON_FK_PLUS "###add_location")) {
                saturn_add_location(location_name);
                do_save = true;
            }
            imgui_bundled_tooltip("Add");
            ImGui::SameLine();
            ImGui::InputText("###location_input", location_name, 256);
            ImGui::EndMenu();
        }
        for (std::string key : forRemoval) {
            locations->erase(key);
        }
        if (do_save) saturn_save_locations();
    }
    ImGui::Separator();
    ImGui::Checkbox("HUD", &configHUD);
    imgui_bundled_tooltip("Controls the in-game HUD visibility.");
    saturn_keyframe_popout("k_hud");
    ImGui::Checkbox("Shadows", &enable_shadows);
    imgui_bundled_tooltip("Displays the shadows of various objects.");
    saturn_keyframe_popout("k_shadows");
    ImGui::Checkbox("Invulnerability", (bool*)&enable_immunity);
    imgui_bundled_tooltip("If enabled, Mario will be invulnerable to most enemies and hazards.");
    ImGui::Checkbox("Fog", &enable_fog);
    imgui_bundled_tooltip("Toggles the fog, useful for near-clipping shots");
    int previous_time_freeze_state = time_freeze_state;
    ImGui::PushItemWidth(150);
    ImGui::Combo("Time Freeze", &time_freeze_state, "Unfrozen\0Mario-exclusive\0Everything\0");
    if (previous_time_freeze_state != time_freeze_state) {
        if (previous_time_freeze_state == 1) disable_time_stop();
        if (previous_time_freeze_state == 2) disable_time_stop_including_mario();
        if (time_freeze_state == 1) enable_time_stop();
        if (time_freeze_state == 2) enable_time_stop_including_mario();
    }
    imgui_bundled_tooltip("Pauses all in-game movement, excluding the camera.");
    ImGui::Checkbox("Object Interactions", (bool*)&enable_dialogue);
    imgui_bundled_tooltip("Toggles interactions with some objects; This includes opening/closing doors, triggering dialogue when interacting with an NPC or readable sign, etc.");
    if (mario_exists) {
        if (gMarioState->action == ACT_IDLE) {
            if (ImGui::Button("Sleep")) {
                set_mario_action(gMarioState, ACT_START_SLEEPING, 0);
            }
        }
        ImGui::Separator();
        const char* mEnvSettings[] = { "Default", "None", "Snow", "Blizzard" };
        ImGui::PushItemWidth(100);
        ImGui::Combo("Environment###env_dropdown", (int*)&gLevelEnv, mEnvSettings, IM_ARRAYSIZE(mEnvSettings));
        ImGui::SliderFloat("Gravity", &gravity, 0.f, 3.f);
        saturn_keyframe_popout("k_gravity");
        ImGui::PopItemWidth();
        
        if (ImGui::BeginMenu("Spawn Object")) {
            ImGui::Text("Position");
            ImGui::SameLine();
            ImGui::InputFloat3("###obj_set_pos", (float*)&obj_pos);
            ImGui::Text("Rotation");
            ImGui::SameLine();
            ImGui::InputInt3("###obj_set_rot", (int*)&obj_rot);
            if (ImGui::Button("Copy Mario")) {
                vec3f_copy(obj_pos, gMarioState->pos);
                obj_rot[0] = gMarioState->faceAngle[0];
                obj_rot[1] = gMarioState->faceAngle[1];
                obj_rot[2] = gMarioState->faceAngle[2];
            }
            ImGui::Separator();
            ImGui::Text("Model");
            if (ImGui::BeginCombo("###obj_model", obj_models[obj_model].first.c_str())) {
                for (int i = 0; i < IM_ARRAYSIZE(obj_models); i++) {
                    bool selected = obj_model == i;
                    if (ImGui::Selectable(obj_models[i].first.c_str())) obj_model = i;
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::Text("Behavior");
            if (ImGui::BeginCombo("###obj_beh", obj_behaviors[obj_beh].first.c_str())) {
                for (int i = 0; i < IM_ARRAYSIZE(obj_behaviors); i++) {
                    bool selected = obj_beh == i;
                    if (ImGui::Selectable(obj_behaviors[i].first.c_str())) obj_beh = i;
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::Text("Behavior Parameters");
            ImGui::InputInt4("###obj_beh_params", obj_beh_params);
            ImGui::Separator();
            if (ImGui::Button("Spawn Object")) {
                saturn_create_object(
                    obj_models[obj_model].second, obj_behaviors[obj_beh].second,
                    obj_pos[0], obj_pos[1], obj_pos[2],
                    obj_rot[0], obj_rot[1], obj_rot[2],
                    ((obj_beh_params[0] & 0xFF) << 24) | ((obj_beh_params[1] & 0xFF) << 16) | ((obj_beh_params[2] & 0xFF) << 8) | (obj_beh_params[3] & 0xFF)
                );
            }
            ImGui::EndMenu();
        }
    }

    if (ImGui::BeginMenu("Shading")) {
        ImGui::SliderFloat("X###wdir_x", &world_light_dir1, -2.f, 2.f);
        saturn_keyframe_popout("k_shade_x");
        ImGui::SliderFloat("Y###wdir_y", &world_light_dir2, -2.f, 2.f);
        saturn_keyframe_popout("k_shade_y");
        ImGui::SliderFloat("Z###wdir_z", &world_light_dir3, -2.f, 2.f);
        saturn_keyframe_popout("k_shade_z");
        ImGui::SliderFloat("Tex###wdir_tex", &world_light_dir4, 1.f, 4.f);
        saturn_keyframe_popout("k_shade_t");

        ImGui::ColorEdit4("Col###wlight_col", gLightingColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions);
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            ImGui::OpenPopup("###texColColorPresets");

        if (ImGui::BeginPopup("###texColColorPresets")) {
            if (ImGui::Selectable(ICON_FK_UNDO " Reset")) {
                gLightingColor[0] = 1.f;
                gLightingColor[1] = 1.f;
                gLightingColor[2] = 1.f;
            }
            if (ImGui::Selectable("Randomize")) {
                gLightingColor[0] = (rand() % 255) / 255.0f;
                gLightingColor[1] = (rand() % 255) / 255.0f;
                gLightingColor[2] = (rand() % 255) / 255.0f;
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine(); ImGui::Text("Col");
        saturn_keyframe_popout("k_light_col");

        if (world_light_dir1 != 0.f || world_light_dir2 != 0.f || world_light_dir3 != 0.f || world_light_dir4 != 1.f) {
            if (ImGui::Button("Reset###reset_wshading")) {
                world_light_dir1 = 0.f;
                world_light_dir2 = 0.f;
                world_light_dir3 = 0.f;
                world_light_dir4 = 1.f;
                gLightingColor[0] = 1.f;
                gLightingColor[1] = 1.f;
                gLightingColor[2] = 1.f;
            }
        }

        ImGui::EndMenu();
    }

    UNSTABLE
    if (ImGui::BeginMenu("(!) Custom Level")) {
        bool in_custom_level = gCurrLevelNum == LEVEL_SA && gCurrAreaIndex == 3;
        ImGui::PushItemWidth(80);
        ImGui::InputFloat("Scale###cl_scale", &custom_level_scale);
        ImGui::PopItemWidth();
        if (!is_custom_level_loaded || in_custom_level) ImGui::BeginDisabled();
        if (ImGui::Button("Load Level")) {
            auto size = fs::file_size(custom_level_path);
            char* data = (char*)malloc(size);
            std::ifstream file = std::ifstream((char*)custom_level_path.c_str(), std::ios::binary);
            file.read(data, size);
            parse_custom_level(data);
            free(data);
            warp_to_level(0, 3);
        }
        if (!is_custom_level_loaded || in_custom_level) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Load .obj")) {
            auto selection = choose_file_dialog("Select a model", { "Wavefront Model (.obj)", "*.obj", "All Files", "*" }, false);
            if (selection.size() != 0) {
                fs::path path = selection[0];
                is_custom_level_loaded = true;
                custom_level_path = path.string();
                custom_level_dirname = path.parent_path().string();
                custom_level_filename = path.filename().string();
            }
        }
        ImGui::Text(is_custom_level_loaded ? custom_level_filename.c_str() : "No model loaded!");
        ImGui::EndMenu();
    }
}

static char animSearchTerm[128];
static bool selected_groups[8] = { true, true, true, true, true, true, true, true };

bool case_insensitive_contains(std::string base, std::string substr) {
    std::string lower_b = base;
    std::string lower_s = substr;
    std::transform(base.begin(), base.end(), lower_b.begin(),
        [](unsigned char c){ return std::tolower(c); });
    std::transform(substr.begin(), substr.end(), lower_s.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return lower_b.find(lower_s) != std::string::npos;
}

std::vector<int> get_sorted_anim_list() {
    std::vector<int> anim_list = {};
    std::vector<int> fav_anim_list = {};
    for (int i = 0; i < 8; i++) {
        if (!selected_groups[i]) continue;
        for (const auto& entry : sanim_maps[i]) {
            if (!case_insensitive_contains(entry.first.second, animSearchTerm)) continue;
            bool contains = std::find(favorite_anims.begin(), favorite_anims.end(), entry.second) != favorite_anims.end();
            if (contains) fav_anim_list.push_back(entry.second);
            else anim_list.push_back(entry.second);
        }
    }
    std::reverse(fav_anim_list.begin(), fav_anim_list.end());
    for (int fav : fav_anim_list) {
        anim_list.insert(anim_list.begin(), fav);
    }
    return anim_list;
}

struct Animation sampling_animation;
float sampling_frame = 0;
bool sampling_anim_loaded = false;
std::vector<s16> sampling_values = {};
std::vector<s16> sampling_indices = {};

void imgui_machinima_animation_player(MarioActor* actor, bool sampling) {
    if (!sampling) actor->custom_bone = false;
    if (ImGui::BeginTabBar("###anim_tab_bar")) {
        if (ImGui::BeginTabItem("SM64")) {
            ImGui::PushItemWidth(316);
            ImGui::InputTextWithHint("###anim_search", ICON_FK_SEARCH " Search...", animSearchTerm, 128);
            if (ImGui::BeginChild("###anim_box_child", ImVec2(316, 100), true)) {
                std::vector<int> anim_order = get_sorted_anim_list();
                for (int i : anim_order) {
                    const bool is_selected = i == actor->animstate.id && !actor->animstate.custom;
                    auto position = std::find(favorite_anims.begin(), favorite_anims.end(), i);
                    bool contains = position != favorite_anims.end();
                    if (ImGui::SmallButton((std::string(contains ? ICON_FK_STAR : ICON_FK_STAR_O) + "###anim_fav_" + std::to_string(i)).c_str())) {
                        if (contains) favorite_anims.erase(position);
                        else favorite_anims.push_back(i);
                        saturn_save_favorite_anims();
                    }
                    ImGui::SameLine();
                    if (ImGui::Selectable(saturn_animations_list[i], is_selected)) {
                        if (sampling) {
                            sampling_anim_loaded = true;
                            load_animation(&sampling_animation, i);
                            sampling_frame = 0;
                        }
                        else {
                            actor->animstate.id = i;
                            actor->animstate.custom = false;
                            actor->animstate.frame = 0;
                        }
                    }
                }
                ImGui::EndChild();
            }
            ImGui::PopItemWidth();
            ImGui::Checkbox("Movement",      &selected_groups[0]); ImGui::SameLine();
            ImGui::Checkbox("Actions",       &selected_groups[1]); ImGui::SameLine();
            ImGui::Checkbox("Automatic",     &selected_groups[2]);
            ImGui::Checkbox("Damage/Deaths", &selected_groups[3]); ImGui::SameLine();
            ImGui::Checkbox("Cutscenes",     &selected_groups[4]);
            ImGui::Checkbox("Water",         &selected_groups[5]); ImGui::SameLine();
            ImGui::Checkbox("Climbing",      &selected_groups[6]); ImGui::SameLine();
            ImGui::Checkbox("Object",        &selected_groups[7]);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("MComp")) {
            ImGui::PushItemWidth(316);
            saturn_file_browser_filter_extension("json");
            saturn_file_browser_scan_directory(current_anim_dir_path);
            saturn_file_browser_height(120);
            if (saturn_file_browser_show("animations")) {
                std::string path = saturn_file_browser_get_selected().string();
                if (std::find(canim_array.begin(), canim_array.end(), path) == canim_array.end()) canim_array.push_back(path);
                if (sampling) {
                    sampling_anim_loaded = true;
                    std::ifstream stream = std::ifstream(current_anim_dir_path / fs::path(path));
                    Json::Value value;
                    value << stream;
                    auto [ length, values, indices ] = read_bone_data(value);
                    sampling_values = values;
                    sampling_indices = indices;
                    sampling_animation.flags = 4;
                    sampling_animation.unk02 = 0;
                    sampling_animation.unk04 = 0;
                    sampling_animation.unk06 = 0;
                    sampling_animation.unk08 = (s16)length;
                    sampling_animation.unk0A = sampling_indices.size() / 6 - 1;
                    sampling_animation.values = sampling_values.data();
                    sampling_animation.index = (const u16*)sampling_indices.data();
                    sampling_animation.length = (s16)length;
                }
                else {
                    actor->animstate.id = std::find(canim_array.begin(), canim_array.end(), path) - canim_array.begin();
                    actor->animstate.custom = true;
                    actor->animstate.frame = 0;
                    saturn_read_mcomp_animation(actor, path.c_str());
                }
            }
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (!sampling) if (ImGui::BeginTabItem("Custom")) {
            actor->custom_bone = true;
            int currbone = 0;
            if (ImGui::BeginMenu("Sample")) {
                imgui_machinima_animation_player(actor, true);
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Reset")) {
                for (int i = 0; i < 20; i++) {
                    actor->bones[i][0] = 0;
                    actor->bones[i][1] = 0;
                    actor->bones[i][2] = 0;
                }
            }
#define SHOW if (true)
#define HIDE if (false)
#define BONE_ENTRY(name) {                                                     \
                ImGui::TableSetColumnIndex(0);                                  \
                ImGui::DragFloat3(name, actor->bones[currbone++]);               \
                ImGui::TableSetColumnIndex(1);                                    \
                saturn_keyframe_popout("k_mariobone_" + std::to_string(currbone)); \
                ImGui::TableNextRow();                                              \
            } else { currbone++; }
            if (ImGui::BeginTable("Bone Editor", 2)) {
                ImGui::TableNextRow();
                SHOW BONE_ENTRY("Root"           ); // Bone 1
                HIDE BONE_ENTRY("Bone 2"         ); // Bone 1 Duplicatee
                SHOW BONE_ENTRY("Torso"          ); // Bone 3
                SHOW BONE_ENTRY("Head"           ); // Bone 4
                HIDE BONE_ENTRY("Bone 5"         ); // Bone 6 Duplicate
                SHOW BONE_ENTRY("Upper Left Arm" ); // Bone 6
                SHOW BONE_ENTRY("Lower Left Arm" ); // Bone 7
                SHOW BONE_ENTRY("Left Hand"      ); // Bone 8
                HIDE BONE_ENTRY("Bone 9"         ); // Bone 10 Duplicate
                SHOW BONE_ENTRY("Upper Right Arm"); // Bone 10
                SHOW BONE_ENTRY("Lower Right Arm"); // Bone 11
                SHOW BONE_ENTRY("Right Hand"     ); // Bone 12
                HIDE BONE_ENTRY("Bone 13"        ); // Bone 14 Duplicate
                SHOW BONE_ENTRY("Upper Left Leg" ); // Bone 14
                SHOW BONE_ENTRY("Lower Left Leg" ); // Bone 15
                SHOW BONE_ENTRY("Left Foot"      ); // Bone 16
                HIDE BONE_ENTRY("Bone 17"        ); // Bone 18 Duplicate
                SHOW BONE_ENTRY("Upper Right Leg"); // Bone 18
                SHOW BONE_ENTRY("Lower Right Leg"); // Bone 19
                SHOW BONE_ENTRY("Right Foot"     ); // Bone 20
                ImGui::EndTable();
            }
#undef BONE_ENTRY
#undef HIDE
#undef SHOW
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    if (actor->custom_bone && !sampling) return;
    ImGui::Separator();
    if (sampling) {
        if (!ImGui::SliderFloat("Frame", &sampling_frame, 0, sampling_animation.unk08, "%.0f")) return;
        if (!sampling_anim_loaded) return;
        const u16* curindex = sampling_animation.index;
        curindex += 6;
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 3; j++) {
                int valindex = 0;
                if (sampling_frame < curindex[0]) valindex = curindex[1] + sampling_frame;
                else valindex = curindex[1] + curindex[0] - 1;
                curindex += 2;
                actor->bones[i][j] = (float)(sampling_animation.values[valindex]) * 360.f / 65536.f;
            }
        }
        return;
    }
    ImGui::SliderFloat("Frame", &actor->animstate.frame, 0, actor->animstate.length, "%.0f");
    saturn_keyframe_popout("k_mario_anim_frame");
    saturn_keyframe_popout_next_line("k_mario_anim");
    if (saturn_timeline_exists(saturn_keyframe_get_mario_timeline_id("k_mario_anim_frame", saturn_actor_indexof(actor)).c_str()))
        saturn_keyframe_helper("k_mario_anim_frame", &actor->animstate.frame, actor->animstate.length);
}

/*void imgui_machinima_animation_player() {
    selected_animation = (MarioAnimID)current_sanim_id;
    
    if (is_anim_playing || keyframe_playing)
        ImGui::BeginDisabled();

    const char* anim_groups[] = { "Movement (50)", "Actions (25)", "Automatic (27)", "Damage/Deaths (22)",
        "Cutscenes (23)", "Water (16)", "Climbing (20)", "Object (24)", ICON_FK_FILE_O " CUSTOM...", "All (209)",
        (std::string("Favorites (") + std::to_string(favorite_anims.size()) + ")").c_str() };
    int animArraySize = (canim_array.size() > 0) ? IM_ARRAYSIZE(anim_groups) : IM_ARRAYSIZE(anim_groups) - 1;

    ImGui::PushItemWidth(290);
    if (ImGui::BeginCombo("###anim_group", anim_groups[current_sanim_group_index], ImGuiComboFlags_HeightLarge)) {
        for (int n = 0; n < animArraySize; n++) {
            const bool is_selected = (current_sanim_group_index == n);
            if (ImGui::Selectable(anim_groups[n], is_selected)) {
                current_sanim_group_index = n;
                if (current_sanim_group_index == 8) {
                    current_animation.custom = true;
                    current_sanim_id = MARIO_ANIM_A_POSE;

                    current_sanim_name = canim_array[custom_anim_index];
                    anim_preview_name = current_sanim_name;
                    anim_preview_name = anim_preview_name.substr(0, anim_preview_name.size() - 5);
                    saturn_read_mcomp_animation(anim_preview_name);
                    current_animation.loop = current_canim_looping;
                } else if (current_sanim_group_index == 9) {
                    current_anim_map = sanim_maps[9];
                    current_animation.custom = false;

                    current_anim_map.clear();
                    for (int i = 0; i < 8; i++) {
                        for (auto& anim : sanim_maps[i]) {
                            current_anim_map.insert(anim);
                        }
                    }
                    current_sanim_index = current_anim_map.begin()->first.first;
                    current_sanim_name = current_anim_map.begin()->first.second;
                    current_sanim_id = current_anim_map.begin()->second;
                    anim_preview_name = current_sanim_name;
                } else if (current_sanim_group_index == 10) {
                    current_animation.custom = false;
                    current_sanim_id = favorite_anims.size() == 0 ? MARIO_ANIM_A_POSE : favorite_anims[0];
                    current_sanim_index = 0;
                    current_sanim_name = saturn_animations_list[current_sanim_id];
                    anim_preview_name = current_sanim_name;
                } else {
                    current_anim_map = sanim_maps[current_sanim_group_index];
                    current_animation.custom = false;
                    current_sanim_index = current_anim_map.begin()->first.first;
                    current_sanim_name = current_anim_map.begin()->first.second;
                    current_sanim_id = current_anim_map.begin()->second;
                    anim_preview_name = current_sanim_name;
                }
            }

            if (n == 8) {
                ImGui::SameLine();
                imgui_bundled_help_marker("These are custom METAL Composer+ JSON animations.\nPlace in dynos/anims.");
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if ((current_sanim_group_index == 8 && canim_array.size() >= 20) || current_sanim_group_index == 9) {
        ImGui::InputTextWithHint("###anim_search_text", ICON_FK_SEARCH " Search animations...", animSearchTerm, IM_ARRAYSIZE(animSearchTerm), ImGuiInputTextFlags_AutoSelectAll);
    } else {
        // If our anim list is reloaded, and we now have less than 20 anims, this can cause filter issues if not reset to nothing
        if (animSearchTerm != "") strcpy(animSearchTerm, "");
    }
    string animSearchLower = animSearchTerm;
    std::transform(animSearchLower.begin(), animSearchLower.end(), animSearchLower.begin(),
        [](unsigned char c){ return std::tolower(c); });

    ImGui::PopItemWidth();

    ImGui::BeginChild("###anim_box_child", ImVec2(290, 100), true);
    if (current_sanim_group_index == 8) {
        for (int i = 0; i < canim_array.size(); i++) {
            current_sanim_name = canim_array[i];
            if (canim_array[i].find("/") != string::npos)
                current_sanim_name = ICON_FK_FOLDER " " + canim_array[i].substr(0, canim_array[i].size() - 1);

            if (canim_array[i] == "../")
                current_sanim_name = ICON_FK_FOLDER " ../";

            const bool is_selected = (custom_anim_index == i);

            // If we're searching, only include anims with the search keyword in the name
            // Also convert to lowercase
            if (animSearchLower != "") {
                string nameLower = current_sanim_name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                    [](unsigned char c1){ return std::tolower(c1); });

                if (nameLower.find(animSearchLower) == string::npos) {
                    continue;
                }
            }

            if (ImGui::Selectable(current_sanim_name.c_str(), is_selected)) {
                custom_anim_index = i;
                current_sanim_name = canim_array[i];
                anim_preview_name = current_sanim_name;
                current_animation.loop = current_canim_looping;
                // Remove .json extension
                if (canim_array[i].find(".json") != string::npos) {
                    anim_preview_name = anim_preview_name.substr(0, anim_preview_name.size() - 5);
                    saturn_read_mcomp_animation(anim_preview_name);
                    current_animation.loop = current_canim_looping;
                } else if (canim_array[i].find("/") != string::npos) {
                    saturn_load_anim_folder(anim_preview_name, &custom_anim_index);
                    current_sanim_name = canim_array[custom_anim_index];
                    anim_preview_name = current_sanim_name;
                    anim_preview_name = anim_preview_name.substr(0, anim_preview_name.size() - 5);
                    saturn_read_mcomp_animation(anim_preview_name);
                    current_animation.loop = current_canim_looping;
                }
                // Stop anim
                is_anim_playing = false;
                is_anim_paused = false;
                using_chainer = false;
                chainer_index = 0;
                current_animation.custom = true;
                current_animation.id = i;
            }

            if (ImGui::BeginPopupContextItem()) {
                ImGui::Text(canim_array[i].c_str());
                imgui_bundled_tooltip((current_anim_dir_path + canim_array[i]).c_str());
                ImGui::Separator();
                ImGui::TextDisabled("%i MComp+ animation(s)", canim_array.size());
                if (ImGui::Button("Refresh###refresh_canim")) {
                    saturn_load_anim_folder(current_anim_dir_path, &custom_anim_index);
                    current_sanim_name = canim_array[custom_anim_index];
                    anim_preview_name = current_sanim_name;
                    anim_preview_name = anim_preview_name.substr(0, anim_preview_name.size() - 5);
                    saturn_read_mcomp_animation(anim_preview_name);
                    current_animation.loop = current_canim_looping;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
    } else if (current_sanim_group_index == 9) {
        for (int i = 0; i < 209; i++) {
            const bool is_selected = (current_sanim_index == i);
            current_sanim_name = saturn_animations_list[i];

            // If we're searching, only include anims with the search keyword in the name
            // Also convert to lowercase
            if (animSearchLower != "") {
                string nameLower = current_sanim_name;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
                    [](unsigned char c1){ return std::tolower(c1); });

                if (nameLower.find(animSearchLower) == string::npos) {
                    continue;
                }
            }

            auto position = std::find(favorite_anims.begin(), favorite_anims.end(), i);
            bool contains = position != favorite_anims.end();
            if (ImGui::SmallButton((std::string(contains ? ICON_FK_STAR : ICON_FK_STAR_O) + "###" + std::to_string(i)).c_str())) {
                if (contains) favorite_anims.erase(position);
                else favorite_anims.push_back(i);
                saturn_save_favorite_anims();
            }
            ImGui::SameLine();
            if (ImGui::Selectable(current_sanim_name.c_str(), is_selected)) {
                current_sanim_index = i;
                current_sanim_name = saturn_animations_list[i];
                current_sanim_id = i;
                current_animation.custom = false;
                current_animation.id = i;
            }

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
    } else if (current_sanim_group_index == 10) {
        int idx = 0;
        for (int anim : favorite_anims) {
            current_sanim_name = saturn_animations_list[anim];
            const bool is_selected = (current_sanim_id == anim);
            auto position = std::find(favorite_anims.begin(), favorite_anims.end(), anim);
            if (ImGui::SmallButton((std::string(ICON_FK_STAR) + "###" + std::to_string(anim)).c_str())) {
                favorite_anims.erase(position);
                saturn_save_favorite_anims();
            }
            ImGui::SameLine();
            if (ImGui::Selectable(current_sanim_name.c_str(), is_selected)) {
                current_sanim_index = idx;
                current_sanim_name = saturn_animations_list[anim];
                current_sanim_id = anim;
                current_animation.custom = false;
                current_animation.id = anim;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
            idx++;
        }
    } else {
        for (auto &[a,b]:current_anim_map) {
            current_sanim_index = a.first;
            current_sanim_name = a.second;

            const bool is_selected = (current_sanim_id == b);
            auto position = std::find(favorite_anims.begin(), favorite_anims.end(), b);
            bool contains = position != favorite_anims.end();
            if (ImGui::SmallButton((std::string(contains ? ICON_FK_STAR : ICON_FK_STAR_O) + "###" + std::to_string(b)).c_str())) {
                if (contains) favorite_anims.erase(position);
                else favorite_anims.push_back(b);
                saturn_save_favorite_anims();
            }
            ImGui::SameLine();
            if (ImGui::Selectable(current_sanim_name.c_str(), is_selected)) {
                current_sanim_index = a.first;
                current_sanim_name = a.second;
                current_sanim_id = b;
                anim_preview_name = current_sanim_name;
                current_animation.custom = false;
                current_animation.id = b;
            }

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
    }
    ImGui::EndChild();

    if (is_anim_playing && !keyframe_playing)
        ImGui::EndDisabled();

    ImGui::PushItemWidth(290);
    ImGui::Separator();

    // Metadata
    if (current_animation.custom && custom_anim_index != -1) {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("###anim_metadata", ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 3), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text(current_canim_name.c_str());
        ImGui::TextDisabled(("@ " + current_canim_author).c_str());
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    ImGui::PopItemWidth();

    // Player
    if (is_anim_playing) {
        if (current_animation.custom) {
            ImGui::Text("Now Playing: %s", current_canim_name.c_str());
            string anim_credit1 = ("By " + current_canim_author);
            ImGui::SameLine(); imgui_bundled_help_marker(anim_credit1.c_str());
            if (using_chainer) ImGui::Text("Chainer: Enabled");
        } else {
            ImGui::Text("Now Playing: %s", anim_preview_name.c_str());
        }

        if (ImGui::Button("Stop")) {
            is_anim_playing = false;
            is_anim_paused = false;
            using_chainer = false;
            chainer_index = 0;
        }
        ImGui::SameLine(); ImGui::Checkbox("Loop", &current_animation.loop);
        ImGui::SameLine();
        if (ImGui::Checkbox("Hang", &current_animation.hang) && !current_animation.hang) {
            is_anim_playing = false;
            is_anim_paused = false;
            using_chainer = false;
            chainer_index = 0;
        }
        
        ImGui::PushItemWidth(150);
        ImGui::SliderInt("Frame###animation_frames", &current_anim_frame, 0, current_anim_length - 1);
        ImGui::PopItemWidth();
        ImGui::Checkbox("Paused###animation_paused", &is_anim_paused);
    } else {
        ImGui::Text("");
        if (ImGui::Button("Play")) {
            anim_play_button();
        }
        ImGui::SameLine(); ImGui::Checkbox("Loop", &current_animation.loop);
        ImGui::SameLine(); ImGui::Checkbox("Hang", &current_animation.hang);

        ImGui::PushItemWidth(150);
        ImGui::SliderFloat("Speed###anim_speed", &current_animation.speed, 0.1f, 2.0f);
        ImGui::PopItemWidth();
        if (current_animation.speed != 1.0f) {
            if (ImGui::Button("Reset###reset_anim_speed"))
                current_animation.speed = 1.0f;
        }
    }
    if (keyframe_playing) ImGui::EndDisabled();
    saturn_keyframe_popout_next_line("k_mario_anim");
}*/
