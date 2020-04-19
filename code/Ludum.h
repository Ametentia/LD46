#if !defined(LUDUM_H_)
#define LUDUM_H_ 1

#include "Ludum_Types.h"
#include "Ludum_Maths.h"

enum Direction_Flags {
    Direction_Left   = 0x1,
    Direction_Right  = 0x2,
    Direction_Top    = 0x4,
    Direction_Bottom = 0x8,
    Direction_All = (Direction_Left | Direction_Right | Direction_Top | Direction_Bottom)
};

struct Bounding_Box {
    u32 direction_flags;
    v2 centre;
    v2 half_dim;
};

// @Note: This is the level format
//
#pragma pack(push, 1)

/// [ Level_Header       ]
//  [ Level_Segment 0    ]
//  [ Level_Segment 1    ]
//  [       ...          ]
//  [ Level_Segment 126  ]
//  [ Level_Segment 127  ]
//  [ Entity Array       ]
//  [ Bounding Box Array ]

struct Level_Segment {
    b32 in_use;

    u32 texture_number;

    u32 entity_count;
    u32 box_count;

    u32 box_range_start;
    u32 box_range_one_past_last;

    u32 entity_range_start;
    u32 entity_range_one_past_last;
};

struct Level_Header {
    u8 signature[4];
    u32 segment_dim[2];

    u32 total_box_count;
    u32 total_entity_count;
    // @Todo: Maybe lights are sepearte
};

#pragma pack(pop)

enum Asset_Type {
    Asset_Texture,
    Asset_Sound,
    Asset_Music,
    Asset_Font
};

enum Asset_Flags {
    AssetFlag_Animation = 0x1,
};

struct Asset {
    b32 occupied;
    u32 flags;
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

enum Entity_Type {
    EntityType_Rocks = 0,
    EntityType_Player,
    EntityType_Torch,

    EntityType_Count,

    EntityType_Light = 10000 // @Note: This doesn't have a texture so I am just putting it out of the way
};

struct Entity {
    u32 type;
    v2 position;
    v2 scale;

    Bounding_Box box;
};

struct Fire_Ball {
    b32 active;
    v2 position;
    v2 velocity;
    f32 radius;
    f32 rotation;
    b32 held;
};

enum Entity_State_Flags {
    EntityState_OnGround = 0x1,
    EntityState_Falling  = 0x2,
    EntityState_FireballBreak = 0x4,
    EntityState_HoldingFireball = 0x8
};

struct Player {
    Entity *entity_handle;
    Animation animation;

    v2 half_dim;

    f32 jump_time;
    f32 fall_time;
    f32 floor_time;

    u32 state_flags;

    b32 holding_fireball;
    b32 fireball_break;

    v2 position;
    v2 velocity;

    s8 health;
};

struct World {
    Level_Segment segments[128]; // @Note: 16x8 grid

    u32 box_count;
    Bounding_Box *boxes;

    u32 entity_count;
    Entity *entities;
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

    World world;

    Fire_Ball fire_balls[3];

    Animation torch_animation;
    Animation candle[3];

    ParticleSpawner rain[1];
    ParticleSpawner wind[1];

    f32 total_time;
    f32 distance_scale;
    sfShader *shader;
};

struct Edit_Segment {
    b32 in_use;

    s32 texture_index;

    u32 grid_x;
    u32 grid_y;

    u32 entity_count;
    Entity entities[64];

    u32 box_count;
    Bounding_Box boxes[128];
};

enum Edit_Mode {
    EditMode_BoundingBox,
    EditMode_Entity,
    EditMode_Segment
};

struct Edit_State {
    b32 initialised;

    v2 displacement;

    b32 is_editing;
    v2 first_mouse_down;

    Edit_Mode mode;
    s32 entity_type;

    Edit_Segment segments[16][8];
    Edit_Segment *current_segment;

    f32 zoom_factor;
    v2 last_mouse;
    v2 camera_pos;
    f32 zoom;
    sfView *edit_view;

    v2 entity_scales[EntityType_Count];
    Animation animations[EntityType_Count];
};

enum Level_Type {
    LevelType_Play,
    LevelType_Logo,
    LevelType_Edit
};

struct Level_State {
    Level_Type type;
    union {
        Play_State play;
        Logo_State logo;
        Edit_State edit;
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

    if (!a || !b) { return result; }

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

internal void CopySize(void *to, void *from, umm size) {
    u8 *a = cast(u8 *) to;
    u8 *b = cast(u8 *) from;

    for (umm it = 0; it < size; ++it) {
        a[it] = b[it];
    }
}

internal b32 HasFlags(u32 flags, u32 check) {
    b32 result = (flags & check) == check;
    return result;
}

internal void RemoveFlags(u32 *flags, u32 remove) {
    *flags &= ~remove;
}

internal void AddFlags(u32 *flags, u32 add) {
    *flags |= add;
}

#endif  // LUDUM_H_
