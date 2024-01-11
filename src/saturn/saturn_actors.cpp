#include "saturn_actors.h"

extern "C" {
#include "include/object_fields.h"
#include "game/object_list_processor.h"
#include "game/level_update.h"
#include "engine/math_util.h"
#include "game/memory.h"
}

#define o gCurrentObject

MarioActor* mario_actor = nullptr;

void saturn_spawn_actor(float x, float y, float z) {
    MarioActor actor;
    actor.x = x;
    actor.y = y;
    actor.z = z;
    actor.animstate.custom = false;
    actor.animstate.hang = false;
    actor.animstate.loop = true;
    actor.animstate.speed = 1;
    actor.animstate.id = MARIO_ANIM_FIRST_PERSON;
    actor.animstate.frame = 0;
    saturn_add_actor(actor);
}

void saturn_add_actor(MarioActor& actor) {
    MarioActor** actorptr = &mario_actor;
    while (*actorptr) actorptr = &(*actorptr)->next;
    MarioActor* new_actor = new MarioActor(actor);
    new_actor->marioObj->oMarioActorIndex = saturn_actor_sizeof();
    new_actor->prev = *actorptr;
    *actorptr = new_actor;
}

void saturn_remove_actor(int index) {
    MarioActor* actorptr = mario_actor;
    for (int i = 0; i < index; i++) {
        if (!actorptr) return;
        actorptr = actorptr->next;
    }
    MarioActor* prev = actorptr->prev;
    MarioActor* next = actorptr->next;
    if (prev) prev->next = next;
    if (next) next->prev = prev;
    obj_mark_for_deletion(actorptr->marioObj);
    delete actorptr;
}

MarioActor* saturn_get_actor(int index) {
    MarioActor* actorptr = mario_actor;
    for (int i = 0; i < index; i++) {
        if (!actorptr) return nullptr;
        actorptr = actorptr->next;
    }
    return actorptr;
}

int saturn_actor_indexof(MarioActor* actor) {
    MarioActor* actorptr = mario_actor;
    int idx = 0;
    while (actorptr) {
        if (actorptr == actor) return idx;
        actorptr = actorptr->next;
        idx++;
    }
    return idx;
}

int saturn_actor_sizeof() {
    return saturn_actor_indexof(nullptr);
}

void bhv_mario_actor_loop() {
    std::cout << "processing actor " << o->oMarioActorIndex << std::endl;
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
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
    int length = o->header.gfx.unk38.curAnim->unk08 - 1;
    actor->animstate.frame += actor->animstate.speed;
    if (actor->animstate.frame >= length) {
        if (actor->animstate.loop) actor->animstate.frame -= length;
        else actor->animstate.frame = length - 1;
    }
    o->header.gfx.unk38.animYTrans = 0xBD;
    o->header.gfx.unk38.animFrame = actor->animstate.frame;
}

void override_cc_color(int* r, int* g, int* b, int ccIndex, int marioIndex, int shadeIndex, float intensity, bool additive) {
    MarioActor* actor = saturn_get_actor(marioIndex);
    if (actor == nullptr) return;
    *r = (*r * additive) + intensity * actor->colorcode[ccIndex].red[shadeIndex];
    *g = (*g * additive) + intensity * actor->colorcode[ccIndex].green[shadeIndex];
    *b = (*b * additive) + intensity * actor->colorcode[ccIndex].blue[shadeIndex];
}

bool saturn_rotate_head(Vec3s rotation) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (actor != nullptr) vec3s_set(rotation,
        actor->head_rot_x,
        actor->head_rot_y,
        0
    );
    else vec3s_set(rotation, 0, 0, 0);
    return actor != nullptr;
}