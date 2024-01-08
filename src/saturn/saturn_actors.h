#ifndef SaturnActors
#define SaturnActors

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

#define cur_obj_mario_actor(obj) saturn_get_actor(obj->oMarioActorIndex)

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
    ~MarioActor() {
        obj_mark_for_deletion(marioObj);
    }
};

extern MarioActor* mario_actor;

extern void saturn_add_actor(MarioActor& actor);
extern void saturn_remove_actor(int index);
extern MarioActor* saturn_get_actor(int index);
extern int saturn_actor_indexof(MarioActor* actor);
extern int saturn_actor_sizeof();

extern "C" {
#endif
    void bhv_mario_actor_loop();
    void override_cc_color(int* r, int* g, int* b, int ccIndex, int marioIndex, int shadeIndex, float intensity, bool additive);
#ifdef __cplusplus
}
#endif

#endif