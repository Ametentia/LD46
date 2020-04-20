#define _CRT_SECURE_NO_WARNINGS 1
#include <SFML/System.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "Ludum.h"
#include "Ludum.cpp"

global sfRenderWindow *global_window;
global sfView *global_view;
global v2 global_view_size;
global b32 global_running;

internal void CSFMLProcessGameButton(Game_Button *current, Game_Button *prev, b32 pressed) {
    current->pressed = pressed;
    current->transitions = (current->pressed != prev->pressed) ? 1 : 0;
}

internal void CSFMLHandleInputs(Game_Input *current_input, Game_Input *prev_input) {
    sfEvent event;
    current_input->mouse_wheel_delta = 0;
    while (sfRenderWindow_pollEvent(global_window, &event)) {
        switch (event.type) {
            case sfEvtClosed: {
                current_input->requested_quit = true;
            }
            break;

            case sfEvtResized: {
                global_view_size.x = cast(f32) event.size.width;
                global_view_size.y = cast(f32) event.size.height;
            }
            break;
            case sfEvtMouseWheelScrolled: {
                current_input->mouse_wheel_delta = event.mouseWheel.x;
            }
            break;

            default: {} break;
        }
    }

    v2i position = sfMouse_getPositionRenderWindow(global_window);
    const sfView *view = sfRenderWindow_getView(global_window);
    current_input->mouse_position = sfRenderWindow_mapPixelToCoords(global_window, position, view);
    current_input->screen_mouse = V2(position.x, position.y);

    Game_Controller *current_keyboard = &current_input->controllers[0];
    Game_Controller *prev_keyboard    = &prev_input->controllers[0];

    CSFMLProcessGameButton(&current_keyboard->move_left, &prev_keyboard->move_left, sfKeyboard_isKeyPressed(sfKeyA));
    CSFMLProcessGameButton(&current_keyboard->move_right, &prev_keyboard->move_right, sfKeyboard_isKeyPressed(sfKeyD));
    CSFMLProcessGameButton(&current_keyboard->menu, &prev_keyboard->menu, sfKeyboard_isKeyPressed(sfKeyEscape));
    CSFMLProcessGameButton(&current_keyboard->interact, &prev_keyboard->interact, sfKeyboard_isKeyPressed(sfKeyE));
    CSFMLProcessGameButton(&current_keyboard->jump, &prev_keyboard->jump, sfKeyboard_isKeyPressed(sfKeySpace));

    CSFMLProcessGameButton(&current_input->mouse_buttons[0], &prev_input->mouse_buttons[0], sfMouse_isButtonPressed(sfMouseLeft));
    CSFMLProcessGameButton(&current_input->mouse_buttons[1], &prev_input->mouse_buttons[1], sfMouse_isButtonPressed(sfMouseRight));
    CSFMLProcessGameButton(&current_input->mouse_buttons[2], &prev_input->mouse_buttons[2], sfMouse_isButtonPressed(sfMouseMiddle));


    u32 num = 1;
    for (u32 it = sfKeyF1; it < sfKeyF13; ++it) {
        CSFMLProcessGameButton(&current_input->f[num], &prev_input->f[num], sfKeyboard_isKeyPressed(cast(sfKeyCode) it));
        num += 1;
    }

    CSFMLProcessGameButton(&current_input->debug_next, &prev_input->debug_next, sfKeyboard_isKeyPressed(sfKeyRBracket));

    CSFMLProcessGameButton(&current_input->debug_prev, &prev_input->debug_prev, sfKeyboard_isKeyPressed(sfKeyLBracket));

    CSFMLProcessGameButton(&current_input->debug_up, &prev_input->debug_up, sfKeyboard_isKeyPressed(sfKeyUp));
    CSFMLProcessGameButton(&current_input->debug_down, &prev_input->debug_down, sfKeyboard_isKeyPressed(sfKeyDown));
}

#if LUDUM_WINDOWS
#include <windows.h>

//int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
int wmain(int argc, char **argv) {
    SetCurrentDirectoryA("data");
#else
#include <unistd.h>
int main(int argc, char **argv) {
    chdir("data");
#endif


    sfVideoMode mode = { 1280, 720, 24 };
//    sfVideoMode mode = { 1920, 1080, 24 };
    sfContextSettings settings = {};
    settings.antialiasingLevel = 8;

    global_window = sfRenderWindow_create(mode, "Ludum Dare 46", sfTitlebar | sfClose, &settings);
    if (!global_window) {
        printf("Error :: Failed to create render window\n");
        return 1;
    }

    global_view_size = V2(1280, 720);
    global_view = sfView_create();

    sfView_setSize(global_view, global_view_size);
    sfView_setCenter(global_view, 0.5f * global_view_size);
    sfRenderWindow_setView(global_window, global_view);

    sfRenderWindow_setVerticalSyncEnabled(global_window, true);

    Game_Input inputs[2] = {};
    Game_Input *current_input = &inputs[0];
    Game_Input *prev_input    = &inputs[1];

    Game_State __state = {};
    Game_State *state  = &__state;
    state->renderer    = global_window;
    state->view        = global_view;

    global_running = true;
    sfClock *timer = sfClock_create();

    while (global_running) {
        CSFMLHandleInputs(current_input, prev_input);

        sfRenderWindow_clear(global_window, sfBlack);
        LudumUpdateRender(state, current_input);
        sfRenderWindow_display(global_window);

        global_running = !current_input->requested_quit;
        Swap(current_input, prev_input);

        sfTime elapsed = sfClock_getElapsedTime(timer);
        current_input->delta_time = sfTime_asSeconds(elapsed);
        sfClock_restart(timer);

        // @Hack: This should only happen for one of the frames because the initial loading of assets
        // makes the delta time too large meaning it causes the player to jump like 500 units and glitch
        // through the ground
        // :)
        if (current_input->delta_time > 0.2) { current_input->delta_time = 0; }
    }

    sfClock_destroy(timer);
    sfRenderWindow_close(global_window);
    sfRenderWindow_destroy(global_window);
}
