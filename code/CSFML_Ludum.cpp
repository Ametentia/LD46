#include <SFML/System.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

            default: {} break;
        }
    }

    v2i position = sfMouse_getPositionRenderWindow(global_window);
    current_input->mouse_position = sfRenderWindow_mapPixelToCoords(global_window, position, global_view);

    Game_Controller *current_keyboard = &current_input->controllers[0];
    Game_Controller *prev_keyboard    = &prev_input->controllers[0];

    CSFMLProcessGameButton(&current_keyboard->move_left, &prev_keyboard->move_left, sfKeyboard_isKeyPressed(sfKeyA));
    CSFMLProcessGameButton(&current_keyboard->move_right, &prev_keyboard->move_right, sfKeyboard_isKeyPressed(sfKeyD));
    CSFMLProcessGameButton(&current_keyboard->menu, &prev_keyboard->menu, sfKeyboard_isKeyPressed(sfKeyEscape));
    CSFMLProcessGameButton(&current_keyboard->interact, &prev_keyboard->interact, sfKeyboard_isKeyPressed(sfKeyE));
    CSFMLProcessGameButton(&current_keyboard->jump, &prev_keyboard->jump, sfKeyboard_isKeyPressed(sfKeySpace));
}

#if LUDUM_WINDOWS
#include <windows.h>

//int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
int wmain(int argc, char **argv) {
    SetCurrentDirectoryA("data");
#else
int main(int argc, char **argv) {
    chdir("data");
#endif


    sfVideoMode mode = { 1280, 720, 24 };
    sfContextSettings settings = {};
    settings.antialiasingLevel = 8;

    global_window = sfRenderWindow_create(mode, "Ludum Dare 46", sfTitlebar | sfClose, &settings);
    if (!global_window) {
        printf("Error :: Failed to create render window\n");
        return 1;
    }

    global_view_size = V2(mode.width, mode.height);
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
    }

    sfClock_destroy(timer);
    sfRenderWindow_close(global_window);
    sfRenderWindow_destroy(global_window);
}
