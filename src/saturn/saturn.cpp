#include "saturn.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <thread>
#include <map>
#include <SDL2/SDL.h>

#include "data/dynos.cpp.h"
#include "engine/math_util.h"
#include "saturn/imgui/saturn_imgui.h"
#include "saturn/imgui/saturn_imgui_machinima.h"
#include "saturn/imgui/saturn_imgui_chroma.h"
#include "libs/sdl2_scancode_to_dinput.h"
#include "pc/configfile.h"
#include "saturn/filesystem/saturn_projectfile.h"
#include "saturn/imgui/saturn_imgui_dynos.h"
#include "saturn/filesystem/saturn_locationfile.h"
#include "data/dynos.cpp.h"
#include "saturn/filesystem/saturn_animfile.h"
#include "saturn/saturn_animations.h"
#include "saturn/saturn_colors.h"
#include "saturn/saturn_rom_extract.h"
#include "saturn/saturn_timelines.h"
#include "saturn/saturn_actors.h"

extern "C" {
#include "audio/external.h"
#include "engine/surface_collision.h"
#include "game/object_collision.h"
}

bool mario_exists;

bool camera_frozen = true;
float camera_speed = 0.0f;
float camera_focus = 1.f;
float camera_savestate_mult = 1.f;
bool camera_fov_smooth = false;
bool is_camera_moving;

bool camera_view_enabled;
bool camera_view_moving;
bool camera_view_zooming;
bool camera_view_rotating;
int camera_view_move_x;
int camera_view_move_y;

struct MouseState mouse_state;
struct MouseState prev_mouse_state;

bool enable_head_rotations = false;
bool enable_shadows = false;
bool enable_dust_particles = false;
bool enable_torso_rotation = true;
bool enable_fog = true;
float run_speed = 127.0f;
bool can_fall_asleep = false;
int saturnModelState = 0;
bool linkMarioScale = true;
bool is_spinning;
float spin_mult = 1.0f;

bool using_chainer;
int chainer_index;
enum MarioAnimID selected_animation = MARIO_ANIM_BREAKDANCE;
int current_anim_frame;
int current_anim_id;
int current_anim_length;
bool is_anim_playing = false;
bool is_anim_paused = false;
int paused_anim_frame;
struct AnimationState current_animation = {
    .custom = false,
    .id = MarioAnimID::MARIO_ANIM_RUNNING,
};

float this_face_angle;

bool limit_fps = true;

// discord
bool has_discord_init;

// private
bool is_chroma_keying = false;
bool prev_quicks[3];
int lastCourseNum = -1;
int saturn_launch_timer;

float* active_key_float_value = &camera_fov;
bool* active_key_bool_value;

s32 active_data_type = KEY_FLOAT;
bool keyframe_playing;
bool k_popout_open;
bool k_popout_focused;
int mcam_timer = 0;
int k_current_frame = 0;
int k_previous_frame = 0;
int k_curr_curve_type = 0;

int k_current_anim = -1;

std::map<std::string, std::pair<KeyframeTimeline, std::vector<Keyframe>>> k_frame_keys = {};

int k_last_passed_index = 0;
int k_distance_between;
int k_current_distance;
float k_static_increase_value;
int k_last_placed_frame;
bool k_loop;
bool k_animating_camera;
float k_c_pos0_incr;
float k_c_pos1_incr;
float k_c_pos2_incr;
float k_c_foc0_incr;
float k_c_foc1_incr;
float k_c_foc2_incr;
float k_c_rot0_incr;
float k_c_rot1_incr;
float k_c_rot2_incr;
bool has_set_initial_k_frames;

bool is_cc_editing;

bool autoChroma;
bool autoChromaLevel;
bool autoChromaObjects;

u8 activatedToads = 0;

f32 mario_headrot_yaw = 0;
f32 mario_headrot_pitch = 0;
f32 mario_headrot_speed = 10.0f;

extern "C" {
#include "game/camera.h"
#include "game/area.h"
#include "game/level_update.h"
#include "engine/level_script.h"
#include "game/game_init.h"
#include "data/dynos.h"
#include "pc/configfile.h"
#include "game/mario.h"
#include <mario_animation_ids.h>
#include <sm64.h>
#include "pc/controller/controller_keyboard.h"
#include "pc/cheats.h"
#include "game/save_file.h"
}

using namespace std;

#define FREEZE_CAMERA	    0x0800
#define CYCLE_EYE_STATE     0x0100
#define LOAD_ANIMATION      0x0200
#define TOGGLE_MENU         0x0400

unsigned int chromaKeyColorR = 0;
unsigned int chromaKeyColorG = 255;
unsigned int chromaKeyColorB = 0;

int autosaveDelay = -1;

u16 gChromaKeyColor = 0x07C1;
u16 gChromaKeyBackground = 0;

u8 godmode_temp_off = false;

bool extract_thread_began = false;
bool extraction_finished = false;
float extraction_progress = -1;
int extract_return_code = 0;

int marios_spawned = 0;

extern void saturn_run_chainer();

float key_increase_val(std::vector<float> vecfloat) {
    float next_val = vecfloat.at(k_last_passed_index + 1);
    float this_val = vecfloat.at(k_last_passed_index);

    return (next_val - this_val) / k_distance_between;
}

bool timeline_has_id(std::string id) {
    if (k_frame_keys.size() > 0) {
        for (auto& entry : k_frame_keys) {
            for (Keyframe keyframe : entry.second.second) {
                if (keyframe.timelineID == id)
                    return true;
            }
        }
    }

    return false;
}

// SATURN Machinima Functions

float inpreccam_distfrommario = 500.f;
s16 inpreccam_yaw = 0;
s16 inpreccam_pitch = 0;
Vec3f inpreccam_pos;
Vec3f inpreccam_focus;

void saturn_update() {

    // Keybinds

    if (mario_exists) {
        if (!saturn_disable_sm64_input()) {
            if (gPlayer1Controller->buttonPressed & D_JPAD) {
                saturn_actor_stop_recording();
            }
        }
    }

    bool mouse_l, mouse_r;
    prev_mouse_state = mouse_state;
    ImGuiIO& io = ImGui::GetIO();
    mouse_state.x = io.MousePos.x;
    mouse_state.y = io.MousePos.y;
    mouse_state.held = 0;
    for (int i = 0; i < 5; i++) {
        mouse_state.held |= io.MouseDown[i] << i;
    }
    mouse_state.pressed = mouse_state.held & ~prev_mouse_state.held;
    mouse_state.released = ~mouse_state.held & prev_mouse_state.held;
    mouse_state.x_diff = mouse_state.x - prev_mouse_state.x;
    mouse_state.y_diff = mouse_state.y - prev_mouse_state.y;

    // Machinima

    machinimaMode = (camera_frozen) ? 1 : 0;
    machinimaKeyframing = (keyframe_playing && active_data_type == KEY_CAMERA);

    bool is_focused = is_focused_on_game();
    if (mouse_state.pressed & (MOUSEBTN_MASK_L | MOUSEBTN_MASK_R)) {
        mouse_state.x_orig = mouse_state.x;
        mouse_state.y_orig = mouse_state.y;
        mouse_state.focused_on_game = is_focused;
        if (mouse_state.focused_on_game) mouse_state.update_camera = true;
    }
    if (mouse_state.held & (MOUSEBTN_MASK_L | MOUSEBTN_MASK_R))
        mouse_state.dist_travelled = sqrt(
            (mouse_state.x - mouse_state.x_orig) * (mouse_state.x - mouse_state.x_orig) +
            (mouse_state.y - mouse_state.y_orig) * (mouse_state.y - mouse_state.y_orig)
        );
    else mouse_state.update_camera = false;

    if (!saturn_disable_sm64_input()) {
        cameraRollLeft  = SDL_GetKeyboardState(NULL)[SDL_SCANCODE_V];
        cameraRollRight = SDL_GetKeyboardState(NULL)[SDL_SCANCODE_B];
        mouse_state.scrollwheel_modifier = 1.00f;
        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LSHIFT]) mouse_state.scrollwheel_modifier = 0.25f;
        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LCTRL ]) mouse_state.scrollwheel_modifier = 2.00f;
    }
    else mouse_state.scrollwheel_modifier = 1.f;

    if (!keyframe_playing && !camera_frozen) {
        gLakituState.focHSpeed = camera_focus * camera_savestate_mult * 0.8f;
        gLakituState.focVSpeed = camera_focus * camera_savestate_mult * 0.3f;
        gLakituState.posHSpeed = camera_focus * camera_savestate_mult * 0.3f;
        gLakituState.posVSpeed = camera_focus * camera_savestate_mult * 0.3f;
    }

    camera_default_fov = camera_fov + 5.0f;

    //SDL_GetMouseState(&camera_view_move_x, &camera_view_move_y);

    if (gCurrLevelNum == LEVEL_SA || autoChroma) {
        if (!is_chroma_keying) is_chroma_keying = true;
    }

    //if (gCurrLevelNum == LEVEL_SA && !is_chroma_keying) {
        //is_chroma_keying = true;
        // Called once when entering Chroma Key Stage
        //prev_quicks[0] = enable_shadows;
        //prev_quicks[1] = enable_dust_particles;
        //prev_quicks[2] = configHUD;
        //enable_shadows = false;
        //enable_dust_particles = false;
        //configHUD = false;
    //}
    if (gCurrLevelNum != LEVEL_SA && !autoChroma) {
        if (!is_chroma_keying) is_chroma_keying = false;
        // Called once when exiting Chroma Key Stage
        //enable_shadows = prev_quicks[0];
        //enable_dust_particles = prev_quicks[1];
        //configHUD = prev_quicks[2];
    }

    if (splash_finished) saturn_launch_timer++;
    if (gCurrLevelNum == LEVEL_SA && saturn_launch_timer <= 1 && splash_finished) {
        gMarioState->faceAngle[1] = 0;
        if (gCamera) { // i hate the sm64 camera system aaaaaaaaaaaaaaaaaa
            float dist = 0;
            s16 yaw, pitch;
            vec3f_set(gCamera->pos, 0.f, 192.f, 264.f);
            vec3f_set(gCamera->focus, 0.f, 181.f, 28.f);
            vec3f_copy(freezecamPos, gCamera->pos);
            vec3f_get_dist_and_angle(gCamera->pos, gCamera->focus, &dist, &pitch, &yaw);
            freezecamYaw = (float)yaw;
            freezecamPitch = (float)pitch;
            vec3f_copy(gLakituState.pos, gCamera->pos);
            vec3f_copy(gLakituState.focus, gCamera->focus);
            vec3f_copy(gLakituState.goalPos, gCamera->pos);
            vec3f_copy(gLakituState.goalFocus, gCamera->focus);
            gCamera->yaw = calculate_yaw(gCamera->focus, gCamera->pos);
            gLakituState.yaw = gCamera->yaw;
        }
    }

    // Keyframes

    if (!k_popout_open) k_popout_focused = false;
    if (keyframe_playing) {
        if (timeline_has_id("k_c_camera_pos0")) {
            // Prevents smoothing for sharper, more consistent panning
            gLakituState.focHSpeed = 15.f * camera_focus * 0.8f;
            gLakituState.focVSpeed = 15.f * camera_focus * 0.3f;
            gLakituState.posHSpeed = 15.f * camera_focus * 0.3f;
            gLakituState.posVSpeed = 15.f * camera_focus * 0.3f;
        }
        
        bool end = true;
        for (const auto& entry : k_frame_keys) {
            if (!saturn_keyframe_apply(entry.first, k_current_frame)) end = false;
        }
        if (end) {
            if (saturn_imgui_is_capturing_video()) saturn_imgui_stop_capture();
            else if (k_loop) k_current_frame = 0;
        }

        if (timeline_has_id("k_angle"))
            gMarioState->faceAngle[1] = (s16)(this_face_angle * 182.04f);

        k_current_frame++;
    }

    // Camera

    f32 dist;
    if (mouse_state.update_camera) {
        if (saturn_actor_is_recording_input()) {
            if (mouse_state.held & (MOUSEBTN_MASK_L | MOUSEBTN_MASK_R)) {
                inpreccam_yaw   += mouse_state.x_diff * 50 * camVelRSpeed;
                inpreccam_pitch -= mouse_state.y_diff * 50 * camVelRSpeed;
            }
        }
        else {
            Vec3f offset;
            vec3f_set(offset, 0, 0, 0);
            if (mouse_state.held & MOUSEBTN_MASK_L) {
                offset[0] += sins(freezecamYaw + atan2s(0, 127)) * mouse_state.x_diff * camVelSpeed;
                offset[2] += coss(freezecamYaw + atan2s(0, 127)) * mouse_state.x_diff * camVelSpeed;
                offset[1] += coss(freezecamPitch) * mouse_state.y_diff * camVelSpeed;
                offset[0] += sins(freezecamPitch) * coss(freezecamYaw + atan2s(0, 127)) * mouse_state.y_diff * camVelSpeed;
                offset[2] -= sins(freezecamPitch) * sins(freezecamYaw + atan2s(0, 127)) * mouse_state.y_diff * camVelSpeed;
            }
            if (mouse_state.held & MOUSEBTN_MASK_R) {
                freezecamYaw   += mouse_state.x_diff * 20 * camVelRSpeed;
                freezecamPitch += mouse_state.y_diff * 20 * camVelRSpeed;
            }
            vec3f_add(freezecamPos, offset);
        }
    }
    if (saturn_actor_is_recording_input()) {
        inpreccam_distfrommario += mouse_state.scrollwheel * 200 * mouse_state.scrollwheel_modifier;
        MarioActor* actor = saturn_get_actor(recording_mario_actor);
        if (actor != nullptr) {
            InputRecordingFrame last = actor->input_recording[actor->input_recording.size() - 1];
            vec3f_set(inpreccam_focus, last.x, last.y + 80, last.z);
            vec3f_set_dist_and_angle(inpreccam_focus, inpreccam_pos, inpreccam_distfrommario, inpreccam_pitch, inpreccam_yaw);
        }
    }
    else vec3f_set_dist_and_angle(freezecamPos, freezecamPos, mouse_state.scrollwheel * 200 * mouse_state.scrollwheel_modifier * camVelSpeed, freezecamPitch, freezecamYaw);
    mouse_state.scrollwheel = 0;

    if (cameraRollLeft) freezecamRoll += camVelRSpeed * 512;
    if (cameraRollRight) freezecamRoll -= camVelRSpeed * 512;

    if (gCamera) {
        if (saturn_actor_is_recording_input()) {
            vec3f_copy(gCamera->pos, inpreccam_pos);
            vec3f_copy(gCamera->focus, inpreccam_focus);
        }
        else {
            vec3f_copy(gCamera->pos, freezecamPos);
            vec3f_set_dist_and_angle(gCamera->pos, gCamera->focus, 100, freezecamPitch, freezecamYaw);
        }
        vec3f_copy(gLakituState.pos, gCamera->pos);
        vec3f_copy(gLakituState.focus, gCamera->focus);
        vec3f_copy(gLakituState.goalPos, gCamera->pos);
        vec3f_copy(gLakituState.goalFocus, gCamera->focus);
        gCamera->yaw = calculate_yaw(gCamera->focus, gCamera->pos);
        gLakituState.yaw = gCamera->yaw;
        gLakituState.roll = freezecamRoll;
    }

    // Animations

    /*if (mario_exists) {
        if (is_anim_paused) {
            gMarioState->marioObj->header.gfx.unk38.animFrame = current_anim_frame;
            gMarioState->marioObj->header.gfx.unk38.animFrameAccelAssist = current_anim_frame;
        } else if (is_anim_playing) {
            if (current_animation.hang) {
                if (is_anim_past_frame(gMarioState, (int)gMarioState->marioObj->header.gfx.unk38.curAnim->unk08 - 1)) {
                    is_anim_paused = !is_anim_paused;
                }
            }

            if (is_anim_past_frame(gMarioState, (int)gMarioState->marioObj->header.gfx.unk38.curAnim->unk08) || is_anim_at_end(gMarioState)) {
                if (current_animation.loop && !using_chainer) {
                    gMarioState->marioObj->header.gfx.unk38.animFrame = 0;
                    gMarioState->marioObj->header.gfx.unk38.animFrameAccelAssist = 0;
                } else {
                    if (using_chainer) {
                        chainer_index++;
                    } else {
                        if (gMarioState->action == ACT_DEBUG_FREE_MOVE)
                            set_mario_animation(gMarioState, MARIO_ANIM_A_POSE);
                        is_anim_playing = false;
                        is_anim_paused = false;
                    }
                }
            }

            if (selected_animation != gMarioState->marioObj->header.gfx.unk38.animID) {
                is_anim_playing = false;
                is_anim_paused = false;
            }

            current_anim_id = (int)gMarioState->marioObj->header.gfx.unk38.animID;
            if (gMarioState->action == ACT_IDLE || gMarioState->action == ACT_FIRST_PERSON || gMarioState->action == ACT_DEBUG_FREE_MOVE) {
                current_anim_frame = (int)gMarioState->marioObj->header.gfx.unk38.animFrame;
                current_anim_length = (int)gMarioState->marioObj->header.gfx.unk38.curAnim->unk08 - 1;
            }

            if (current_animation.speed != 1.0f)
                gMarioState->marioObj->header.gfx.unk38.animAccel = current_animation.speed * 65535;

            if (using_chainer && is_anim_playing) saturn_run_chainer();
        }
    }*/

    if (mouse_state.dist_travelled <= 3 && mouse_state.released && mouse_state.focused_on_game && !saturn_actor_is_recording_input()) {
        Vec3f dir, hit;
        s16 yaw, pitch;
        float dist;
        float x = (mouse_state.x - game_viewport[0]) / game_viewport[2];
        float y = (mouse_state.y - game_viewport[1]) / game_viewport[3];
        struct Surface* surface = nullptr;
        vec3f_get_dist_and_angle(gCamera->pos, gCamera->focus, &dist, &pitch, &yaw);
        get_raycast_dir(dir, yaw, pitch, camera_fov, gfx_current_dimensions.aspect_ratio, x, y);
        vec3f_mul(dir, 8000);
        if (mouse_state.released & MOUSEBTN_MASK_L) {
            find_surface_on_ray(gCamera->pos, dir, &surface, hit);
            vec3f_get_dist_and_angle(hit, gCamera->pos, &dist, &pitch, &yaw);
            MarioActor* actor = saturn_spawn_actor(hit[0], hit[1], hit[2]);
            actor->angle = yaw;
            std::string name = "Unnamed Mario " + std::to_string(++marios_spawned);
            memcpy(actor->name, name.c_str(), name.length() + 1);
        }
        if (mouse_state.released & MOUSEBTN_MASK_R) {
            struct Object* obj = get_mario_actor_from_ray(gCamera->pos, dir);
            if (obj) {
                if (obj->oMarioActorIndex >= 0 && obj->oMarioActorIndex < saturn_actor_sizeof()) {
                    saturn_imgui_open_mario_menu(obj->oMarioActorIndex);
                }
            }
        }
    }

    // Misc

    mario_exists = (gMarioState->action != ACT_UNINITIALIZED & sCurrPlayMode != 2 & mario_loaded);

    if (!mario_exists) {
        is_anim_playing = false;
        is_anim_paused = false;
    }

    switch(saturnModelState) {
        case 0:     scrollModelState = 0;       break;
        case 1:     scrollModelState = 0x200;   break;  // Metal Cap
        case 2:     scrollModelState = 0x180;   break;  // Vanish Cap
        case 3:     scrollModelState = 0x380;   break;  // Both
        default:    scrollModelState = 0;       break;
    }

    if (linkMarioScale) {
        marioScaleSizeY = marioScaleSizeX;
        marioScaleSizeZ = marioScaleSizeX;
    }

    if (is_spinning && mario_exists) {
        gMarioState->faceAngle[1] += (s16)(spin_mult * 15 * 182.04f);
    }

    if (current_project != "") saturn_load_project((char*)current_project.c_str());

    // Autosave

    if (gCurrLevelNum != LEVEL_SA || gCurrAreaIndex != 3) {
        if (autosaveDelay <= 0) autosaveDelay = 30 * configAutosaveDelay;
        autosaveDelay--;
        if (autosaveDelay == 0) saturn_save_project("autosave.spj");
    }
}

void* saturn_keyframe_get_timeline_ptr(KeyframeTimeline& timeline) {
    if (timeline.marioIndex == -1) return timeline.dest;
    return (char*)saturn_get_actor(timeline.marioIndex) + (size_t)timeline.dest;
    // cast to char since its 1 byte long
}

float saturn_keyframe_setup_interpolation(std::string id, int frame, int* keyframe, bool* last) {
    KeyframeTimeline timeline = k_frame_keys[id].first;
    std::vector<Keyframe> keyframes = k_frame_keys[id].second;

    // Get the keyframe to interpolate from
    for (int i = 0; i < keyframes.size(); i++) {
        if (frame < keyframes[i].position) break;
        *keyframe = i;
    }

    // Stop/loop if reached the end
    *last = *keyframe + 1 == keyframes.size();
    if (*last) *keyframe -= 1; // Assign values from final keyframe

    // Interpolate, formulas from easings.net
    float x = (frame - keyframes[*keyframe].position) / (float)(keyframes[*keyframe + 1].position - keyframes[*keyframe].position);
    if (*last) x = 1;
    else if (keyframes[*keyframe].curve == InterpolationCurve::SLOW) x = x * x;
    else if (keyframes[*keyframe].curve == InterpolationCurve::FAST) x = 1 - (1 - x) * (1 - x);
    else if (keyframes[*keyframe].curve == InterpolationCurve::SMOOTH) x = x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;
    else if (keyframes[*keyframe].curve == InterpolationCurve::WAIT) x = floor(x);

    return x;
}

// applies the values from keyframes to its destination, returns true if its the last frame, false if otherwise
bool saturn_keyframe_apply(std::string id, int frame) {
    KeyframeTimeline timeline = k_frame_keys[id].first;
    std::vector<Keyframe> keyframes = k_frame_keys[id].second;

    void* ptr = saturn_keyframe_get_timeline_ptr(timeline);
    std::vector<float> values;
    bool last = true;
    if (keyframes.size() == 1) values = keyframes[0].value;
    else {
        int keyframe = 0;
        last = false;
        float x = saturn_keyframe_setup_interpolation(id, frame, &keyframe, &last);
        for (int i = 0; i < keyframes[keyframe].value.size(); i++) {
            values.push_back((keyframes[keyframe + 1].value[i] - keyframes[keyframe].value[i]) * x + keyframes[keyframe].value[i]);
        }    
    }

    if (timeline.type == KFTYPE_BOOL) *(bool*)ptr = values[0] >= 1;
    if (timeline.type == KFTYPE_FLOAT || timeline.type == KFTYPE_COLORF) {
        float* vals = (float*)ptr;
        for (int i = 0; i < timeline.numValues; i++) {
            *vals = values[i];
            vals++;
        }
    }
    if (timeline.type == KFTYPE_COLOR) {
        int* colors = (int*)ptr;
        for (int i = 0; i < 6; i++) {
            *colors = values[i];
            colors++;
        }
    }
    if (timeline.type == KFTYPE_ANIM) {
        AnimationState* dest = (AnimationState*)ptr;
        dest->custom = values[0] >= 1;
        dest->id = values[1];
    }
    if (timeline.type == KFTYPE_EXPRESSION) {
        Model* dest = (Model*)ptr;
        for (int i = 0; i < values.size(); i++) {
            dest->Expressions[i].CurrentIndex = values[i];
        }
    }
    if (timeline.type == KFTYPE_SWITCH) {
        *((int*)ptr) = values[0];
    }

    return last;
}

// returns true if the value is the same as if the keyframe was applied
bool saturn_keyframe_matches(std::string id, int frame) {
    KeyframeTimeline& timeline = k_frame_keys[id].first;
    std::vector<Keyframe> keyframes = k_frame_keys[id].second;

    std::vector<float> expectedValues;
    if (keyframes.size() == 1) expectedValues = keyframes[0].value;
    else {
        int keyframe = 0;
        bool last = false;
        float x = saturn_keyframe_setup_interpolation(id, frame, &keyframe, &last);
        for (int i = 0; i < keyframes[keyframe].value.size(); i++) {
            expectedValues.push_back((keyframes[keyframe + 1].value[i] - keyframes[keyframe].value[i]) * x + keyframes[keyframe].value[i]);
        }
    }

    void* ptr = saturn_keyframe_get_timeline_ptr(timeline);
    if (timeline.type == KFTYPE_BOOL) {
        if (*(bool*)ptr != 0 != expectedValues[0] >= 1) return false;
        return true;
    }
    if (timeline.type == KFTYPE_FLOAT || timeline.type == KFTYPE_COLORF) {
        for (int i = 0; i < timeline.numValues; i++) {
            float value = ((float*)ptr)[i];
            float distance = abs(value - expectedValues[i]);
            if (distance > pow(10, timeline.precision)) return false;
        }
    }
    if (timeline.type == KFTYPE_COLOR) {
        for (int i = 0; i < 6; i++) {
            int value = ((int*)ptr)[i];
            if (value != (int)expectedValues[i]) return false;
        }
    }
    if (timeline.type == KFTYPE_ANIM) {
        AnimationState* anim_state = (AnimationState*)ptr;
        if (anim_state->custom != (expectedValues[0] >= 1)) return false;
        if (anim_state->id != ((int)expectedValues[1])) return false;
    }
    if (timeline.type == KFTYPE_EXPRESSION) {
        Model* model = (Model*)ptr;
        for (int i = 0; i < model->Expressions.size(); i++) {
            if (model->Expressions[i].CurrentIndex != (int)expectedValues[i]) return false;
        }
    }
    if (timeline.type == KFTYPE_SWITCH) {
        return *((int*)ptr) == (int)expectedValues[0];
    }

    return true;
}

// Play Animation

void saturn_play_animation(MarioAnimID anim) {
    force_set_mario_animation(gMarioState, anim);
    //set_mario_anim_with_accel(gMarioState, anim, anim_speed * 65535);
    is_anim_playing = true;
}

void saturn_play_keyframe() {
    if (k_frame_keys.size() <= 0) return;

    if (!keyframe_playing) {
        k_last_passed_index = 0;
        k_distance_between = 0;
        k_current_frame = 0;
        mcam_timer = 0;
        keyframe_playing = true;
    } else {
        if (k_current_frame > 0)
            keyframe_playing = false;
    }
}

// Copy

void saturn_copy_object(Vec3f from, Vec3f to) {
    vec3f_copy(from, to);
    vec3s_set(gMarioState->marioObj->header.gfx.angle, 0, gMarioState->faceAngle[1], 0);
}

Vec3f stored_mario_pos;
Vec3s stored_mario_angle;

void saturn_copy_mario() {
    vec3f_copy(stored_mario_pos, gMarioState->pos);
    vec3s_copy(stored_mario_angle, gMarioState->faceAngle);

    vec3f_copy(stored_camera_pos, gCamera->pos);
    vec3f_copy(stored_camera_focus, gCamera->focus);
}

void saturn_paste_mario() {
    if (machinimaCopying == 0) {
        vec3f_copy(gMarioState->pos, stored_mario_pos);
        vec3f_copy(gMarioState->marioObj->header.gfx.pos, stored_mario_pos);
        vec3s_copy(gMarioState->faceAngle, stored_mario_angle);
        vec3s_set(gMarioState->marioObj->header.gfx.angle, 0, stored_mario_angle[1], 0);
    }
    machinimaCopying = 1;
}

Vec3f pos_relative;
Vec3s foc_relative;
bool was_relative;

void saturn_copy_camera(bool relative) {
    if (relative) {
        pos_relative[0] = floor(gCamera->pos[0] - gMarioState->pos[0]);
        pos_relative[1] = floor(gCamera->pos[1] - gMarioState->pos[1]);
        pos_relative[2] = floor(gCamera->pos[2] - gMarioState->pos[2]);

        foc_relative[0] = floor(gCamera->focus[0] - gMarioState->pos[0]);
        foc_relative[1] = floor(gCamera->focus[1] - gMarioState->pos[1]);
        foc_relative[2] = floor(gCamera->focus[2] - gMarioState->pos[2]);
    } else {
        vec3f_copy(stored_camera_pos, gCamera->pos);
        vec3f_copy(stored_camera_focus, gCamera->focus);
    }
    was_relative = relative;
}

void saturn_paste_camera() {
    if (was_relative) {
        stored_camera_pos[0] = floor(gMarioState->pos[0] + pos_relative[0]);
        stored_camera_pos[1] = floor(gMarioState->pos[1] + pos_relative[1]);
        stored_camera_pos[2] = floor(gMarioState->pos[2] + pos_relative[2]);
        stored_camera_focus[0] = floor(gMarioState->pos[0] + foc_relative[0]);
        stored_camera_focus[1] = floor(gMarioState->pos[1] + foc_relative[1]);
        stored_camera_focus[2] = floor(gMarioState->pos[2] + foc_relative[2]);
    }
    machinimaCopying = 1;
}

// Debug

void saturn_print(const char* text) {
    if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_RSHIFT])
        printf(text);
}

// Other

SDL_Scancode saturn_key_to_scancode(unsigned int configKey[]) {
    for (int i = 0; i < MAX_BINDS; i++) {
        unsigned int key = configKey[i];

        if (key >= 0 && key < 0xEF) {
            for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
                if(scanCodeToKeyNum[i] == key) return (SDL_Scancode)i;
            }
        }
        return SDL_SCANCODE_UNKNOWN;
    }
}

const char* saturn_get_stage_name(int courseNum) {
    switch(courseNum) {
        case LEVEL_SA: return "Chroma Key Stage"; break;
        case LEVEL_CASTLE: return "Peach's Castle"; break;
        case LEVEL_CASTLE_GROUNDS: return "Castle Grounds"; break;
        case LEVEL_CASTLE_COURTYARD: return "Castle Courtyard"; break;
        case LEVEL_BOB: return "Bob-omb Battlefield"; break;
        case LEVEL_CCM: return "Cool, Cool Mountain"; break;
        case LEVEL_WF: return "Whomp's Fortress"; break;
        case LEVEL_JRB: return "Jolly Roger Bay"; break;
        case LEVEL_PSS: return "Princess's Secret Slide"; break;
        case LEVEL_TOTWC: return "Tower of the Wing Cap"; break;
        case LEVEL_BITDW: return "Bowser in the Dark World"; break;
        case LEVEL_BBH: return "Big Boo's Haunt"; break;
        case LEVEL_HMC: return "Hazy Maze Cave"; break;
        case LEVEL_COTMC: return "Cavern of the Metal Cap"; break;
        case LEVEL_LLL: return "Lethal Lava Land"; break;
        case LEVEL_SSL: return "Shifting Sand Land"; break;
        case LEVEL_VCUTM: return "Vanish Cap Under the Moat"; break;
        case LEVEL_DDD: return "Dire, Dire Docks"; break;
        case LEVEL_BITFS: return "Bowser in the Fire Sea"; break;
        case LEVEL_SL: return "Snowman's Land"; break;
        case LEVEL_WDW: return "Wet-Dry World"; break;
        case LEVEL_TTM: return "Tall, Tall Mountain"; break;
        case LEVEL_THI: return "Tiny, Huge Island"; break;
        case LEVEL_TTC: return "Tick Tock Clock"; break;
        case LEVEL_WMOTR: return "Wing Mario Over the Rainbow"; break;
        case LEVEL_RR: return "Rainbow Ride"; break;
        case LEVEL_BITS: return "Bowser in the Sky"; break;

        default: return "Unknown"; break;
    }
}

std::thread extract_thread;
std::thread load_thread;

bool load_thread_began = false;
bool loading_finished = false;

bool saturn_begin_extract_rom_thread() {
    if (extract_thread_began) return extraction_finished;
    extract_thread_began = true;
    extraction_finished = false;
    extract_thread = std::thread([]() {
        extract_return_code = saturn_extract_rom(EXTRACT_TYPE_ALL);
        extraction_finished = true;
    });
    return false;
}

bool saturn_do_load() {
    if (load_thread_began) return loading_finished;
    load_thread_began = true;
    loading_finished = false;
    load_thread = std::thread([]() {
        if (!(save_file_get_flags() & SAVE_FLAG_TALKED_TO_ALL_TOADS)) DynOS_Gfx_GetPacks().Clear();
        DynOS_Opt_Init();
        //model_details = "" + std::to_string(DynOS_Gfx_GetPacks().Count()) + " model pack";
        //if (DynOS_Gfx_GetPacks().Count() != 1) model_details += "s";
        saturn_imgui_init();
        saturn_load_locations();
        saturn_load_favorite_anims();
        saturn_fill_data_table();
        saturn_launch_timer = 0;
        loading_finished = true;
    });
    return false;
}

void saturn_on_splash_finish() {
    splash_finished = true;
}

bool saturn_timeline_exists(const char* name) {
    return k_frame_keys.find(name) != k_frame_keys.end();
}

// vvv    i would put this in a separate file but for SOME REASON the linker defines these but still throws "function undefined" smh

#define NUM_STARS 2048
#define STAR_WIDTH 16000
#define STAR_HEIGHT 9000
#define STAR_SPAWN_DISTANCE 1
#define STAR_SPEED 2

SDL_Texture* saturn_splash_screen_banner = nullptr;
int saturn_splash_screen_banner_width = 0;
int saturn_splash_screen_banner_height = 0;
unsigned char saturn_splash_screen_banner_data[] = {
#include "saturn_splash_screen_banner.h"
};

int load_delay = 2;

struct Star {
    float x, y, z;
};

Star stars[NUM_STARS];

void saturn_splash_screen_init(SDL_Renderer* renderer) {
    if (saturn_splash_screen_banner == nullptr) {
        int width, height;
        unsigned char* image_data = stbi_load_from_memory(saturn_splash_screen_banner_data, sizeof(saturn_splash_screen_banner_data), &width, &height, nullptr, STBI_rgb_alpha);
        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(image_data, width, height, 32, width * 4, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        saturn_splash_screen_banner = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        saturn_splash_screen_banner_width = width;
        saturn_splash_screen_banner_height = height;
    }
    memset(stars, 0, sizeof(Star) * NUM_STARS);
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i] = (Star){ .x = rand() % STAR_WIDTH - STAR_WIDTH / 2, .y = rand() % STAR_HEIGHT - STAR_HEIGHT / 2, .z = (i + 1) * STAR_SPAWN_DISTANCE };
    }
}

void saturn_splash_screen_update_stars() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].z -= STAR_SPEED;
        if (stars[i].z <= 0) stars[i] = (Star){ .x = rand() % STAR_WIDTH - STAR_WIDTH / 2, .y = rand() % STAR_HEIGHT - STAR_HEIGHT / 2, .z = NUM_STARS * STAR_SPAWN_DISTANCE };
    }
}

bool saturn_splash_screen_update(SDL_Renderer* renderer) {
    SDL_SetRenderDrawColor(renderer, 64, 0, 64, 255);
    SDL_Rect bg = (SDL_Rect){ .x = 0, .y = 0, .w = 640, .h = 360 };
    SDL_RenderFillRect(renderer, &bg);
    saturn_splash_screen_update_stars();
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < NUM_STARS; i++) {
        float x = stars[i].x / (stars[i].z / 16) + 640 / 2.f;
        float y = stars[i].y / (stars[i].z / 16) + 360 / 2.f;
        SDL_Rect rect = (SDL_Rect){ .x = x, .y = y, .w = 1, .h = 1 };
        SDL_RenderFillRect(renderer, &rect);
    }
    SDL_Rect src = (SDL_Rect){
        .x = 0,
        .y = 0,
        .w = saturn_splash_screen_banner_width,
        .h = saturn_splash_screen_banner_height
    };
    SDL_Rect dst = (SDL_Rect){
        .x = 640 / 2 - saturn_splash_screen_banner_width / 2,
        .y = 360 / 2 - saturn_splash_screen_banner_height / 2,
        .w = saturn_splash_screen_banner_width,
        .h = saturn_splash_screen_banner_height
    };
    SDL_RenderCopy(renderer, saturn_splash_screen_banner, &src, &dst);
    if (!saturn_begin_extract_rom_thread()) {
        if (extraction_progress >= 0) {
            SDL_Rect rect1 = (SDL_Rect){ .x = 16, .y = 360 - 32, .w = 640 - 32, .h = 16 };
            SDL_Rect rect2 = (SDL_Rect){ .x = 16, .y = 360 - 32, .w = (640 - 32) * extraction_progress, .h = 16 };
            SDL_SetRenderDrawColor(renderer, 127, 127, 127, 255);
            SDL_RenderFillRect(renderer, &rect1);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect2);
        }
        return false;
    }
    if (load_delay-- > 0) return false;
    if (!saturn_do_load()) return false;
    return true;
}

int saturn_splash_screen_open() {
    // make the x11 compositor on linux not kill itself
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    SDL_Window* window = SDL_CreateWindow("Saturn Studio", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetWindowBordered(window, SDL_FALSE);
    saturn_splash_screen_init(renderer);
    bool extracting_assets = true;
    while (true) {
        clock_t before = clock();
        if (saturn_splash_screen_update(renderer)) break;
        SDL_RenderPresent(renderer);
        clock_t after = clock();
        if (after - before < CLOCKS_PER_SEC / 60) std::this_thread::sleep_for(std::chrono::microseconds((int)(1000000 / 60.0f - (float)(after - before) / CLOCKS_PER_SEC * 1000000)));
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return extract_return_code;
}
