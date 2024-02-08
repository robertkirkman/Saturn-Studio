#include "saturn/saturn_timelines.h"

#define DEFAULT 0
#define FORCE_WAIT 1
#define EVENT 2

#define SATURN_KFENTRY_BOOL(id, variable, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_BOOL, KFBEH_FORCE_WAIT, name, 0, 1, mario } })
#define SATURN_KFENTRY_FLOAT(id, variable, values, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_FLOAT, KFBEH_DEFAULT, name, -3, values, mario } })
#define SATURN_KFENTRY_ANIM(id, name) timelineDataTable.insert({ id, { &current_animation, KFTYPE_ANIM, KFBEH_EVENT, name, 0, 2, true } })
#define SATURN_KFENTRY_EXPRESSION(id, name) timelineDataTable.insert({ id, { &current_model, KFTYPE_EXPRESSION, KFBEH_EVENT, name, 0, 1, true } })
#define SATURN_KFENTRY_COLOR(id, variable, name, mario) timelineDataTable.insert({ id, { variable, KFTYPE_COLOR, KFBEH_DEFAULT, name, 0, 6, mario } })

std::map<std::string, std::tuple<void*, KeyframeType, char, std::string, int, int, bool>> timelineDataTable = {};

#define MARIO_ENTRY(var) (void*)offsetof(MarioActor, var)
#define CC_ENTRY(index) (void*)(offsetof(MarioActor, colorcode) + index * sizeof(struct ColorTemplate))

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
    SATURN_KFENTRY_FLOAT("k_light_col", gLightingColor, 3, "Light Color", false);
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
    for (int i = 0; i < 20; i++) {
        std::string num = std::to_string(i + 1);
        SATURN_KFENTRY_FLOAT("k_mariobone_" + num, (void*)(offsetof(MarioActor, bones) + i * sizeof(Vec3f)), 3, "Anim Bone " + num, true);
    }
}