#ifndef OBJECT_COLLISION_H
#define OBJECT_COLLISION_H

struct Object* get_mario_actor_from_ray(Vec3f from, Vec3f to);
void detect_object_collisions(void);

#endif // OBJECT_COLLISION_H
