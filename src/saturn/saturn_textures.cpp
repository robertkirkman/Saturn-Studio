#include "saturn/saturn_textures.h"

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <SDL2/SDL.h>

#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn/imgui/saturn_imgui.h"
#include "saturn/imgui/saturn_imgui_chroma.h"

#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"

#include "pc/configfile.h"

extern "C" {
#include "game/mario.h"
#include "game/camera.h"
#include "game/level_update.h"
#include "sm64.h"
#include "pc/gfx/gfx_pc.h"
#include "levels/castle_inside/header.h"
}

using namespace std;
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <assert.h>
#include <stdlib.h>
namespace fs = std::filesystem;
#include "pc/fs/fs.h"

#include "saturn/saturn_json.h"

bool custom_eyes_enabled;
bool show_vmario_emblem;

Expression VanillaEyes;
/* Loads textures from dynos/eyes/ into a global Expression */
void LoadEyesFolder() {
    // Check if the dynos/eyes/ folder exists
    if (fs::is_directory("dynos/eyes")) {

        VanillaEyes.Name = "eyes";
        VanillaEyes.FolderPath = "dynos/eyes";

        // Load Textures
        for (const auto & entry : fs::directory_iterator("dynos/eyes")) {
            // Only allow PNG files
            if (entry.path().extension() == ".png") {
                TexturePath texture;
                texture.FileName = entry.path().filename().generic_u8string();
                texture.FilePath = entry.path().generic_u8string();
                VanillaEyes.Textures.push_back(texture);
            }
        }
    }
}


/* Loads textures into an expression */
std::vector<TexturePath> LoadExpressionTextures(Expression expression) {
    std::vector<TexturePath> textures;

    // Check if the expression's folder exists
    if (fs::is_directory(fs::path(expression.FolderPath))) {
        for (const auto & entry : fs::directory_iterator(expression.FolderPath)) {
            if (!fs::is_directory(entry.path())) {
                // Only allow PNG files
                if (entry.path().extension() == ".png") {
                    TexturePath texture;
                    texture.FileName = entry.path().filename().generic_u8string();
                    texture.FilePath = entry.path().generic_u8string();
                    textures.push_back(texture);
                }
            }
        }
    }
    return textures;
}

/* Loads an expression list into a specified model */
std::vector<Expression> LoadExpressions(std::string modelFolderPath) {
    std::vector<Expression> expressions_list;

    // Check if the model's /expressions folder exists
    if (fs::is_directory(fs::path(modelFolderPath + "/expressions"))) {
        for (const auto & entry : fs::directory_iterator(modelFolderPath + "/expressions")) {
            // Load individual expression folders
            if (fs::is_directory(entry.path())) {

                Expression expression;
                expression.Name = entry.path().filename().generic_u8string();
                if (expression.Name == "eye")
                    expression.Name = "eyes";

                expression.FolderPath = entry.path().generic_u8string();
                // Load all PNG files
                expression.Textures = LoadExpressionTextures(expression);

                // Eyes will always appear first
                if (expression.Name == "eyes") {
                    expressions_list.insert(expressions_list.begin(), expression);
                } else {
                    expressions_list.push_back(expression);
                }
            }
        }
    }

    return expressions_list;
}

std::string saturn_bind_texture_internal(const void* input) {
    const char* inputTexture = static_cast<const char*>(input);
    const char* outputTexture;

    if (input == (const void*)0x7365727574786574) return std::string(inputTexture);
    
    std::string texName = inputTexture;

    // Custom model expressions
    if (current_model.Active && texName.find("saturn_") != std::string::npos) {
        for (int i = 0; i < current_model.Expressions.size(); i++) {
            Expression expression = current_model.Expressions[i];
            // Checks if the incoming texture has the expression's "key"
            // This could be "saturn_eye", "saturn_mouth", etc.
            if (expression.PathHasReplaceKey(texName)) {
                return expression.Textures[expression.CurrentIndex].GetRelativePath();
            }
        }
    }

    // Vanilla eye textures
    if (custom_eyes_enabled && current_model.UsingVanillaEyes() && VanillaEyes.Textures.size() > 0) {
        if (texName.find("saturn_eye") != string::npos ||
            // Unused vanilla textures
            texName == "actors/mario/mario_eyes_left_unused.rgba16.png" ||
            texName == "actors/mario/mario_eyes_right_unused.rgba16.png" ||
            texName == "actors/mario/mario_eyes_up_unused.rgba16.png" ||
            texName == "actors/mario/mario_eyes_down_unused.rgba16.png") {
                return VanillaEyes.Textures[VanillaEyes.CurrentIndex].GetRelativePath();
        }
    }

    // Non-model custom blink cycle
    /*if (force_blink && eye_array.size() > 0 && is_replacing_eyes) {
        if (texName == "actors/mario/mario_eyes_center.rgba16.png" && blink_eye_1 != "") {
            outputTexture = blink_eye_1.c_str();
            const void* output = static_cast<const void*>(outputTexture);
            return output;
        } else if (texName == "actors/mario/mario_eyes_half_closed.rgba16.png" && blink_eye_2 != "") {
            outputTexture = blink_eye_2.c_str();
            const void* output = static_cast<const void*>(outputTexture);
            return output;
        } else if (texName == "actors/mario/mario_eyes_closed.rgba16.png" && blink_eye_3 != "") {
            outputTexture = blink_eye_3.c_str();
            const void* output = static_cast<const void*>(outputTexture);
            return output;
        }
    }*/

    // Non-model cap logo/emblem

    if (show_vmario_emblem) {
        if (texName == "actors/mario/no_m.rgba16.png")
            return "actors/mario/mario_logo.rgba16.png";
    }

    // AUTO-CHROMA

    // Overwrite skybox
    // This runs for both Auto-chroma and the Chroma Key Stage
    if (autoChroma || gCurrLevelNum == LEVEL_SA) {
        if (use_color_background) {
            // Use white, recolorable textures for our color background
            if (texName.find("textures/skyboxes/") != string::npos)
                return "textures/saturn/white.rgba16.png";
        } else {
            // Swapping skyboxes IDs
            if (texName.find("textures/skyboxes/") != string::npos) {
                std::string skybox_key = texName.substr(18, texName.find_first_of(".") - 18);
                switch(gChromaKeyBackground) {
                    case 0: return texName.replace(18, skybox_key.length(), "water");
                    case 1: return texName.replace(18, skybox_key.length(), "bitfs");
                    case 2: return texName.replace(18, skybox_key.length(), "wdw");
                    case 3: return texName.replace(18, skybox_key.length(), "cloud_floor");
                    case 4: return texName.replace(18, skybox_key.length(), "ccm");
                    case 5: return texName.replace(18, skybox_key.length(), "ssl");
                    case 6: return texName.replace(18, skybox_key.length(), "bbh");
                    case 7: return texName.replace(18, skybox_key.length(), "bidw");
                    case 8: return texName.replace(18, skybox_key.length(), "clouds");
                    case 9: return texName.replace(18, skybox_key.length(), "bits");
                }
            }
        }

        if (autoChroma) {
            // Toggle object visibility
            if (!autoChromaObjects) {
                if (texName.find("castle_grounds_textures.0BC00.ia16") != string::npos ||
                    texName.find("butterfly_wing.rgba16") != string::npos) {
                        return "textures/saturn/mario_logo.rgba16.png";
                }
            }
            // Toggle level visibility
            if (!autoChromaLevel) {
                if (texName.find("saturn") == string::npos &&
                    texName.find("dynos") == string::npos &&
                    texName.find("mario_") == string::npos &&
                    texName.find("mario/") == string::npos &&
                    texName.find("skybox") == string::npos &&
                    texName.find("shadow_quarter_circle.ia8") == string::npos &&
                    texName.find("shadow_quarter_square.ia8") == string::npos) {

                        if (texName.find("segment2.11C58.rgba16") != string::npos ||
                            texName.find("segment2.12C58.rgba16") != string::npos ||
                            texName.find("segment2.13C58.rgba16") != string::npos) {
                                return "textures/saturn/mario_logo.rgba16.png";
                        }

                }
            }
        }

        // Painting visibility
        bob_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        ccm_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        wf_painting.alpha =         (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        jrb_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        lll_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        ssl_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        hmc_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        ddd_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        wdw_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        thi_tiny_painting.alpha =   (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        ttm_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        ttc_painting.alpha =        (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        sl_painting.alpha =         (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;
        thi_huge_painting.alpha =   (autoChroma & !autoChromaObjects) ? 0x00 : 0xFF;

        if (autoChroma && !autoChromaObjects)  bob_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  ccm_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  wf_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  jrb_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  lll_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  ssl_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  hmc_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  ddd_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  wdw_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  thi_tiny_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  ttm_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  ttc_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  sl_painting.state = 1;
        if (autoChroma && !autoChromaObjects)  thi_huge_painting.state = 1;
    }

    if (texName.find("textures/skyboxes/cloud.") != string::npos)
        return texName.replace(18, 5, "cloud_floor");

    return texName;
}

/*
    Handles texture replacement. Called from gfx_pc.c
*/
std::map<std::string, std::string*> loaded_texture_paths = {};
const void* saturn_bind_texture(const void* input) {
    if (input == nullptr) return input;
    std::string output = saturn_bind_texture_internal(input);
    if (loaded_texture_paths.find(output) == loaded_texture_paths.end()) {
        loaded_texture_paths.insert({ output, new std::string(output) });
    }
    return loaded_texture_paths[output]->c_str();
}

void saturn_copy_file(string from, string to) {
    fs::path from_path(from);
    string final = "" + fs::current_path().generic_string() + "/" + to + from_path.filename().generic_string();

    fs::path final_path(final);
    // Convert TXT files to GS
    if (final_path.extension() == ".txt") {
        final = "" + fs::current_path().generic_string() + "/" + to + from_path.stem().generic_string() + ".gs";
    }

    std::cout << from << " - " << final << std::endl;

    // Skip existing files
    if (fs::exists(final))
        return;

    fs::copy(from, final, fs::copy_options::skip_existing);
}

void saturn_delete_file(string file) {
    remove(file.c_str());
}

std::size_t number_of_files_in_directory(std::filesystem::path path) {
    return (std::size_t)std::distance(std::filesystem::directory_iterator{path}, std::filesystem::directory_iterator{});
}