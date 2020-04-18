#if !defined(LUDUM_H_)
#define LUDUM_H_ 1

#include "Ludum_Types.h"
#include "Ludum_Maths.h"

enum Asset_Type {
    Asset_Texture,
    Asset_Sound,
    Asset_Music,
    Asset_Font
};

struct Asset {
    b32 occupied;
    Asset_Type type;
    const char *name;

    union {
        sfTexture *texture;
        sfSoundBuffer *sound;
        sfMusic *music;
        sfFont *font;

        void *__checker; // @Debug: Do not use
    };

    Asset *next;
};

struct Asset_Manager {
    u32 hash_table_size;
    Asset *assets;
};

struct Animation {
    sfTexture *texture;

    v2 size;
    v2 frame_size;

    u32 rows;
    u32 columns;

    u32 total_frames;

    u32 current_frame;
    f32 accumulator;
    f32 time_per_frame;
    b32 flip;
};

struct Bounding_Box {

};

struct Game_State {
    b32 initialised;

    sfRenderWindow *renderer;
    Asset_Manager assets;

    v2 player_pos;
    Animation debug_anim;
};

internal b32 StringsEqual(const char *a, const char *b) {
    b32 result = false;

    u32 it;
    for (it = 0; a[it] && b[it]; ++it) {
        if (a[it] != b[it]) { break; }
    }

    result = (a[it] == b[it]) && (a[it] == 0);
    return result;
}

internal void ClearSize(void *ptr_init, umm size) {
    u8 *ptr = cast(u8 *) ptr_init;

    for (umm it = 0; it < size; ++it) { ptr[it] = 0; }
}

#endif  // LUDUM_H_
