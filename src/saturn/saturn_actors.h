#ifndef SaturnActors
#define SaturnActors

#ifdef __cplusplus

extern "C" {
#include "game/object_helpers.h"
#include "game/level_update.h"
#include "include/model_ids.h"
#include "include/object_fields.h"
#include "include/behavior_data.h"
}

#define cur_obj_mario_actor(obj) (MarioActor*)(void*)((obj->oMarioActorPtrHi << 32) | obj->oMarioActorPtrLo);

class MarioActor;

extern MarioActor* mario_actor;

extern void saturn_add_actor(MarioActor& actor);
extern void saturn_remove_actor(int index);
extern MarioActor* saturn_get_actor(int index);
extern int saturn_actor_indexof(MarioActor* actor);
extern int saturn_actor_sizeof();

extern "C" {
#endif
    void bhv_mario_actor_loop();
#ifdef __cplusplus
}
#endif

#endif