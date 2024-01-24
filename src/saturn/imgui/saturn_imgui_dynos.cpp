#include "saturn_imgui_dynos.h"

#include "saturn/libs/portable-file-dialogs.h"

#include <algorithm>
#include <string>
#include <cstring>
#include <iostream>
#include "GL/glew.h"
#include "saturn/saturn_actors.h"
#include "types.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdlib>
#endif

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/libs/imgui/imgui-knobs.h"
#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_textures.h"
#include "saturn_imgui.h"
#include "saturn_imgui_cc_editor.h"
#include "saturn_imgui_expressions.h"
#include "pc/controller/controller_keyboard.h"
#include "data/dynos.cpp.h"
#include <SDL2/SDL.h>

#include "icons/IconsForkAwesome.h"

extern "C" {
#include "pc/gfx/gfx_pc.h"
#include "pc/configfile.h"
#include "pc/platform.h"
#include "game/mario.h"
#include "game/level_update.h"
#include <mario_animation_ids.h>
#include "pc/cheats.h"
#include "include/sm64.h"
#include "game/sound_init.h"
}

Array<PackData *> &sDynosPacks = DynOS_Gfx_GetPacks();

using namespace std;

int current_eye_state = 0;
int current_eye_id = 0;
string eye_name;
int current_mouth_id = 0;
string mouth_name;

int windowListSize = 325;

bool using_custom_eyes;

static char modelSearchTerm[128];

bool has_copy_mario;

bool last_model_had_model_eyes;

bool is_gameshark_open;

std::vector<std::string> choose_file_dialog(std::string windowTitle, std::vector<std::string> filetypes, bool multiselect) {
    return pfd::open_file(windowTitle, ".", filetypes, multiselect ? pfd::opt::multiselect : pfd::opt::none).result();
}

void open_directory(std::string path) {
#if defined(_WIN32) // Windows
    ShellExecuteA(NULL, "open", ("\"" + path + "\"").c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__) // macOS
    system(("open \"" + path + "\"").c_str());
#else // Linux
    system(("xdg-open \"" + path + "\"").c_str());
#endif
}

// UI

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

GLuint global_expression_preview = 0;
int global_expression_preview_width = 0;
int global_expression_preview_height = 0;
int global_sample_ratio;
string last_preview_name;

void expression_preview(const char* filename) {
    global_expression_preview = 0;
    bool ret = LoadTextureFromFile(filename, &global_expression_preview, &global_expression_preview_width, &global_expression_preview_height);
    IM_ASSERT(ret);
    global_sample_ratio = global_expression_preview_width / global_expression_preview_height;
    //if (global_expression_preview_height > 16 && global_expression_preview_width > 16) {
    //    global_sample_ratio = (256 / global_expression_preview_height);
    //}
}

void OpenModelSelector(MarioActor* actor) {
    CCChangeActor(actor);
    ImGui::Text("Model Packs");
    ImGui::SameLine(); imgui_bundled_help_marker(
        "DynOS v1.1 by PeachyPeach\n\nThese are DynOS model packs, used for live model loading.\nPlace packs in /dynos/packs.");

    // Model search when our list is 20 or more
    if (model_list.size() >= 20) {
        ImGui::InputTextWithHint("###model_search_text", ICON_FK_SEARCH " Search models...", modelSearchTerm, IM_ARRAYSIZE(modelSearchTerm), ImGuiInputTextFlags_AutoSelectAll);
    } else {
        // If our model list is reloaded, and we now have less than 20 packs, this can cause filter issues if not reset to nothing
        if (modelSearchTerm != "") strcpy(modelSearchTerm, "");
    }
    std::string modelSearchLower = modelSearchTerm;
    std::transform(modelSearchLower.begin(), modelSearchLower.end(), modelSearchLower.begin(),
        [](unsigned char c){ return std::tolower(c); });

    if (model_list.size() <= 0) {
        ImGui::TextDisabled("No model packs found.\nPlace model folders in\n/dynos/packs/.");
    } else {
        ImGui::BeginChild("###menu_model_selector", ImVec2(-FLT_MIN, 125), true);
        for (int i = 0; i < model_list.size(); i++) {
            Model model = model_list[i];
            if (model.Active) {
                bool is_selected = actor->selected_model == i;

                // If we're searching, only include CCs with the search keyword in the name
                // Also convert to lowercase
                std::string label_name_lower = model.SearchMeta();
                std::transform(label_name_lower.begin(), label_name_lower.end(), label_name_lower.begin(),
                        [](unsigned char c1){ return std::tolower(c1); });
                if (modelSearchLower != "") {
                    if (label_name_lower.find(modelSearchLower) == std::string::npos) {
                        continue;
                    }
                }

                std::string packLabelId = model.FolderName + "###s_model_pack_" + std::to_string(i);
                if (ImGui::Selectable(packLabelId.c_str(), &is_selected)) {
                    if (saturn_timeline_exists("k_mario_expr")) k_frame_keys.erase("k_mario_expr");
                    
                    // Select model
                    actor->model = model;
                    actor->selected_model = i;

                    // Load expressions
                    actor->model.Expressions.clear();
                    actor->model.Expressions = LoadExpressions(current_model.FolderPath);

                    if (is_selected) {
                        std::cout << "Loaded " << model.Name << " by " << model.Author << std::endl;

                        // Load model CCs
                        RefreshColorCodeList();
                        if (is_selected && (current_color_code.IsDefaultColors() || last_model_cc_address == current_color_code.GameShark)) {
                            ResetColorCode(true);
                            last_model_cc_address = current_color_code.GameShark;
                        }
                    } else {
                        if (!AnyModelsEnabled()) {
                            actor->model = Model();
                            actor->selected_model = -1;
                        }
                        
                        // Reset model CCs
                        model_color_code_list.clear();
                        RefreshColorCodeList();
                        if (last_model_cc_address == current_color_code.GameShark)
                            ResetColorCode(false);
                    }
                }
                std::string popupLabelId = "###menu_model_popup_" + std::to_string(i);
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    ImGui::OpenPopup(popupLabelId.c_str());
                if (ImGui::BeginPopup(popupLabelId.c_str())) {
                    // Right-click menu
                    if (model.Author.empty()) ImGui::Text(ICON_FK_FOLDER_O " %s/", model.FolderName.c_str());
                    else ImGui::Text(ICON_FK_FOLDER " %s/", model.FolderName.c_str());

                    imgui_bundled_tooltip(("/%s", model.FolderPath).c_str());
                    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                        open_directory(std::string(sys_exe_path()) + "/" + model.FolderPath + "/");

                    ImGui::SameLine(); ImGui::TextDisabled(" Pack #%i", model.DynOSId + 1);

                    std::vector<std::string> cc_list = GetColorCodeList(model.FolderPath + "/colorcodes");
                    has_open_any_model_cc = true;

                    if (model.HasColorCodeFolder() && cc_list.size() > 0) {
                        // List of model color codes
                        ImGui::BeginChild("###menu_model_ccs", ImVec2(-FLT_MIN, 75), true);
                        OpenModelCCSelector(model, cc_list, "");
                        ImGui::EndChild();
                    }

                    ImGui::Separator();
                    ImGui::TextDisabled("%i model pack(s)", model_list.size());
                    if (ImGui::Button(ICON_FK_DOWNLOAD " Refresh Packs###refresh_dynos_packs")) {
                        sDynosPacks.Clear();
                        DynOS_Opt_Init();
                        model_list = GetModelList("dynos/packs");
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine(); imgui_bundled_help_marker("WARNING: Experimental - this will probably lag the game.");
                    ImGui::EndPopup();
                }

                if (!model.Author.empty()) {
                    ImGui::SameLine();
                    ImGui::TextDisabled(model.Author.c_str());
                }
            }
        }
        ImGui::EndChild();

        // Open DynOS Folder Button

        if (ImGui::Button(ICON_FK_FOLDER_OPEN_O " Open Packs Folder...###open_packs_folder"))
            open_directory(std::string(sys_exe_path()) + "/dynos/packs/");
    }
}

void sdynos_imgui_init() {
    LoadEyesFolder();

    model_list = GetModelList("dynos/packs");
    RefreshColorCodeList();

    //model_details = "" + std::to_string(sDynosPacks.Count()) + " model pack";
    //if (sDynosPacks.Count() != 1) model_details += "s";
}

bool link_scaling = true;

void sdynos_imgui_menu(int index) {
    MarioActor* actor = saturn_get_actor(index);
    if (ImGui::BeginMenu(ICON_FK_USER_CIRCLE " Edit Avatar###menu_edit_avatar")) {
        // Color Code Selection
        if (!support_color_codes || !current_model.ColorCodeSupport) ImGui::BeginDisabled();
            OpenCCSelector(actor);
            // Open File Dialog
            if (ImGui::Button(ICON_FK_FILE_TEXT_O " Open CC Folder...###open_cc_folder"))
                open_directory(std::string(sys_exe_path()) + "/dynos/colorcodes/");
        if (!support_color_codes || !current_model.ColorCodeSupport) ImGui::EndDisabled();

        // Model Selection
        OpenModelSelector(actor);

        ImGui::EndMenu();
    }

    // Color Code Editor
    if (ImGui::BeginMenu(ICON_FK_PAINT_BRUSH " Color Code Editor###menu_cc_editor", support_color_codes & current_model.ColorCodeSupport)) {
        OpenCCEditor(actor);
        ImGui::EndMenu();
    }

    // Animation Mixtape
    if (ImGui::MenuItem(ICON_FK_FILM " Animation Mixtape###menu_anim_player", NULL, windowAnimPlayer, mario_exists)) {
        windowCcEditor = false;
        windowChromaKey = false;
        windowAnimPlayer = !windowAnimPlayer;
    }

    ImGui::Separator();

    if (!current_model.ColorCodeSupport) ImGui::BeginDisabled();
        ImGui::Checkbox("Color Code Support", &support_color_codes);
        imgui_bundled_tooltip(
            "Toggles color code features.");

        if (!support_color_codes) ImGui::BeginDisabled();
            if (current_model.SparkSupport) {
                ImGui::Checkbox("CometSPARK Support", &support_spark);
                imgui_bundled_tooltip(
                    "Toggles SPARK features, which provides supported models with extra color values.");
            }
        if (!support_color_codes) ImGui::EndDisabled();
    if (!current_model.ColorCodeSupport) ImGui::EndDisabled();

    // Misc. Avatar Settings
    if (ImGui::BeginMenu("Customize...###menu_misc")) {

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("Misc.###misc_child", ImVec2(225, 175), true, ImGuiWindowFlags_None);
        if (ImGui::BeginTabBar("###misc_tabbar", ImGuiTabBarFlags_None)) {

            if (ImGui::BeginTabItem("Switches###switches_scale")) {
                const char* eyes[] = { "Blinking", "Open", "Half", "Closed", "Left", "Right", "Up", "Down", "Dead" };
                ImGui::Combo("Eyes###eye_state", &actor->eye_state, eyes, IM_ARRAYSIZE(eyes));
                const char* hands[] = { "Fists", "Open", "Peace", "With Cap", "With Wing Cap", "Right Open" };
                ImGui::Combo("Hand###hand_state", &actor->hand_state, hands, IM_ARRAYSIZE(hands));
                const char* caps[] = { "Cap On", "Cap Off", "Wing Cap" }; // unused "wing cap off" not included
                ImGui::Combo("Cap###cap_state", &actor->cap_state, caps, IM_ARRAYSIZE(caps));
                const char* powerups[] = { "Default", "Vanish", "Metal", "Metal & Vanish" };
                ImGui::Combo("Powerup###powerup_state", &actor->powerup_state, powerups, IM_ARRAYSIZE(powerups));
                if (actor->powerup_state & 1) ImGui::SliderFloat("Alpha", &actor->alpha, 0, 255);
                if (AnyModelsEnabled()) ImGui::BeginDisabled();
                ImGui::Checkbox("M Cap Emblem", &actor->show_emblem);
                imgui_bundled_tooltip("Enables the signature \"M\" logo on Mario's cap.");
                saturn_keyframe_popout("k_v_cap_emblem");
                if (AnyModelsEnabled()) ImGui::EndDisabled();

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Special###tab_special")) {
                if (link_scaling) {
                    if (ImGui::SliderFloat("Size###mscale_all", &actor->xScale, 0.f, 2.f)) {
                        actor->yScale = actor->xScale;
                        actor->zScale = actor->xScale;
                    }
                    saturn_keyframe_popout("k_scale");
                } else {
                    ImGui::SliderFloat("X###mscale_x", &actor->xScale, -2.f, 2.f);
                    saturn_keyframe_popout("k_scale_x");
                    ImGui::SliderFloat("Y###mscale_y", &actor->yScale, -2.f, 2.f);
                    saturn_keyframe_popout("k_scale_y");
                    ImGui::SliderFloat("Z###mscale_z", &actor->zScale, -2.f, 2.f);
                    saturn_keyframe_popout("k_scale_z");
                }
                ImGui::Checkbox("Link###link_mario_scale", &link_scaling);
                if (actor->xScale != 1.f || actor->yScale != 1.f || actor->zScale != 1.f) {
                    ImGui::SameLine(); if (ImGui::Button("Reset###reset_mscale")) {
                        actor->xScale = 1.f;
                        actor->yScale = 1.f;
                        actor->zScale = 1.f;
                    }
                }

                if (mario_exists) {
                    Vec3f actor_pos;
                    vec3f_set(actor_pos, actor->x, actor->y, actor->z);

                    ImGui::Dummy(ImVec2(0, 5));
                    ImGui::Text("Position");
                    ImGui::InputFloat3("###mario_set_pos", actor_pos);
                    if (ImGui::Button(ICON_FK_FILES_O " Copy###copy_mario")) {
                        vec3f_copy(stored_mario_pos, actor_pos);
                        vec3s_set(stored_mario_angle, 0, actor->angle, 0);
                        has_copy_mario = true;
                    } ImGui::SameLine();
                    if (!has_copy_mario) ImGui::BeginDisabled();
                    if (ImGui::Button(ICON_FK_CLIPBOARD " Paste###paste_mario")) {
                        if (has_copy_mario) {
                            vec3f_copy(actor_pos, stored_mario_pos);
                            actor->angle = stored_mario_angle[1];
                        }
                    }
                    if (!has_copy_mario) ImGui::EndDisabled();

                    actor->x = actor_pos[0];
                    actor->y = actor_pos[1];
                    actor->z = actor_pos[2];
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        if (ImGui::BeginTable("misc_table", 2)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Checkbox("Dust Particles", &enable_dust_particles);
            imgui_bundled_tooltip("Displays dust particles when Mario moves.");
            ImGui::Checkbox("Torso Rotations", &enable_torso_rotation);
            imgui_bundled_tooltip("Tilts Mario's torso when he moves; Disable for a \"beta running\" effect.");
            ImGui::Dummy(ImVec2(0, 0)); ImGui::SameLine(25); ImGui::Text(ICON_FK_SKATE " Walkpoint");
            ImGui::Dummy(ImVec2(0, 0)); ImGui::SameLine(25); ImGui::SliderFloat("###run_speed", &run_speed, 0.f, 127.f, "%.0f");
            imgui_bundled_tooltip("Controls the axis range for Mario's running input; Can be used to force walking (36) or tiptoe (25) animations.");

            ImGui::TableSetColumnIndex(1);
            float angle = actor->angle / 32768.0f * 180;

            ImGuiKnobs::Knob("Angle", &angle, -180.f, 180.f, 0.f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal);
            saturn_keyframe_popout("k_angle");

            ImGui::Checkbox("Spin###spin_angle", &actor->spinning);
            if (actor->spinning) {
                angle += actor->spin_speed * 5;
                ImGui::SliderFloat("Speed###spin,speed", &actor->spin_speed, -2.f, 2.f, "%.1f");
            }

            actor->angle = angle / 180 * 32768;

            ImGui::EndTable();
        }
        if (mario_exists) if (ImGui::BeginMenu("Head Rotations")) {
            ImGui::Text("C-Up Settings");
            ImGui::PushItemWidth(75);
            ImGui::SliderFloat("Speed", &mario_headrot_speed, 0, 50, "%.1f");
            ImGui::PopItemWidth();
            if (ImGui::BeginTable("headrot_table", 2)) {
                float fake_yaw = actor->head_rot_x * 360.f / 65536;
                float fake_pitch = actor->head_rot_y * 360.f / 65536;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGuiKnobs::Knob("Yaw", &fake_yaw, configCUpLimit ? -120.0f : -180.0f, configCUpLimit ? 120.0f : 180.0f, 0.0f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal)) {
                    actor->head_rot_x = fake_yaw * 65536 / 360;
                }
                ImGui::TableSetColumnIndex(1);
                if (ImGuiKnobs::Knob("Pitch", &fake_pitch, configCUpLimit ? -45.0f : -180.0f, configCUpLimit ? 80.0f : 180.0f, 0.0f, "%.0f deg", ImGuiKnobVariant_Dot, 0.f, ImGuiKnobFlags_DragHorizontal)) {
                    actor->head_rot_y = fake_pitch * 65536 / 360;
                }
                ImGui::EndTable();
            }
            saturn_keyframe_popout("k_mario_headrot");
            ImGui::EndMenu();
        }

        ImGui::PopStyleVar();
        ImGui::EndMenu();
    }

    ImGui::Separator();

    // Model Metadata
    if (!current_model.Author.empty()) {
        string metaLabelText = (ICON_FK_USER " " + current_model.Name);
        string metaDataText = "v" + current_model.Version;
        if (current_model.Description != "")
            metaDataText = "v" + current_model.Version + "\n" + current_model.Description;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("###model_metadata", ImVec2(0, 45), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text(metaLabelText.c_str()); imgui_bundled_tooltip(metaDataText.c_str());
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            open_directory(std::string(sys_exe_path()) + "/" + current_model.FolderPath + "/");
        ImGui::TextDisabled(("@ " + current_model.Author).c_str());
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::Separator();
    }

    if (current_model.CustomEyeSupport) {
        // Custom Eyes Checkbox
        if (ImGui::Checkbox("Custom Eyes", &custom_eyes_enabled)) {
            if (scrollEyeState < 4 && custom_eyes_enabled) scrollEyeState = 4;
            else scrollEyeState = 0;
        }
        if (!AnyModelsEnabled())
            imgui_bundled_tooltip("Place custom eye PNG textures in /dynos/eyes/.");

        if ((custom_eyes_enabled && current_model.Expressions.size() > 0) || current_model.Expressions.size() > 1)
            ImGui::Separator();

        // Expressions Selector
        OpenExpressionSelector();

        // Keyframing
        if (custom_eyes_enabled ||
            current_model.Expressions.size() > 0)
                saturn_keyframe_popout_next_line("k_mario_expr");
    }
}