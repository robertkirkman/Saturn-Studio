#include "saturn_actors.h"
#include "game/object_helpers.h"
#include "mario_animation_ids.h"
#include "saturn/saturn.h"
#include "saturn/saturn_models.h"

extern "C" {
#include "include/object_fields.h"
#include "game/object_list_processor.h"
#include "game/level_update.h"
#include "engine/math_util.h"
#include "game/memory.h"
}

#define gMarioAnims mario_animation_data
#include "assets/mario_anim_data.c"
#undef gMarioAnims

#define o gCurrentObject

MarioActor* gMarioActorList = nullptr;

void delete_mario_actor_timelines(int index) {
    std::vector<std::string> ids = {};
    for (auto& timeline : k_frame_keys) {
        if (timeline.second.first.marioIndex != index) continue;
        ids.push_back(timeline.first);
    }
    for (std::string id : ids) {
        k_frame_keys.erase(id);
    }
}

struct Animation* load_animation(MarioActor* actor, int index) {
    struct Animation* anim = (struct Animation*)((u8*)&mario_animation_data + mario_animation_data.entries[index].offset);
    actor->anim.index = anim->index;
    actor->anim.values = anim->values;
    actor->anim.flags = anim->flags;
    actor->anim.length = anim->length;
    actor->anim.unk02 = anim->unk02;
    actor->anim.unk04 = anim->unk04;
    actor->anim.unk06 = anim->unk06;
    actor->anim.unk08 = anim->unk08;
    actor->anim.unk0A = anim->unk0A;
    actor->anim.values = (const s16*)((u8*)anim + (uintptr_t)anim->values);
    actor->anim.index  = (const u16*)((u8*)anim + (uintptr_t)anim->index );
    return &actor->anim;
}

MarioActor* saturn_spawn_actor(float x, float y, float z) {
    MarioActor actor;
    actor.x = x;
    actor.y = y;
    actor.z = z;
    actor.animstate.custom = false;
    actor.animstate.id = MARIO_ANIM_A_POSE;
    actor.animstate.frame = 0;
    return saturn_add_actor(actor);
}

MarioActor* saturn_add_new_actor(MarioActor& actor) {
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

MarioActor* saturn_try_replace_actor(MarioActor& actor) {
    MarioActor* curr = gMarioActorList;
    int i = 0;
    while (curr) {
        if (!curr->exists) break;
        curr = curr->next;
        i++;
    }
    if (!curr) return nullptr;
    MarioActor* new_actor = new MarioActor(actor);
    new_actor->marioObj->oMarioActorIndex = i;
    MarioActor* prev = curr->prev;
    MarioActor* next = curr->next;
    delete curr;
    new_actor->prev = prev;
    new_actor->next = next;
    if (next) next->prev = new_actor;
    if (prev) prev->next = new_actor;
    else gMarioActorList = new_actor;
    return new_actor;
}

MarioActor* saturn_add_actor(MarioActor& actor) {
    MarioActor* new_actor = saturn_try_replace_actor(actor);
    if (new_actor) return new_actor;
    return saturn_add_new_actor(actor);
}

void saturn_remove_actor(int index) {
    MarioActor* actorptr = gMarioActorList;
    for (int i = 0; i < index; i++) {
        if (!actorptr) return;
        actorptr = actorptr->next;
    }
    if (!actorptr) return;
    if (!actorptr->exists) return;
    actorptr->exists = false;
    delete_mario_actor_timelines(index);
    obj_mark_for_deletion(actorptr->marioObj);
}

MarioActor* saturn_get_actor(int index) {
    MarioActor* actorptr = gMarioActorList;
    for (int i = 0; i < index; i++) {
        if (!actorptr) return nullptr;
        actorptr = actorptr->next;
    }
    if (!actorptr) return nullptr;
    if (!actorptr->exists) return nullptr;
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
    int i = 0;
    while (actor) {
        delete_mario_actor_timelines(i++);
        obj_mark_for_deletion(actor->marioObj);
        actor->exists = false;
        actor = actor->next;
    }
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
        o->header.gfx.unk38.curAnim->flags = 4;
        o->header.gfx.unk38.curAnim->unk02 = 0;
        o->header.gfx.unk38.curAnim->unk04 = 0;
        o->header.gfx.unk38.curAnim->unk06 = 0;
        o->header.gfx.unk38.curAnim->unk08 = (s16)actor->animstate.length;
        o->header.gfx.unk38.curAnim->unk0A = actor->animstate.customanim_indices.size() / 6 - 1;
        o->header.gfx.unk38.curAnim->values = actor->animstate.customanim_values.data();
        o->header.gfx.unk38.curAnim->index = (const u16*)actor->animstate.customanim_indices.data();
        o->header.gfx.unk38.curAnim->length = (s16)actor->animstate.length;
    }
    else {
        o->header.gfx.unk38.animID = actor->animstate.id;
        o->header.gfx.unk38.curAnim = load_animation(actor, actor->animstate.id);
        o->header.gfx.unk38.curAnim->flags = 4; // prevent the anim to get a mind on its own
        actor->animstate.length = o->header.gfx.unk38.curAnim->unk08;
    }
    o->header.gfx.unk38.animYTrans = 0xBD;
    o->header.gfx.unk38.animFrame = actor->animstate.frame;
}

void override_cc_color(int* r, int* g, int* b, int ccIndex, int marioIndex, int shadeIndex, float intensity, bool additive) {
    MarioActor* actor = saturn_get_actor(marioIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return;
    *r = (*r * additive) + intensity * actor->colorcode[ccIndex].red[shadeIndex];
    *g = (*g * additive) + intensity * actor->colorcode[ccIndex].green[shadeIndex];
    *b = (*b * additive) + intensity * actor->colorcode[ccIndex].blue[shadeIndex];
}

bool saturn_rotate_head(Vec3s rotation) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor != nullptr) {
        if (actor->custom_bone_iter != 3) actor = nullptr;
        if (actor != nullptr) {
            vec3s_set(rotation,
                actor->head_rot_x,
                actor->head_rot_y,
                actor->head_rot_z
            );
            return true;
        }
    }
    vec3s_set(rotation, 0, 0, 0);
    return false;
}

s16 saturn_actor_geo_switch(u8 item) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
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
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return 255;
    if (actor->powerup_state & 1) return actor->alpha;
    return 255;
}

bool saturn_actor_has_custom_anim_extra() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return false;
    return actor->animstate.custom && actor->animstate.customanim_extra;
}

int saturn_actor_get_support_flags(int marioIndex) {
    MarioActor* actor = saturn_get_actor(marioIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return 0;
    return (actor->spark_support << 1) | actor->cc_support;
}

float saturn_actor_get_shadow_scale() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return 1.f;
    return actor->shadow_scale;
}

void saturn_actor_bone_override_begin() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return;
    actor->custom_bone_iter = 0;
}

bool saturn_actor_bone_should_override() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return false;
    return actor->custom_bone;
}

void saturn_actor_bone_iterate() {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return;
    actor->custom_bone_iter++;
}

void saturn_actor_bone_do_override(Vec3s rotation) {
    MarioActor* actor = saturn_get_actor(o->oMarioActorIndex);
    if (o->behavior != bhvMarioActor) actor = nullptr;
    if (actor == nullptr) return;
    vec3s_set(rotation,
        actor->bones[actor->custom_bone_iter][0] / 360.f * 65536,
        actor->bones[actor->custom_bone_iter][1] / 360.f * 65536,
        actor->bones[actor->custom_bone_iter][2] / 360.f * 65536
    );
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