#ifdef TOUCH_CONTROLS
#include "controller_touchscreen_textures.h"

ALIGNED8 const u8 texture_touch_button[] = {
"textures/touchcontrols/touch_button.rgba16.png"
};
ALIGNED8 const u8 texture_touch_button_dark[] = {
"textures/touchcontrols/touch_button_dark.rgba16.png"
};
ALIGNED8 const u8 texture_touch_cup[] = {
"textures/touchcontrols/touch_cup.rgba16.png"
};
ALIGNED8 const u8 texture_touch_cdown[] = {
"textures/touchcontrols/touch_cdown.rgba16.png"
};
ALIGNED8 const u8 texture_touch_cleft[] = {
"textures/touchcontrols/touch_cleft.rgba16.png"
};
ALIGNED8 const u8 texture_touch_cright[] = {
"textures/touchcontrols/touch_cright.rgba16.png"
};
ALIGNED8 const u8 texture_touch_up[] = {
"textures/touchcontrols/touch_up.rgba16.png"
};
ALIGNED8 const u8 texture_touch_down[] = {
"textures/touchcontrols/touch_down.rgba16.png"
};
ALIGNED8 const u8 texture_touch_left[] = {
"textures/touchcontrols/touch_left.rgba16.png"
};
ALIGNED8 const u8 texture_touch_right[] = {
"textures/touchcontrols/touch_right.rgba16.png"
};
ALIGNED8 const u8 texture_touch_check[] = {
"textures/touchcontrols/touch_check.rgba16.png"
};
ALIGNED8 const u8 texture_touch_cross[] = {
"textures/touchcontrols/touch_cross.rgba16.png"
};
ALIGNED8 const u8 texture_touch_reset[] = {
"textures/touchcontrols/touch_reset.rgba16.png"
};
ALIGNED8 const u8 texture_touch_snap[] = {
"textures/touchcontrols/touch_snap.rgba16.png"
};
ALIGNED8 const u8 texture_touch_trash[] = {
"textures/touchcontrols/touch_trash.rgba16.png"
};

const u8 *const touch_textures[TOUCH_TEXTURE_COUNT] = {
    texture_touch_button,
    texture_touch_button_dark,
    texture_touch_cup,
    texture_touch_cdown,
    texture_touch_cleft,
    texture_touch_cright,
    texture_touch_up,
    texture_touch_down,
    texture_touch_left,
    texture_touch_right,
    texture_touch_check,
    texture_touch_cross,
    texture_touch_reset,
    texture_touch_snap,
    texture_touch_trash,
};
#endif