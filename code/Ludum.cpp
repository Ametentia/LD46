internal Level_State *CreateLevelState(Game_State *state, Level_Type type) {
    Level_State *result = cast(Level_State *) Alloc(sizeof(Level_State));
    memset(result, 0, sizeof(Level_State));

    result->type = type;
    result->next = state->current_state;
    state->current_state = result;

    return result;
}

internal void UpdateRenderPlayState(Game_State *state, Play_State *playState, Game_Input *input) {
    if(!playState->initialised) {
        CreateLevelState(state, LevelType_Play);
        Player *player = &playState->player[0];
        player->position = V2(640, 360);
        playState->initialised = true;
    }
    sfCircleShape *c = sfCircleShape_create();
    sfCircleShape_setRadius(c, 40.0);
    sfCircleShape_setPosition(c, playState->player->position);
    sfRenderWindow_drawCircleShape(state->renderer, c, 0);
}

internal void UpdateRenderLudum(Game_State *state, Game_Input *input) {
    if(!state->initialised) {
        CreateLevelState(state, LevelType_Play);
        state->initialised = true;
    }
    Level_State *current_state = state->current_state;
    switch(current_state->type) {
        case LevelType_Play: {
            Play_State *play = &current_state->play;
            UpdateRenderPlayState(state, play, input);
        }
        break;
    }
}
