#if !defined(LUDUM_TYPES_H_)
#define LUDUM_TYPES_H_ 1

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uintptr_t umm;
typedef  intptr_t smm;

typedef float  f32;
typedef double f64;

typedef u32 b32;

typedef sfVector2f v2;
typedef sfVector2i v2i;

typedef sfVector3f v3;

#define internal static
#define local static
#define global static

#define cast
#define Swap(a, b) { auto __temp = a; a = b; b = __temp; }
#define Assert(exp) do { if (!(exp)) { printf("Assertion :: %s (%s:%d)\n", #exp, __FILE__, __LINE__); asm("int3"); } } while (0)

#define Abs(x) ((x) < 0 ? -(x) : (x))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Square(x) ((x) * (x))
#define Alloc(size) malloc(size)

struct Game_Button {
    b32 pressed;
    u32 transitions;
};

struct Game_Controller {
    // @Todo: Add more buttons as we need once we have more of the game idea
    union {
        struct {
            Game_Button move_left;
            Game_Button move_right;
            Game_Button jump;

            Game_Button interact;
            Game_Button menu;
        };

        Game_Button buttons[5];
    };
};

struct Game_Input {
    f32 delta_time;
    b32 requested_quit;
    b32 requested_fullscreen;

    Game_Controller controllers[4];

    Game_Button mouse_buttons[3];
    v2 mouse_position;
};

internal b32 IsPressed(Game_Button button) {
    b32 result = (button.pressed);
    return result;
}

internal b32 JustPressed(Game_Button button) {
    b32 result = (button.pressed) && (button.transitions > 0);
    return result;
}

internal b32 WasPressed(Game_Button button) {
    b32 result = (!button.pressed) && (button.transitions > 0);
    return result;
}

#endif  // LUDUM_TYPES_H_
