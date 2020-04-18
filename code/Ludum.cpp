#include "Ludum_Assets.cpp"

internal Animation CreateAnimationFromTexture(sfTexture *texture, v2 scale, u32 rows, u32 columns, f32 time_per_frame = 0.1f) {
    Animation result;
    result.texture = texture;

    result.scale = scale;

    v2 tex_size = V2(sfTexture_getSize(texture));
    result.frame_size.x = tex_size.x / cast(f32) columns;
    result.frame_size.y = tex_size.y / cast(f32) rows;

    result.rows = rows;
    result.columns = columns;

    result.total_frames = (rows * columns);

    result.time_per_frame = time_per_frame;
    result.current_frame  = 0;
    result.accumulator    = 0;
    result.flip           = false;
    result.pause          = false;

    return result;
}

internal void AddFireBall(Play_State *playState) {
    for(u8 i = 0; i < 3; i++) {
        Fire_Ball *fireball = &playState->fire_balls[i];
        Player *player = &playState->player[0];
        if(!fireball->active) {
            fireball->active = true;
            fireball->held = true;
            fireball->radius = 15;
            player->holding_fireball = true;
            return;
        }
    }
}

internal void UpdateRenderAnimation(Game_State *state, Animation *animation, v2 position, f32 delta_time) {
    if(!animation->pause) {
        animation->accumulator += delta_time;
    }
    if (animation->accumulator >= animation->time_per_frame) {
        animation->current_frame += 1;
        if (animation->current_frame >= animation->total_frames) {
            animation->current_frame = 0;
        }

        animation->accumulator = 0;
    }

    u32 row = animation->current_frame / animation->columns;
    u32 col = animation->current_frame % animation->columns;

    sfSprite *sprite = sfSprite_create();
    sfSprite_setTexture(sprite, animation->texture, true);
    sfIntRect rect;
    rect.top    = cast(u32) (row * animation->frame_size.y);
    rect.left   = cast(u32) (col * animation->frame_size.x);
    rect.width  = cast(u32) (animation->frame_size.x);
    rect.height = cast(u32) (animation->frame_size.y);

    sfSprite_setTextureRect(sprite, rect);
    sfSprite_setOrigin(sprite, 0.5f * animation->frame_size);
    sfSprite_setPosition(sprite, position);
    v2 scale = animation->scale;
    if(animation->flip){
        scale.x *= -1;
    }
    sfSprite_setScale(sprite, scale);

    sfRenderWindow_drawSprite(state->renderer, sprite, 0);

    sfSprite_destroy(sprite);
}

internal Level_State *CreateLevelState(Game_State *state, Level_Type type) {
    Level_State *result = cast(Level_State *) Alloc(sizeof(Level_State));
    memset(result, 0, sizeof(Level_State));

    result->type = type;
    result->next = state->current_state;
    state->current_state = result;

    return result;
}


// @Todo: Maybe this should just return the axis
internal b32 Overlaps(Bounding_Box *a, Bounding_Box *b) {
    v2 centre = a->centre - b->centre;
    v2 size   = a->half_dim + b->half_dim;

    b32 result = (Abs(centre.x) <= size.x) && (Abs(centre.y) <= size.y);
    return result;
}

internal u32 GetSmallestAxis(Bounding_Box *a, Bounding_Box *b) {
    u32 result = 0;

    v2 centre = a->centre - b->centre;
    v2 size   = a->half_dim + b->half_dim;
    if (size.x - Abs(centre.x) < (size.y - Abs(centre.y))) {
        if (centre.x > 0) { result = 1; }
    }
    else {
        if (centre.y < 0) { result = 2; }
        else { result = 3; }
    }

    return result;
}

internal void UpdateRenderFireBalls(Game_State *state, Play_State *playState, Game_Input *input) {
    for(u8 i = 0; i < 3; i++) {
        Fire_Ball *fireball = &playState->fire_balls[i];
        if(!fireball->active) { continue; }

        Player *player = &playState->player[0];
        Game_Controller *controller = &input->controllers[0];


        if(fireball->held) {
            fireball->rotation-=90*input->delta_time;
            sfCircleShape *r = sfCircleShape_create();
            sfCircleShape_setRadius(r, fireball->radius);
            sfCircleShape_setOrigin(r, V2(fireball->radius, fireball->radius));
            sfCircleShape_setRotation(r, fireball->rotation);
            sfCircleShape_setTexture(r, GetAsset(&state->assets, "Fireball")->texture, false);

            fireball->position = V2(player->position.x + 40, player->position.y-10);
            if(JustPressed(controller->interact)) {
                fireball->held = false;
                v2 dir = input->mouse_position - fireball->position;
                f32 len = length(dir);
                fireball->velocity = 3 * dir * (1/sqrt(len));
                player->holding_fireball = false;
                player->fireball_break = true;
            }

            sfCircleShape_setPosition(r, fireball->position);
            sfRenderWindow_drawCircleShape(state->renderer, r, 0);
            sfCircleShape_destroy(r);
            continue;
        }
        else {
            fireball->rotation-=360*input->delta_time;
        }
        if(length(fireball->velocity) < 30000) {
            fireball->velocity += fireball->velocity * 100 * input->delta_time;
        }
        if(fireball->radius < 3) {
            fireball->active = false;
        }

        fireball->radius -= 2*input->delta_time;
        fireball->position += input->delta_time * fireball->velocity;
        sfColor c = sfRed;
        sfCircleShape *r = sfCircleShape_create();
        sfCircleShape_setOrigin(r, V2(fireball->radius, fireball->radius));
        sfCircleShape_setRadius(r, fireball->radius);
        sfCircleShape_setRotation(r, fireball->rotation);
        sfCircleShape_setTexture(r, GetAsset(&state->assets, "Fireball")->texture, false);
        sfCircleShape_setPosition(r, fireball->position);
        sfRenderWindow_drawCircleShape(state->renderer, r, 0);
        sfCircleShape_destroy(r);
    }
}

internal void UpdateRenderPlayState(Game_State *state, Play_State *playState, Game_Input *input) {
    if(!playState->initialised) {
        Player *player = &playState->player[0];
        player->position = V2(640, 360);
        player->half_dim = V2(35, 50);
        player->velocity = V2(0, 250);
        player->health = 2;

        Asset *anim = GetAsset(&state->assets, "IbbSheet");
        Assert(anim);

        Asset *torch = GetAsset(&state->assets, "TorchSheet");
        Assert(torch);

        Asset *candle_low = GetAsset(&state->assets, "CandleLow");
        Assert(candle_low);
        Asset *candle_medium = GetAsset(&state->assets, "CandleMid");
        Assert(candle_medium);
        Asset *candle_high = GetAsset(&state->assets, "CandleHigh");
        Assert(candle_high);

        sfTexture_setSmooth(torch->texture, true);
        playState->torch_animation = CreateAnimationFromTexture(torch->texture, V2(0.6, 0.6), 4, 4, 0.13f);
        playState->debug_anim = CreateAnimationFromTexture(anim->texture, V2(0.16, 0.16), 2, 3, 0.09f);
        playState->candle[0] = CreateAnimationFromTexture(candle_low->texture, V2(0.16, 0.16), 1, 3, 0.08f);
        playState->candle[1] = CreateAnimationFromTexture(candle_medium->texture, V2(0.16, 0.16), 1, 3, 0.08f);
        playState->candle[2] = CreateAnimationFromTexture(candle_high->texture, V2(0.16, 0.16), 1, 3, 0.08f);

        playState->initialised = true;

        playState->total_time = 0;
        playState->distance_scale = 0.08f;
        const char *vertex_code = R"VERT(
            varying out vec2 frag_position;

            void main() {
                gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
                gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

                frag_position = vec2(gl_Vertex.x, gl_Vertex.y);
            }
        )VERT";

        const char *frag_code = R"FRAG(
            in vec2 frag_position;

            varying out vec4 final_colour;

            uniform sampler2D noise;
            uniform float time;

            uniform sampler2D image;
            uniform float distance_scale;
            uniform vec2 light_position;

            void main() {
                vec2 dir = light_position - frag_position;

                vec2 noise_uv = vec2(abs(sin(0.3 * time)), abs(cos(0.4 * time)));
                float dist = (distance_scale - (0.04 * texture2D(noise, noise_uv))) * length(dir);

                float attenuation = 1.0 / (1.0 + (0.09 * dist) + (0.032 * (dist * dist)));

                vec3 ambient = attenuation * vec3(0.5, 0.5, 1);
                final_colour = vec4(ambient, 1) * texture2D(image, gl_TexCoord[0].xy);
            }
        )FRAG";

        playState->shader = sfShader_createFromMemory(vertex_code, 0, frag_code);
        Assert(playState->shader);
    }

    f32 dt = input->delta_time;

    v2 view_size = sfView_getSize(state->view);

    Game_Controller *controller = &input->controllers[0];
    Player *player = &playState->player[0];
    player->falling = true;

    if (JustPressed(input->mouse_buttons[0])) {
        u32 x = cast(u32) (input->mouse_position.x / 64);
        u32 y = cast(u32) (input->mouse_position.y / 64);
        Assert(x < 64 && y < 64);

        playState->level[x][y].occupied = true;
    }

    player->velocity.x = 0;
    Animation *playerAnimation = &playState->debug_anim;
    if (IsPressed(controller->move_left)) { 
        player->velocity.x = -220; 
        playerAnimation->flip = true; 
        playerAnimation->pause= false; 
    }
    else if (IsPressed(controller->move_right)) {
        player->velocity.x = 220; 
        playerAnimation->flip = false; 
        playerAnimation->pause= false; 
    }

    player->jump_time -= dt;
    if (IsPressed(controller->jump)) {
        if (player->can_jump && player->fall_time < 0.3 && player->floor_time > 0.04f) {
            player->velocity.y = -350;
            player->can_jump = false;
            player->jump_time = 0.3f;
            player->falling = true;
            player->floor_time = 0;
            playerAnimation->pause = false; 
        }
        else if (player->jump_time > 0) {
            player->velocity.y -= 830*dt;
        }
    }

    player->velocity.x *= (player->can_jump ? 1 : 0.85);
    player->velocity.y += (dt * 980);
    player->position += (dt * player->velocity);

    if (JustPressed(input->mouse_buttons[1])) {
        playState->distance_scale = Min(playState->distance_scale + 0.02f, 1.0f);
    }

    if (JustPressed(input->mouse_buttons[2])) {
        playState->distance_scale = Max(playState->distance_scale - 0.02f, 0.0);
    }

    // @Speed: Looking up the noise texture every frame
    Asset *noise = GetAsset(&state->assets, "PNoise");

    playState->total_time += dt;
    sfShader_setTextureUniform(playState->shader, "noise", noise->texture);
    sfShader_setFloatUniform(playState->shader, "time", playState->total_time);
    sfShader_setFloatUniform(playState->shader, "distance_scale", playState->distance_scale);
    sfShader_setVec2Uniform(playState->shader, "light_position", player->position);
    sfShader_bind(playState->shader);

    UpdateRenderAnimation(state, &playState->torch_animation, V2(600, view_size.y - 145.5), dt);
    UpdateRenderAnimation(state, &playState->debug_anim, player->position, dt);
    u32 candle = Max(Min(player->health, 2), 0);
    UpdateRenderAnimation(state, &playState->candle[candle], player->position + V2(40, -20), dt);

    sfShader_bind(0);

    if (player->position.y + player->half_dim.y >= view_size.y) {
        player->position.y = view_size.y - player->half_dim.y;
        player->velocity.y = 0;
        player->can_jump = true;
        player->floor_time += dt;
        player->falling = false;
        player->fall_time = 0;
    }

    Bounding_Box player_box;
    player_box.centre   = player->position;
    player_box.half_dim = player->half_dim;

    sfColor c = sfRed;

    sfRectangleShape *r = sfRectangleShape_create();
    sfRectangleShape_setOrigin(r, V2(32, 32));
    sfRectangleShape_setSize(r, V2(64, 64));
    sfRectangleShape_setFillColor(r, sfTransparent);
    sfRectangleShape_setOutlineColor(r, c);
    sfRectangleShape_setOutlineThickness(r, 2);

    v2 grid_pos = V2((64 * cast(u32) (input->mouse_position.x / 64)), (64 * cast(u32) (input->mouse_position.y / 64)));
    sfRectangleShape_setPosition(r, grid_pos);

    sfRenderWindow_drawRectangleShape(state->renderer, r, 0);

    for (u32 x = 0; x < 64; ++x) {
        for (u32 y = 0; y < 64; ++y) {
            if (playState->level[x][y].occupied) {
                Bounding_Box other = { V2(x * 64, y * 64), V2(32, 32) };
                if (Overlaps(&player_box, &other)) {
                    c = sfGreen;

                    v2 full_size = player->half_dim + other.half_dim;

                    u32 axis = GetSmallestAxis(&player_box, &other);
                    switch (axis) {
                        case 0: {
                            player->position.x = other.centre.x - full_size.x;
                        }
                        break;
                        case 1: {
                            player->position.x = other.centre.x + full_size.x;
                        }
                        break;
                        case 2: {
                            player->position.y = other.centre.y - full_size.y;
                            player->velocity.y = 0;
                            player->can_jump = true;
                            player->falling = false;
                            player->floor_time += dt;
                            player->fall_time = 0;
                        }
                        break;
                        case 3: {
                            player->position.y = other.centre.y + full_size.y;
                            player->velocity.y = -0.45f * player->velocity.y;
                            player->jump_time = 0;
                        }
                        break;
                    }
                }

                sfRectangleShape_setPosition(r, other.centre);
                sfRenderWindow_drawRectangleShape(state->renderer, r, 0);
            }
        }
    }
    if (player->falling) {
        player->fall_time += dt;
    }
    else {
        playerAnimation->pause= true; 
    }
    UpdateRenderFireBalls(state, playState, input);
    if(JustPressed(controller->interact) && !player->holding_fireball && !player->fireball_break){
        player->holding_fireball = true;
        AddFireBall(playState);
        player->health--;
    }
    else {
        player->fireball_break = false;
    }

    sfRectangleShape_destroy(r);

    sfRectangleShape *bbox = sfRectangleShape_create();

    sfRectangleShape_setOrigin(bbox, player_box.half_dim);
    sfRectangleShape_setSize(bbox, 2 * player_box.half_dim);
    sfRectangleShape_setPosition(bbox, player_box.centre);
    sfRectangleShape_setFillColor(bbox, sfTransparent);
    sfRectangleShape_setOutlineColor(bbox, c);
    sfRectangleShape_setOutlineThickness(bbox, 2);

    //sfRenderWindow_drawRectangleShape(state->renderer, bbox, 0);

    sfRectangleShape_destroy(bbox);
}

internal void LudumUpdateRender(Game_State *state, Game_Input *input) {
    if (!state->initialised) {
        CreateLevelState(state, LevelType_Play);

        state->player_pos = V2(250, 400);
        InitAssets(&state->assets, 64);
        LoadAsset(&state->assets, "IbbSheet", Asset_Texture);
        LoadAsset(&state->assets, "TorchSheet", Asset_Texture);
        LoadAsset(&state->assets, "CandleLow", Asset_Texture);
        LoadAsset(&state->assets, "CandleMid", Asset_Texture);
        LoadAsset(&state->assets, "CandleHigh", Asset_Texture);
        LoadAsset(&state->assets, "PNoise", Asset_Texture);
        LoadAsset(&state->assets, "Fireball", Asset_Texture);

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
