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

    v2 scale;
    v2 frame_size;

    u32 rows;
    u32 columns;

    u32 total_frames;

    u32 current_frame;
    f32 accumulator;
    f32 time_per_frame;
    b32 flip;
    b32 pause;
};

struct Bounding_Box {
    v2 centre;
    v2 half_dim;
};

struct Fire_Ball {
    b32 active;
    v2 position;
    v2 velocity;
    f32 radius;
    f32 rotation;
    b32 held;
};

struct Player {
    v2 half_dim;

    f32 jump_time;
    b32 can_jump;
    b32 falling;
    f32 fall_time;
    f32 floor_time;
    v2 position;
    v2 velocity;

    s8 health;
    b32 holding_fireball;
    b32 fireball_break;
    b32 on_ground;
};

struct Square {
    b32 occupied;
    Bounding_Box box;
};

struct Particle {
    b32 active;
    v2 position;
    v2 velocity;
    v2 size;
    f32 rotation;
    f32 life_time;
};

struct ParticleSpawner {
    v2 centre;
    v2 half_size;
    v2 velocity_min;
    v2 velocity_max;
    v2 size_min;
    v2 size_max;
    v2 rotation_range;
    v2 life_time_range;
    b32 strict_x;
    char* assetName;
    u32 max_particles;
    // Rate is particles/second
    f32 rate;
    Particle *particles;
    u8 transparency = 255;
};

struct Logo_State {
    b32 initialised = false;
    f32 delta_rate;
    f32 rate;
    f32 opacity;
};

struct Play_State {
    b32 initialised;
    Player player[1];

    u32 next_square;
    Square level[128];

    b32 is_editing;
    v2 first_mouse_down;
    Fire_Ball fire_balls[3];

    Animation torch_animation;
    Animation debug_anim;
    Animation candle[3];

    ParticleSpawner rain[1];
    ParticleSpawner wind[1];

    f32 total_time;
    f32 distance_scale;
    sfShader *shader;
};

enum Level_Type {
    LevelType_Play,
    LevelType_Logo
};

struct Level_State {
    Level_Type type;
    union {
        Play_State play;
        Logo_State logo;
    };
    Level_State *next;
};

struct Game_State {
    b32 initialised;

    sfRenderWindow *renderer;
    Asset_Manager assets;

    sfView *view;
    Level_State *current_state;

    v2 player_pos;
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
