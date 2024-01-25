#include "saturn_actors.h"
#include "game/object_helpers.h"
#include "saturn/saturn_models.h"

extern "C" {
#include "include/object_fields.h"
#include "game/object_list_processor.h"
#include "game/level_update.h"
#include "engine/math_util.h"
#include "game/memory.h"
}

#define o gCurrentObject

MarioActor* gMarioActorList = nullptr;

MarioActor* saturn_spawn_actor(float x, float y, float z) {
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
    return saturn_add_actor(actor);
}

void reassign_actor_indexes() {
    MarioActor* actor = gMarioActorList;
    int i = 0;
    while (actor) {
        actor->marioObj->oMarioActorIndex = i++;
        actor = actor->next;
    }
}

MarioActor* saturn_add_actor(MarioActor& actor) {
    MarioActor* new_actor = new MarioActor(actor);
    new_actor->marioObj->oMarioActorIndex = saturn_actor_sizeof();
    if (gMarioActorList) {
        MarioActor* prev = gMarioActorList;
        while (prev->next) prev = prev->next;
        prev->next = new_actor;
        new_actor->prev = prev;
    }
    else gMarioActorList = new_actor;
    return new_actor;
}

void saturn_remove_actor(int index) {
    MarioActor* actorptr = gMarioActorList;
    for (int i = 0; i < index; i++) {
        if (!actorptr) return;
        actorptr = actorptr->next;
    }
    MarioActor* prev = actorptr->prev;
    MarioActor* next = actorptr->next;
    if (next) next->prev = prev;
    if (prev) prev->next = next;
    else gMarioActorList = next;
    reassign_actor_indexes();
    obj_mark_for_deletion(actorptr->marioObj);
    delete actorptr;
}

MarioActor* saturn_get_actor(int index) {
    MarioActor* actorptr = gMarioActorList;
    for (int i = 0; i < index; i++) {
        if (!actorptr) return nullptr;
        actorptr = actorptr->next;
    }
    return actorptr;
}

int saturn_actor_indexof(MarioActor* actor) {
    MarioActor* actorptr = gMarioActorList;
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

void saturn_clear_actors() {
    MarioActor* actor = gMarioActorList;
    while (actor) {
        MarioActor* next = actor->next;
        obj_mark_for_deletion(actor->marioObj);
        delete actor;
        actor = next;
    }
    gMarioActorList = nullptr;
}

void bhv_mario_actor_loop() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (!actor) return;
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

s16 saturn_actor_geo_switch(u8 item) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (actor == nullptr) return 0;
    switch (item) {
        case ACTOR_SWITCH_EYE:
            return actor->custom_eyes ? 4 : actor->eye_state;
            break;
        case ACTOR_SWITCH_CAP:
            return actor->cap_state;
            break;
        case ACTOR_SWITCH_HAND:
            return actor->hand_state;
            break;
        case ACTOR_SWITCH_POWERUP:
            return actor->powerup_state;
            break;
    }
    return 0;
}

float saturn_actor_get_alpha() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (actor == nullptr) return 255;
    if (actor->powerup_state & 1) return actor->alpha;
    return 255;
}

struct ModelTexture {
    struct ModelTexture* next;
    char* id;
    char* data;
    int w, h;
};
struct ModelTexture* gModelTextureList;

void saturn_actor_add_model_texture(char* id, char* data, int w, int h) {
    struct ModelTexture* curr = gModelTextureList;
    while (curr) {
        if (strcmp(curr->id, id) == 0) return;
        curr = curr->next;
    }
    struct ModelTexture* tex = (struct ModelTexture*)malloc(sizeof(struct ModelTexture));
    int texid_len = strlen(id) + 1;
    char* texid = (char*)malloc(texid_len);
    char* texdata = (char*)malloc(w * h * 4);
    memcpy(texid, id, texid_len);
    memcpy(texdata, data, w * h * 4);
    tex->w = w;
    tex->h = h;
    tex->id = texid;
    tex->data = texdata;
    tex->next = nullptr;
    if (gModelTextureList) {
        struct ModelTexture* prev = gModelTextureList;
        while (prev->next) prev = prev->next;
        prev->next = tex;
    }
    else gModelTextureList = tex;
}

char* saturn_actor_get_model_texture(char* id, int* w, int* h) {
    struct ModelTexture* curr = gModelTextureList;
    while (curr) {
        if (strcmp(curr->id, id) == 0) {
            *w = curr->w;
            *h = curr->h;
            return curr->data;
        }
        curr = curr->next;
    }
    return nullptr;
}