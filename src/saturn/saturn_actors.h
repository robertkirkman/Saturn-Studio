#ifndef SaturnActors
#define SaturnActors

#include "include/types.h"

#ifdef __cplusplus

#include "saturn/saturn.h"
#include "saturn/saturn_models.h"
#include "saturn/saturn_colors.h"

extern "C" {
#include "game/object_helpers.h"
#include "game/level_update.h"
#include "include/model_ids.h"
#include "include/object_fields.h"
#include "include/behavior_data.h"
}

class MarioActor {
public:
    float x = 0;
    float y = 0;
    float z = 0;
    float angle = 0;
    float xScale = 1;
    float yScale = 1;
    float zScale = 1;
    bool show_emblem = true;
    int head_rot_x = 0;
    int head_rot_y = 0;
    Model model = Model();
    ColorCode colorcode;
    struct AnimationState animstate;
    MarioActor* prev = nullptr;
    MarioActor* next = nullptr;
    struct Object* marioObj = nullptr;
    MarioActor() {
        u64 ptr = (u64)this;
        PasteGameShark(GameSharkCode().GameShark, colorcode);
        marioObj = spawn_object(gMarioState->marioObj, MODEL_MARIO, bhvMarioActor);
    }
};

extern MarioActor* mario_actor;

extern MarioActor* saturn_spawn_actor(float x, float y, float z);
extern MarioActor* saturn_add_actor(MarioActor& actor);
extern void saturn_remove_actor(int index);
extern MarioActor* saturn_get_actor(int index);
extern int saturn_actor_indexof(MarioActor* actor);
extern int saturn_actor_sizeof();

extern "C" {
#endif
    void saturn_clear_actors();
    void bhv_mario_actor_loop();
    void override_cc_color(int* r, int* g, int* b, int ccIndex, int marioIndex, int shadeIndex, float intensity, bool additive);
    bool saturn_rotate_head(Vec3s rotation);
#ifdef __cplusplus
}
#endif

#endif