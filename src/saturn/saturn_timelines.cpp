#include "saturn/saturn_timelines.h"
#include "saturn/saturn.h"

#define DEFAULT 0
#define FORCE_WAIT 1
#define EVENT 2

#define SATURN_KFENTRY_BOOL(id, variable, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_BOOL, KFBEH_FORCE_WAIT, name, 0, 1, mario } })
#define SATURN_KFENTRY_FLOAT(id, variable, values, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_FLOAT, KFBEH_DEFAULT, name, -3, values, mario } })
#define SATURN_KFENTRY_ANIM(id, name) timelineDataTable.insert({ id, { MARIO_ENTRY(animstate), KFTYPE_ANIM, KFBEH_FORCE_WAIT, name, 0, 2, true } })
#define SATURN_KFENTRY_EXPRESSION(id, name) timelineDataTable.insert({ id, { MARIO_ENTRY(model), KFTYPE_EXPRESSION, KFBEH_FORCE_WAIT, name, 0, 1, true } })
#define SATURN_KFENTRY_COLOR(id, variable, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_COLOR, KFBEH_DEFAULT, name, 0, 6, mario } })
#define SATURN_KFENTRY_COLORF(id, variable, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_COLORF, KFBEH_DEFAULT, name, -3, 3, mario } })
#define SATURN_KFENTRY_SWITCH(id, variable, name) timelineDataTable.insert({ id, { variable, KFTYPE_SWITCH, KFBEH_FORCE_WAIT, name, 0, 1, true } })

std::map<std::string, std::tuple<void*, KeyframeType, char, std::string, int, int, bool>> timelineDataTable = {};
std::map<std::string, std::vector<std::string>> kf_switch_names = {
    { "k_switch_eyes", { "Blinking", "Open", "Half", "Closed", "Left", "Right", "Up", "Down", "Dead" } },
    { "k_switch_hand", { "Fists", "Open", "Peace", "With Cap", "With Wing Cap", "Right Open" } },
    { "k_switch_cap", { "Cap On", "Cap Off", "Wing Cap" } },
    { "k_switch_powerup", { "Default", "Vanish", "Metal", "Metal & Vanish" } },
};

#define MARIO_ENTRY(var) (void*)offsetof(MarioActor, var)
#define ARR_ENTRY(arr, index) (void*)(offsetof(MarioActor, arr) + index * sizeof(*((MarioActor*)0)->arr))
#define CC_ENTRY(index) ARR_ENTRY(colorcode, index)
#define BONE_ENTRY(index) ARR_ENTRY(bones, index)

// { id, { variable_ptr, type, behavior, name, precision, num_values, is_mario } }
void saturn_fill_data_table() {
    SATURN_KFENTRY_BOOL("k_skybox_mode", &use_color_background, "Skybox Mode", false);
    SATURN_KFENTRY_BOOL("k_shadows", &enable_shadows, "Shadows", false);
    SATURN_KFENTRY_FLOAT("k_shade_x", &world_light_dir1, 1, "Shade X", false);
    SATURN_KFENTRY_FLOAT("k_shade_y", &world_light_dir2, 1, "Shade Y", false);
    SATURN_KFENTRY_FLOAT("k_shade_z", &world_light_dir3, 1, "Shade Z", false);
    SATURN_KFENTRY_FLOAT("k_shade_t", &world_light_dir4, 1, "Shade Tex", false);
    SATURN_KFENTRY_FLOAT("k_scale", MARIO_ENTRY(xScale), 3, "Scale", true);
    SATURN_KFENTRY_BOOL("k_v_cap_emblem", MARIO_ENTRY(show_emblem), "M Cap Emblem", true);
    SATURN_KFENTRY_FLOAT("k_angle", MARIO_ENTRY(angle), 1, "Mario Angle", true);
    SATURN_KFENTRY_FLOAT("k_mariopos_x", MARIO_ENTRY(x), 1, "Mario Pos X", true);
    SATURN_KFENTRY_FLOAT("k_mariopos_y", MARIO_ENTRY(y), 1, "Mario Pos Y", true);
    SATURN_KFENTRY_FLOAT("k_mariopos_z", MARIO_ENTRY(z), 1, "Mario Pos Z", true);
    SATURN_KFENTRY_FLOAT("k_shadow_scale", MARIO_ENTRY(shadow_scale), 1, "Shadow Scale", true);
    SATURN_KFENTRY_FLOAT("k_headrot_x", MARIO_ENTRY(head_rot_x), 1, "Head Rotation Yaw", true);
    SATURN_KFENTRY_FLOAT("k_headrot_y", MARIO_ENTRY(head_rot_y), 1, "Head Rotation Roll", true);
    SATURN_KFENTRY_FLOAT("k_headrot_z", MARIO_ENTRY(head_rot_z), 1, "Head Rotation Pitch", true);
    SATURN_KFENTRY_FLOAT("k_inputrec_frame", MARIO_ENTRY(input_recording_frame), 1, "Input Playback Frame", true);
    SATURN_KFENTRY_BOOL("k_inputrec_enable", MARIO_ENTRY(playback_input), "Input Playback Enable", true);
    SATURN_KFENTRY_BOOL("k_mario_hidden", MARIO_ENTRY(hidden), "Hidden", true);
    SATURN_KFENTRY_BOOL("k_hud", &configHUD, "HUD", false);
    SATURN_KFENTRY_FLOAT("k_fov", &camera_fov, 1, "FOV", false);
    SATURN_KFENTRY_FLOAT("k_focus", &camera_focus, 1, "Follow", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_pos0", &freezecamPos[0], 1, "Camera Pos X", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_pos1", &freezecamPos[1], 1, "Camera Pos Y", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_pos2", &freezecamPos[2], 1, "Camera Pos Z", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_yaw", &freezecamYaw, 1, "Camera Yaw", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_pitch", &freezecamPitch, 1, "Camera Pitch", false);
    SATURN_KFENTRY_FLOAT("k_c_camera_roll", &freezecamRoll, 1, "Camera Roll", false);
    SATURN_KFENTRY_FLOAT("k_gravity", &gravity, 1, "Gravity", false);
    SATURN_KFENTRY_COLORF("k_light_col", gLightingColor, "Light Color", false);
    SATURN_KFENTRY_COLOR("k_color", &chromaColor, "Skybox Color", false);
    SATURN_KFENTRY_COLOR("k_hat", CC_ENTRY(CC_HAT), "Hat", true);
    SATURN_KFENTRY_COLOR("k_overalls", CC_ENTRY(CC_OVERALLS), "Overalls", true);
    SATURN_KFENTRY_COLOR("k_gloves", CC_ENTRY(CC_GLOVES), "Gloves", true);
    SATURN_KFENTRY_COLOR("k_shoes", CC_ENTRY(CC_SHOES), "Shoes", true);
    SATURN_KFENTRY_COLOR("k_skin", CC_ENTRY(CC_SKIN), "Skin", true);
    SATURN_KFENTRY_COLOR("k_hair", CC_ENTRY(CC_HAIR), "Hair", true);
    SATURN_KFENTRY_COLOR("k_shirt", CC_ENTRY(CC_SHIRT), "Shirt", true);
    SATURN_KFENTRY_COLOR("k_shoulders", CC_ENTRY(CC_SHOULDERS), "Shoulders", true);
    SATURN_KFENTRY_COLOR("k_arms", CC_ENTRY(CC_ARMS), "Arms", true);
    SATURN_KFENTRY_COLOR("k_overalls_bottom", CC_ENTRY(CC_OVERALLS_BOTTOM), "Overalls (Bottom)", true);
    SATURN_KFENTRY_COLOR("k_leg_top", CC_ENTRY(CC_LEG_TOP), "Leg (Top)", true);
    SATURN_KFENTRY_COLOR("k_leg_bottom", CC_ENTRY(CC_LEG_BOTTOM), "Leg (Bottom)", true);
    SATURN_KFENTRY_ANIM("k_mario_anim", "Animation");
    SATURN_KFENTRY_FLOAT("k_mario_anim_frame", MARIO_ENTRY(animstate.frame), 1, "Anim Frame", true);
    SATURN_KFENTRY_EXPRESSION("k_mario_expr", "Expression");
    SATURN_KFENTRY_SWITCH("k_switch_eyes", MARIO_ENTRY(eye_state), "Eye Switch");
    SATURN_KFENTRY_SWITCH("k_switch_hand", MARIO_ENTRY(hand_state), "Hand Switch");
    SATURN_KFENTRY_SWITCH("k_switch_cap", MARIO_ENTRY(cap_state), "Cap Switch");
    SATURN_KFENTRY_SWITCH("k_switch_powerup", MARIO_ENTRY(powerup_state), "Powerup Switch");
    SATURN_KFENTRY_BOOL("k_customeyes", MARIO_ENTRY(custom_eyes), "Custom Eyes", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_1", BONE_ENTRY(0), 3, "Root", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_3", BONE_ENTRY(2), 3, "Torso", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_4", BONE_ENTRY(3), 3, "Head", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_6", BONE_ENTRY(5), 3, "Upper Left Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_7", BONE_ENTRY(6), 3, "Lower Left Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_8", BONE_ENTRY(7), 3, "Left Hand", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_10", BONE_ENTRY(9), 3, "Upper Right Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_11", BONE_ENTRY(10), 3, "Lower Right Arm", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_12", BONE_ENTRY(11), 3, "Right Hand", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_14", BONE_ENTRY(13), 3, "Upper Left Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_15", BONE_ENTRY(14), 3, "Lower Left Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_16", BONE_ENTRY(15), 3, "Left Foot", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_18", BONE_ENTRY(17), 3, "Upper Right Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_19", BONE_ENTRY(18), 3, "Lower Right Leg", true);
    SATURN_KFENTRY_FLOAT("k_mariobone_20", BONE_ENTRY(19), 3, "Right Foot", true);
}