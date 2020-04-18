#if !defined(LUDUM_H_)
#define LUDUM_H_ 1
#include "Ludum_Types.h"

struct Player {
    v2 position;
    v2 velocity;
    u8 health;
};

struct Play_State {
    b32 initialised;
    Player player[1];
};

enum Level_Type {
    LevelType_Play
};

struct Level_State {
    Level_Type type;
    union {
        Play_State play;
    };
    Level_State *next;
};

struct Game_State {
    b32 initialised;
    sfRenderWindow *renderer;
    sfView *view;
    Level_State *current_state;
};
#endif  // LUDUM_H_
