#include "saturn.h"
#include "saturn_colors.h"
#include "saturn_models.h"
#include "saturn_actors.h"

extern "C" {
#include "include/object_fields.h"
#include "game/object_list_processor.h"
#include "game/level_update.h"
#include "engine/math_util.h"
#include "game/memory.h"
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
        marioObj->oMarioActorPtrHi = (ptr >> 32) & 0xFFFFFFFF;
        marioObj->oMarioActorPtrLo =  ptr        & 0xFFFFFFFF;
    }
    ~MarioActor() {
        obj_mark_for_deletion(marioObj);
    }
};

#define o gCurrentObject

MarioActor* mario_actor = nullptr;

void saturn_add_actor(MarioActor& actor) {
    MarioActor** actorptr = &mario_actor;
    while (*actorptr) actorptr = &(*actorptr)->next;
    MarioActor* new_actor = new MarioActor(actor);
    new_actor->prev = *actorptr;
    *actorptr = new_actor;
}

void saturn_remove_actor(int index) {
    MarioActor** actorptr = &mario_actor;
    for (int i = 0; i < index; i++) {
        if (!*actorptr) return;
        actorptr = &(*actorptr)->next;
    }
    MarioActor* prev = (*actorptr)->prev;
    MarioActor* next = (*actorptr)->next;
    if (prev) prev->next = next;
    if (next) next->prev = prev;
    delete *actorptr;
}

MarioActor* saturn_get_actor(int index) {
    MarioActor** actorptr = &mario_actor;
    for (int i = 0; i < index; i++) {
        if (!*actorptr) return nullptr;
        actorptr = &(*actorptr)->next;
    }
    return *actorptr;
}

int saturn_actor_indexof(MarioActor* actor) {
    MarioActor** actorptr = &mario_actor;
    int idx = 0;
    while (*actorptr) {
        if (*actorptr == actor) return idx;
        actorptr = &(*actorptr)->next;
        idx++;
    }
    return idx;
}

int saturn_actor_sizeof() {
    return saturn_actor_indexof(nullptr);
}

void bhv_mario_actor_loop() {
    MarioActor* actor = cur_obj_mario_actor(o);
    o->oPosX = actor->x;
    o->oPosY = actor->y;
    o->oPosZ = actor->z;
    o->oFaceAngleYaw = actor->angle;
    vec3f_set(o->header.gfx.scale, actor->xScale, actor->yScale, actor->zScale);
    if (actor->animstate.custom) {
        o->header.gfx.unk38.curAnim->flags = 0;
        o->header.gfx.unk38.curAnim->unk02 = 0;
        o->header.gfx.unk38.curAnim->unk04 = 0;
        o->header.gfx.unk38.curAnim->unk06 = 0;
        o->header.gfx.unk38.curAnim->unk08 = (s16)actor->animstate.customanim_length;
        o->header.gfx.unk38.curAnim->unk0A = actor->animstate.customanim_numindices / 6 - 1;
        o->header.gfx.unk38.curAnim->values = actor->animstate.customanim_values;
        o->header.gfx.unk38.curAnim->index = (const u16*)actor->animstate.customanim_indices;
        o->header.gfx.unk38.curAnim->length = 0;
    }
    else {
        struct Animation* targetAnim = gMarioState->animation->targetAnim;
        if (load_patchable_table(gMarioState->animation, actor->animstate.id)) {
            targetAnim->values = (const s16*)VIRTUAL_TO_PHYSICAL((u8*)targetAnim + (uintptr_t)targetAnim->values);
            targetAnim->index  = (const u16*)VIRTUAL_TO_PHYSICAL((u8*)targetAnim + (uintptr_t)targetAnim->index );
        }
        o->header.gfx.unk38.animID = actor->animstate.id;
        o->header.gfx.unk38.curAnim = targetAnim;
        o->header.gfx.unk38.animAccel = 0;
        o->header.gfx.unk38.animYTrans = 0;
    }
    o->header.gfx.unk38.animFrame = actor->animstate.frame;
}