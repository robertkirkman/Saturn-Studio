#include <vector>
#include <iostream>
#include <string>
#include <map>

#include "game/area.h"
#include "saturn/imgui/saturn_imgui_chroma.h"
#include "saturn/imgui/saturn_imgui_machinima.h"
#include "saturn_format.h"

#include "saturn/saturn.h"
#include "saturn/saturn_actors.h"

extern "C" {
#include "engine/geo_layout.h"
#include "game/interaction.h"
#include "pc/gfx/gfx_pc.h"
#include "game/camera.h"
#include "engine/math_util.h"
#include "game/envfx_snow.h"
}

#include "saturn/saturn_timelines.h"

#define SATURN_PROJECT_VERSION 1

std::string current_project = "";
int project_load_timer = 0;

bool saturn_project_level_handler(SaturnFormatStream* stream, int version) {
    int level = saturn_format_read_int8(stream);
    int area = saturn_format_read_int8(stream);
    int acts = saturn_format_read_int8(stream);
    for (int i = 0; i < 6; i++) {
        enabled_acts[i] = (acts & (1 << i)) != 0;
    }
    if (gCurrLevelNum != levelList[level] || area != gCurrAreaIndex) {
        warp_to_level(level, area, 1);
        project_load_timer = 3;
        return false;
    }
    if (project_load_timer > 0) {
        project_load_timer--;
        return false;
    }
    current_project = "";
    return true;
}

bool saturn_project_game_environment_handler(SaturnFormatStream* stream, int version) {
    float cpx = saturn_format_read_float(stream);
    float cpy = saturn_format_read_float(stream);
    float cpz = saturn_format_read_float(stream);
    float crx = saturn_format_read_float(stream);
    float cry = saturn_format_read_float(stream);
    float crz = saturn_format_read_float(stream);
    vec3f_set(cameraPos, cpx, cpy, cpz);
    cameraYaw = cry;
    cameraPitch = crx;
    freezecamRoll = crz;
    camera_fov = saturn_format_read_float(stream);
    camera_focus = saturn_format_read_float(stream);
    camVelSpeed = saturn_format_read_float(stream);
    camVelRSpeed = saturn_format_read_float(stream);
    world_light_dir1 = saturn_format_read_float(stream);
    world_light_dir2 = saturn_format_read_float(stream);
    world_light_dir3 = saturn_format_read_float(stream);
    world_light_dir4 = saturn_format_read_float(stream);
    gLightingColor[0] = saturn_format_read_float(stream);
    gLightingColor[1] = saturn_format_read_float(stream);
    gLightingColor[2] = saturn_format_read_float(stream);
    uiChromaColor.x = saturn_format_read_float(stream);
    uiChromaColor.y = saturn_format_read_float(stream);
    uiChromaColor.z = saturn_format_read_float(stream);
    gravity = saturn_format_read_float(stream);
    configHUD = saturn_format_read_bool(stream);
    enable_shadows = saturn_format_read_bool(stream);
    enable_immunity = saturn_format_read_bool(stream);
    enable_dialogue = saturn_format_read_bool(stream);
    enable_dust_particles = saturn_format_read_bool(stream);
    time_freeze_state = saturn_format_read_int8(stream);
    gLevelEnv = saturn_format_read_int8(stream);
    return true;
}

bool saturn_project_autochroma_handler(SaturnFormatStream* stream, int version) {
    autoChroma = true;
    autoChromaLevel = saturn_format_read_bool(stream);
    autoChromaObjects = saturn_format_read_bool(stream);
    use_color_background = saturn_format_read_bool(stream);
    gChromaKeyBackground = saturn_format_read_int8(stream);
    return true;
}

bool saturn_project_mario_actor_handler(SaturnFormatStream* stream, int version) {
    char name[256];
    saturn_format_read_string(stream, name, 255);
    char modelname[256];
    saturn_format_read_string(stream, modelname, 255);
    float x = saturn_format_read_float(stream);
    float y = saturn_format_read_float(stream);
    float z = saturn_format_read_float(stream);
    MarioActor* actor = saturn_spawn_actor(x, y, z);
    memcpy(actor->name, name, 256);
    actor->angle = saturn_format_read_float(stream);
    actor->xScale = saturn_format_read_float(stream);
    actor->yScale = saturn_format_read_float(stream);
    actor->zScale = saturn_format_read_float(stream);
    actor->spin_speed = saturn_format_read_float(stream);
    actor->alpha = saturn_format_read_float(stream);
    actor->head_rot_x = saturn_format_read_int32(stream);
    actor->head_rot_y = saturn_format_read_int32(stream);
    actor->eye_state = saturn_format_read_int32(stream);
    actor->cap_state = saturn_format_read_int32(stream);
    actor->hand_state = saturn_format_read_int32(stream);
    actor->powerup_state = saturn_format_read_int32(stream);
    actor->cc_index = saturn_format_read_int32(stream);
    actor->input_recording_frame = saturn_format_read_int32(stream);
    actor->playback_input = saturn_format_read_bool(stream);
    actor->show_emblem = saturn_format_read_bool(stream);
    actor->spinning = saturn_format_read_bool(stream);
    actor->hidden = saturn_format_read_bool(stream);
    actor->cc_support = saturn_format_read_bool(stream);
    actor->spark_support = saturn_format_read_bool(stream);
    actor->custom_eyes = saturn_format_read_bool(stream);
    actor->custom_bone = saturn_format_read_bool(stream);
    actor->animstate.custom = saturn_format_read_bool(stream);
    actor->animstate.frame = saturn_format_read_int32(stream);
    actor->animstate.id = saturn_format_read_int32(stream);
    actor->selected_model = saturn_format_read_bool(stream) - 1;
    for (int cc = 0; cc < 12; cc++) {
        for (int shade = 0; shade < 2; shade++) {
            actor->colorcode[cc].red  [shade] = saturn_format_read_int8(stream);
            actor->colorcode[cc].green[shade] = saturn_format_read_int8(stream);
            actor->colorcode[cc].blue [shade] = saturn_format_read_int8(stream);
        }
    }
    if (actor->selected_model != -1) {
        for (int i = 0; i < model_list.size(); i++) {
            if (model_list[i].Name == modelname) {
                actor->selected_model = i;
                actor->model = model_list[i];
                break;
            }
        }
        actor->model.Expressions.clear();
        actor->model.Expressions = LoadExpressions(&actor->model, actor->model.FolderPath);
    }
    for (int i = 0; i < actor->model.Expressions.size(); i++) {
        char expr[256];
        saturn_format_read_string(stream, expr, 255);
        actor->model.Expressions[i].CurrentIndex = 0;
        for (int j = 0; j < actor->model.Expressions[i].Textures.size(); j++) {
            fs_relative::path path = actor->model.Expressions[i].Textures[j].FilePath;
            fs_relative::path base = actor->model.Expressions[i].FolderPath;
            if (fs_relative::relative(path, base) == expr) {
                actor->model.Expressions[i].CurrentIndex = j;
                break;
            }
        }
    }
    for (int i = 0; i < 20; i++) {
        actor->bones[i][0] = saturn_format_read_float(stream);
        actor->bones[i][1] = saturn_format_read_float(stream);
        actor->bones[i][2] = saturn_format_read_float(stream);
    }
    int numFrames = saturn_format_read_int32(stream);
    for (int i = 0; i < numFrames; i++) {
        InputRecordingFrame frame;
        saturn_format_read_any(stream, &frame, sizeof(InputRecordingFrame));
        actor->input_recording.push_back(frame);
    }
    if (actor->animstate.custom) {
        if (actor->animstate.id >= canim_array.size()) {
            actor->animstate.custom = false;
            actor->animstate.frame = 0;
            actor->animstate.id = MARIO_ANIM_A_POSE;
        }
        else saturn_read_mcomp_animation(actor, canim_array[actor->animstate.id]);
    }
    return true;
}

bool saturn_project_keyframe_timeline_handler(SaturnFormatStream* stream, int version) {
    char id[256];
    saturn_format_read_string(stream, id, 255);
    int numKeyframes = saturn_format_read_int32(stream);
    int marioIndex = saturn_format_read_int32(stream);
    if (marioIndex != -1) id[strlen(id) - 8] = 0;
    auto [ptr, type, behavior, name, precision, num_values, is_mario] = timelineDataTable[id];
    KeyframeTimeline timeline;
    timeline.behavior = behavior;
    timeline.dest = ptr;
    timeline.marioIndex = marioIndex;
    timeline.name = name;
    timeline.numValues = num_values;
    timeline.precision = precision;
    timeline.type = type;
    std::vector<Keyframe> keyframes = {};
    for (int i = 0; i < numKeyframes; i++) {
        Keyframe keyframe;
        for (int j = 0; j < num_values; j++) {
            keyframe.value.push_back(saturn_format_read_float(stream));
        }
        keyframe.curve = (InterpolationCurve)saturn_format_read_int8(stream);
        keyframe.timelineID = id;
        keyframe.position = saturn_format_read_int32(stream);
        keyframes.push_back(keyframe);
    }
    k_frame_keys.insert({ id, { timeline, keyframes } });
    return true;
}

bool saturn_project_custom_anim_handler(SaturnFormatStream* stream, int version) {
    canim_array.clear();
    while (true) {
        char entry[256];
        saturn_format_read_string(stream, entry, 255);
        if (entry[0]) canim_array.push_back(entry);
        else break;
    }
    return true;
}

void saturn_load_project(char* filename) {
    k_frame_keys.clear();
    saturn_clear_actors();
    current_project = filename;
    saturn_format_input((char*)(std::string(sys_user_path()) + std::string("/dynos/projects/") + filename).c_str(), "SSPJ", {
        { "GENV", saturn_project_game_environment_handler },
        { "ACHR", saturn_project_autochroma_handler },
        { "MACT", saturn_project_mario_actor_handler },
        { "KFTL", saturn_project_keyframe_timeline_handler },
        { "LEVL", saturn_project_level_handler },
        { "CANM", saturn_project_custom_anim_handler },
    });
    for (auto& entry : k_frame_keys) {
        saturn_keyframe_apply(entry.first, k_current_frame);
    }
    std::cout << "Loaded project " << filename << std::endl;
}

void saturn_save_project(char* filename) {
    SaturnFormatStream _stream = saturn_format_output("SSPJ", SATURN_PROJECT_VERSION);
    SaturnFormatStream* stream = &_stream;
    saturn_format_new_section(stream, "LEVL");
    saturn_format_write_int8(stream, get_saturn_level_id(gCurrLevelNum));
    saturn_format_write_int8(stream, gCurrAreaIndex);
    int acts = 0;
    for (int i = 0; i < 6; i++) {
        acts |= (enabled_acts[i] << i);
    }
    saturn_format_write_int8(stream, acts);
    saturn_format_close_section(stream);
    saturn_format_new_section(stream, "CANM");
    for (std::string canim : canim_array) {
        saturn_format_write_string(stream, canim.data());
    }
    saturn_format_write_int8(stream, 0);
    saturn_format_close_section(stream);
    saturn_format_new_section(stream, "GENV");
    saturn_format_write_float(stream, cameraPos[0]);
    saturn_format_write_float(stream, cameraPos[1]);
    saturn_format_write_float(stream, cameraPos[2]);
    saturn_format_write_float(stream, cameraPitch);
    saturn_format_write_float(stream, cameraYaw);
    saturn_format_write_float(stream, freezecamRoll);
    saturn_format_write_float(stream, camera_fov);
    saturn_format_write_float(stream, camera_focus);
    saturn_format_write_float(stream, camVelSpeed);
    saturn_format_write_float(stream, camVelRSpeed);
    saturn_format_write_float(stream, world_light_dir1);
    saturn_format_write_float(stream, world_light_dir2);
    saturn_format_write_float(stream, world_light_dir3);
    saturn_format_write_float(stream, world_light_dir4);
    saturn_format_write_float(stream, gLightingColor[0]);
    saturn_format_write_float(stream, gLightingColor[1]);
    saturn_format_write_float(stream, gLightingColor[2]);
    saturn_format_write_float(stream, uiChromaColor.x);
    saturn_format_write_float(stream, uiChromaColor.y);
    saturn_format_write_float(stream, uiChromaColor.z);
    saturn_format_write_float(stream, gravity);
    saturn_format_write_bool(stream, configHUD);
    saturn_format_write_bool(stream, enable_shadows);
    saturn_format_write_bool(stream, enable_immunity);
    saturn_format_write_bool(stream, enable_dialogue);
    saturn_format_write_bool(stream, enable_dust_particles);
    saturn_format_write_int8(stream, time_freeze_state);
    saturn_format_write_int8(stream, gLevelEnv);
    saturn_format_close_section(stream);
    if (autoChroma) {
        saturn_format_new_section(stream, "ACHR");
        saturn_format_write_bool(stream, autoChromaLevel);
        saturn_format_write_bool(stream, autoChromaObjects);
        saturn_format_write_bool(stream, use_color_background);
        saturn_format_write_int8(stream, gChromaKeyBackground);
        saturn_format_close_section(stream);
    }
    MarioActor* actor = saturn_get_actor(0);
    while (actor) {
        if (!actor->exists) {
            actor = actor->next;
            continue;
        }
        saturn_format_new_section(stream, "MACT");
        saturn_format_write_string(stream, actor->name);
        if (actor->selected_model == -1) saturn_format_write_int8(stream, 0);
        else saturn_format_write_string(stream, (char*)actor->model.Name.c_str());
        saturn_format_write_float(stream, actor->x);
        saturn_format_write_float(stream, actor->y);
        saturn_format_write_float(stream, actor->z);
        saturn_format_write_float(stream, actor->angle);
        saturn_format_write_float(stream, actor->xScale);
        saturn_format_write_float(stream, actor->yScale);
        saturn_format_write_float(stream, actor->zScale);
        saturn_format_write_float(stream, actor->spin_speed);
        saturn_format_write_float(stream, actor->alpha);
        saturn_format_write_int32(stream, actor->head_rot_x);
        saturn_format_write_int32(stream, actor->head_rot_y);
        saturn_format_write_int32(stream, actor->eye_state);
        saturn_format_write_int32(stream, actor->cap_state);
        saturn_format_write_int32(stream, actor->hand_state);
        saturn_format_write_int32(stream, actor->powerup_state);
        saturn_format_write_int32(stream, actor->cc_index);
        saturn_format_write_int32(stream, actor->input_recording_frame);
        saturn_format_write_bool(stream, actor->playback_input);
        saturn_format_write_bool(stream, actor->show_emblem);
        saturn_format_write_bool(stream, actor->spinning);
        saturn_format_write_bool(stream, actor->hidden);
        saturn_format_write_bool(stream, actor->cc_support);
        saturn_format_write_bool(stream, actor->spark_support);
        saturn_format_write_bool(stream, actor->custom_eyes);
        saturn_format_write_bool(stream, actor->custom_bone);
        saturn_format_write_bool(stream, actor->animstate.custom);
        saturn_format_write_int32(stream, actor->animstate.frame);
        saturn_format_write_int32(stream, actor->animstate.id);
        saturn_format_write_bool(stream, actor->selected_model != -1);
        for (int cc = 0; cc < 12; cc++) {
            for (int shade = 0; shade < 2; shade++) {
                saturn_format_write_int8(stream, actor->colorcode[cc].red  [shade]);
                saturn_format_write_int8(stream, actor->colorcode[cc].green[shade]);
                saturn_format_write_int8(stream, actor->colorcode[cc].blue [shade]);
            }
        }
        for (int i = 0; i < actor->model.Expressions.size(); i++) {
            fs_relative::path path = actor->model.Expressions[i].Textures[actor->model.Expressions[i].CurrentIndex].FilePath;
            fs_relative::path base = actor->model.Expressions[i].FolderPath;
            saturn_format_write_string(stream, (char*)fs_relative::relative(path, base).string().c_str());
        }
        for (int i = 0; i < 20; i++) {
            saturn_format_write_float(stream, actor->bones[i][0]);
            saturn_format_write_float(stream, actor->bones[i][1]);
            saturn_format_write_float(stream, actor->bones[i][2]);
        }
        saturn_format_write_int32(stream, actor->input_recording.size());
        saturn_format_write_any(stream, actor->input_recording.data(), sizeof(InputRecordingFrame) * actor->input_recording.size());
        saturn_format_close_section(stream);
        actor = actor->next;
    }
    for (auto& entry : k_frame_keys) {
        saturn_format_new_section(stream, "KFTL");
        saturn_format_write_string(stream, (char*)entry.first.c_str());
        saturn_format_write_int32(stream, entry.second.second.size());
        saturn_format_write_int32(stream, entry.second.first.marioIndex);
        for (auto& kf : entry.second.second) {
            for (int i = 0; i < entry.second.first.numValues; i++) {
                saturn_format_write_float(stream, kf.value[i]);
            }
            saturn_format_write_int8(stream, kf.curve);
            saturn_format_write_int32(stream, kf.position);
        }
        saturn_format_close_section(stream);
    }
    saturn_format_write((char*)(std::string(sys_user_path()) + std::string("/dynos/projects/") + filename).c_str(), stream);
}

std::string project_dir;
std::vector<std::string> project_array;

void saturn_load_project_list() {
    project_array.clear();
    //project_array.push_back("autosave.spj");

    #ifdef __MINGW32__
        // windows moment
        project_dir = "dynos\\projects\\";
    #else
        project_dir = std::string(sys_user_path()) + "/dynos/projects/";
    #endif

    if (!fs::exists(project_dir))
        return;

    for (const auto & entry : fs::directory_iterator(project_dir)) {
        fs::path path = entry.path();

        if (path.filename().u8string() != "autosave.spj") {
            if (path.extension().u8string() == ".spj")
                project_array.push_back(path.filename().u8string());
        }
    }
}