#include "Ludum_Assets.cpp"

internal void LoadLevelFromFile(Game_State *state, World *world, const char *filename) {
    FILE *handle = fopen(filename, "rb");

    Level_Header header;
    fread(&header, sizeof(Level_Header), 1, handle);

    if (header.signature[0] != 'A' ||
        header.signature[1] != 'M' ||
        header.signature[2] != 'T' ||
        header.signature[3] != 'L')
    {
        printf("Failed signature check\n");
        return;
    }

    printf("Segment dim is { %d, %d }\n", header.segment_dim[0], header.segment_dim[1]);
    printf("Entities: %d, Boxes: %d\n", header.total_entity_count, header.total_box_count);

    fread(world->segments, sizeof(world->segments), 1, handle);

    world->entity_count = header.total_entity_count;
    world->entities = cast(Entity *) Alloc(world->entity_count * sizeof(Entity));

    fread(world->entities, world->entity_count * sizeof(Entity), 1, handle);

    world->box_count = header.total_box_count;
    world->boxes = cast(Bounding_Box *) Alloc(world->box_count * sizeof(Bounding_Box));

    fread(world->boxes, world->box_count * sizeof(Bounding_Box), 1, handle);

    fclose(handle);
}

internal void WriteLevelToFile(Game_State *state, Edit_State *edit) {
    FILE *handle = fopen("Level.aml", "wb");

    // Write out the header
    u32 total_box_count = 0;
    u32 total_entity_count = 0;
    for (u32 x = 0; x < 16; ++x) {
        for (u32 y = 0; y < 8; ++y) {
            Edit_Segment *edit_seg = &edit->segments[x][y];
            total_box_count += edit_seg->box_count;
            total_entity_count += edit_seg->entity_count;
        }
    }

    if (total_box_count == 0 && total_entity_count == 0) {
        printf("Empty level\n");
        fclose(handle);
        return;
    }

    u8 sig[] = { 'A', 'M', 'T', 'L' };
    fwrite(sig, 4, 1, handle);

    u32 segment_dim[2] = { 2880, 1440 };
    fwrite(segment_dim, sizeof(segment_dim), 1, handle);


    fwrite(&total_box_count, sizeof(u32), 1, handle);
    fwrite(&total_entity_count, sizeof(u32), 1, handle);

    u32 running_total_entities = 0;
    u32 running_total_boxes = 0;

    for (u32 x = 0; x < 16; ++x) {
        for (u32 y = 0; y < 8; ++y) {
            Level_Segment segment = {};
            Edit_Segment *edit_seg = &edit->segments[x][y];

            if (!edit_seg->in_use) {
                segment.in_use = false;
            }
            else {
                segment.in_use = true;
                segment.texture_number = edit_seg->texture_index;

                segment.entity_count = edit_seg->entity_count;
                segment.box_count    = edit_seg->box_count;

                segment.entity_range_start = running_total_entities;
                segment.box_range_start    = running_total_boxes;

                running_total_entities += segment.entity_count;
                running_total_boxes += segment.box_count;

                segment.entity_range_one_past_last = running_total_entities;
                segment.box_range_one_past_last    = running_total_boxes;
            }

            fwrite(&segment, sizeof(Level_Segment), 1, handle);
        }
    }

    for (u32 x = 0; x < 16; ++x) {
        for (u32 y = 0; y < 8; ++y) {
            Edit_Segment *edit_seg = &edit->segments[x][y];

            if (edit_seg->in_use) {
                fwrite(edit_seg->entities, edit_seg->entity_count * sizeof(Entity), 1, handle);
            }

        }
    }

    for (u32 x = 0; x < 16; ++x) {
        for (u32 y = 0; y < 8; ++y) {
            Edit_Segment *edit_seg = &edit->segments[x][y];

            if (edit_seg->in_use) {
                fwrite(edit_seg->boxes, edit_seg->box_count * sizeof(Bounding_Box), 1, handle);
            }
        }
    }

    fclose(handle);
}
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
        LoadLevelFromFile(state, &playState->world, "Level.aml");
        Player *player = &playState->player[0];
        player->position = V2(640, 360);
        player->half_dim = V2(35, 50);
        player->velocity = V2(0, 250);
        player->health = 2;

        Asset *player_anim  = GetAsset(&state->assets, "Entity01");
        player->animation = CreateAnimationFromTexture(player_anim->texture, V2(0.16, 0.16), 2, 3, 0.09f);

        Asset *torch = GetAsset(&state->assets, "Entity02");
        sfTexture_setSmooth(torch->texture, true);
        playState->torch_animation = CreateAnimationFromTexture(torch->texture, V2(0.6, 0.6), 4, 4, 0.13f);

        Asset *candle_low = GetAsset(&state->assets, "CandleLow");
        Asset *candle_medium = GetAsset(&state->assets, "CandleMid");
        Asset *candle_high = GetAsset(&state->assets, "CandleHigh");

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

        // @Todo: Vary noise per light?
        const char *frag_code = R"FRAG(
            in vec2 frag_position;

            #define MAX_LIGHTS 16

            varying out vec4 final_colour;

            uniform sampler2D image;

            // Light Information
            uniform vec2 light_positions[MAX_LIGHTS];
            uniform vec3 light_colours[MAX_LIGHTS];
            uniform float light_distance_scales[MAX_LIGHTS];

            uniform sampler2D noise;
            uniform float time;

            void main() {
                vec3 ambient = vec3(0, 0, 0);
                for (int it = 0; it < MAX_LIGHTS; ++it) {
                    vec2 dir = light_positions[it] - frag_position;

                    vec2 noise_uv = vec2(abs(sin(0.1 * time)), abs(cos(0.1 * time)));
                    float dist = (light_distance_scales[it] - (0.01 * texture2D(noise, noise_uv))) * length(dir);

                    float attenuation = 1.0 / (1.0 + (0.09 * dist) + (0.032 * (dist * dist)));

                    ambient += attenuation * light_colours[it];
                }

                final_colour = vec4(ambient, 1) * texture2D(image, gl_TexCoord[0].xy);
            }
        )FRAG";

        playState->shader = sfShader_createFromMemory(vertex_code, 0, frag_code);
        Assert(playState->shader);
    }

    f32 dt = input->delta_time;

    v2 view_size = sfView_getSize(state->view);
    v2 screen_segment_dim = V2(2880, 1440);

    Game_Controller *controller = &input->controllers[0];
    Player *player = &playState->player[0];

    player->velocity.x = 0;
    if (IsPressed(controller->move_left)) {
        player->velocity.x      = -220;
        player->animation.flip  = true;
        player->animation.pause = false;
    }
    else if (IsPressed(controller->move_right)) {
        player->velocity.x = 220;
        player->animation.flip  = false;
        player->animation.pause = false;
    }

    player->jump_time -= dt;
    if (IsPressed(controller->jump)) {
        if (HasFlags(player->state_flags, EntityState_OnGround) &&
                player->fall_time < 0.3 && player->floor_time > 0.04f)
        {
            RemoveFlags(&player->state_flags, EntityState_OnGround);
            AddFlags(&player->state_flags, EntityState_Falling);

            player->velocity.y = -350;
            player->jump_time  = 0.3f;
            player->floor_time = 0;

            player->animation.pause = false;
        }
        else if (player->jump_time > 0) {
            player->velocity.y -= 830 * dt;
        }
    }

    player->velocity.x *= (HasFlags(player->state_flags, EntityState_OnGround) ? 1 : 0.85);
    player->velocity.y += (dt * 980);
    player->position += (dt * player->velocity);

    Asset *noise = GetAsset(&state->assets, "PNoise");

    v2 light_positions[3] = {
        player->position,
        V2(600, view_size.y - (145.5 / 2)),
        V2(100, screen_segment_dim.y - 250)
    };

    v3 light_colours[3] = {
        V3(0.5, 0.5, 1),
        V3(1, 0, 0),
        V3(0, 1, 0)
    };

    f32 light_distance_scales[3] = {
        0.08,
        0.10,
        0.04
    };

    playState->total_time += dt;
    sfShader_setTextureUniform(playState->shader, "noise", noise->texture);
    sfShader_setFloatUniform(playState->shader, "time", playState->total_time);
    sfShader_setFloatUniformArray(playState->shader, "light_distance_scales", light_distance_scales, 3);
    sfShader_setVec2UniformArray(playState->shader, "light_positions", light_positions, 3);
    sfShader_setVec3UniformArray(playState->shader, "light_colours", light_colours, 3);


    // @Speed: Getting the texture every time
    //


    sfCircleShape *origin = sfCircleShape_create();
    sfCircleShape_setOrigin(origin, V2(10, 10));
    sfCircleShape_setRadius(origin, 10);
    sfCircleShape_setPosition(origin, V2(0, 0));
    sfCircleShape_setFillColor(origin, sfMagenta);

    sfRenderWindow_drawCircleShape(state->renderer, origin, 0);

    sfCircleShape_destroy(origin);

    v2 new_view_pos = {};
    new_view_pos.x = player->position.x; //Max(player->position.x, -800);
    new_view_pos.y = player->position.y; //360;

    sfView_setCenter(state->view, new_view_pos);
    sfRenderWindow_setView(state->renderer, state->view);

    sfShader_bind(playState->shader);

    Asset *bg = GetAsset(&state->assets, "Location00");
    sfRectangleShape *back_rect = sfRectangleShape_create();
    sfRectangleShape_setSize(back_rect, screen_segment_dim);

//    sfRenderWindow_drawRectangleShape(state->renderer, back_rect, 0);

    sfRectangleShape_setTexture(back_rect, bg->texture, true);

    sfRenderWindow_drawRectangleShape(state->renderer, back_rect, 0);

    sfRectangleShape_destroy(back_rect);


    UpdateRenderAnimation(state, &playState->torch_animation, V2(1500, 1280), dt);
    UpdateRenderAnimation(state, &player->animation, player->position, dt);

    u32 candle = Max(Min(player->health, 2), 0);
    UpdateRenderAnimation(state, &playState->candle[candle], player->position + V2(40, -20), dt);

    sfCircleShape *li = sfCircleShape_create();
    sfCircleShape_setRadius(li, 5);
    sfCircleShape_setOrigin(li, V2(5, 5));
    for (u32 it = 0; it < 3; ++it) {
        sfCircleShape_setPosition(li, light_positions[it]);

        sfColor c = {
            cast(u8) (255 * light_colours[it].x),
            cast(u8) (255 * light_colours[it].y),
            cast(u8) (255 * light_colours[it].z),
            255
        };

        sfCircleShape_setFillColor(li, c);

        sfRenderWindow_drawCircleShape(state->renderer, li, 0);
    }

    sfCircleShape_destroy(li);

    Bounding_Box player_box;
    player_box.centre   = player->position;
    player_box.half_dim = player->half_dim;

    sfColor c = sfRed;

    sfRectangleShape *r = sfRectangleShape_create();
    sfRectangleShape_setFillColor(r, sfTransparent);
    sfRectangleShape_setOutlineColor(r, c);
    sfRectangleShape_setOutlineThickness(r, 2);

    World *world = &playState->world;

    for (u32 it = 0; it < world->entity_count; ++it) {
        Entity *e = &world->entities[it];

        if (e->type == EntityType_Rocks) {
            Asset *texture = GetAsset(&state->assets, "Entity00");

            sfSprite *sprite = sfSprite_create();

            sfVector2u sizeu = sfTexture_getSize(texture->texture);
            v2 size = V2(sizeu.x, sizeu.y);

            sfSprite_setTexture(sprite, texture->texture, false);
            sfSprite_setOrigin(sprite, 0.5f * size);
            sfSprite_setScale(sprite, e->scale);
            sfSprite_setPosition(sprite, e->position);

            sfRenderWindow_drawSprite(state->renderer, sprite, 0);
            sfSprite_destroy(sprite);
        }
        else {
        }
    }

    sfShader_bind(0);

    for (u32 it = 0; it < world->box_count; ++it) {
        Bounding_Box *box = &world->boxes[it];

        sfRectangleShape_setOrigin(r, box->half_dim);
        sfRectangleShape_setSize(r, 2 * box->half_dim);
        sfRectangleShape_setPosition(r, box->centre);

        sfRenderWindow_drawRectangleShape(state->renderer, r, 0);

        if (Overlaps(&player_box, box)) {
            v2 full_size = player->half_dim + box->half_dim;

            u32 axis = GetSmallestAxis(&player_box, box);
            switch (axis) {
                case 0: {
                    if (HasFlags(box->direction_flags, Direction_Left)) {
                        player->position.x = box->centre.x - full_size.x;
                    }
                }
                break;
                case 1: {
                    if (HasFlags(box->direction_flags, Direction_Right)) {
                        player->position.x = box->centre.x + full_size.x;
                    }
                }
                break;
                case 2: {
                    if (HasFlags(box->direction_flags, Direction_Top))
                    {
                        player->position.y = box->centre.y - full_size.y;
                        player->velocity.y = 0;

                        AddFlags(&player->state_flags, EntityState_OnGround);
                        RemoveFlags(&player->state_flags, EntityState_Falling);

                        player->floor_time += dt;
                        player->fall_time   = 0;
                    }
                }
                break;
                case 3: {
                    if (HasFlags(box->direction_flags, Direction_Bottom)) {
                        player->position.y = box->centre.y + full_size.y;
                        player->velocity.y = -0.45f * player->velocity.y;
                        player->jump_time = 0;
                    }
                }
                break;
            }
        }
    }

    if (HasFlags(player->state_flags, EntityState_Falling)) {
        player->fall_time += dt;
    }
    else {
        player->animation.pause= true;
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

    sfRenderWindow_drawRectangleShape(state->renderer, bbox, 0);

    sfRectangleShape_destroy(bbox);
}

internal b32 Contains(Bounding_Box *box, v2 point) {
    b32 result =
        (point.x >= (box->centre.x - box->half_dim.x)) &&
        (point.x <= (box->centre.x + box->half_dim.x)) &&
        (point.y >= (box->centre.y - box->half_dim.y)) &&
        (point.y <= (box->centre.y + box->half_dim.y));

    return result;
}

internal void ConvertToEditor(World *world, Edit_State *edit) {
    for (u32 it = 0; it < 128; ++it) {
        Edit_Segment *seg = &edit->segments[it / 8][it % 8];
        Level_Segment *world_seg = &world->segments[it];

        if (!world_seg->in_use) { continue; }

        seg->texture_index = world_seg->texture_number;
        seg->entity_count = world_seg->entity_count;
        seg->box_count    = world_seg->box_count;

        CopySize(seg->entities, &world->entities[world_seg->entity_range_start],
                seg->entity_count * sizeof(Entity));

        CopySize(seg->boxes, &world->boxes[world_seg->box_range_start], seg->box_count * sizeof(Bounding_Box));

        printf("Seg has %d, %d\n", seg->entity_count, seg->box_count);
    }

    edit->current_segment = &edit->segments[0][6];
}

internal void UpdateRenderEdit(Game_State *state, Edit_State *edit, Game_Input *input) {
    v2 segment_dim = V2(2880, 1440);

    if (!edit->initialised) {
        edit->mode = EditMode_Segment;

        Edit_Segment *segment = &edit->segments[0][0];
        segment->grid_x = 0;
        segment->grid_y = 0;
        segment->texture_index = 0;

        segment->entity_count = 0;
        segment->box_count = 0;

        edit->current_segment = segment;

        edit->zoom_factor = 1;
        edit->last_mouse = V2(0, 0);
        edit->camera_pos = V2(0, 0);
        edit->edit_view = sfView_create();
        sfView_setCenter(edit->edit_view, edit->camera_pos);
        sfView_setSize(edit->edit_view, V2(1280, 720));

        edit->segments[0][6].in_use = true;

        edit->entity_scales[0] = V2(0.6, 0.6);
        edit->entity_scales[1] = V2(0.16, 0.16);
        edit->entity_scales[2] = V2(0.6, 0.6);

        edit->displacement = {};

        edit->animations[1] = CreateAnimationFromTexture(GetAsset(&state->assets, "Entity01")->texture,
                V2(0.16, 0.16), 2, 3, 0.09f);

        edit->animations[2] = CreateAnimationFromTexture(GetAsset(&state->assets, "Entity02")->texture,
                V2(0.6, 0.6), 4, 4, 0.13f);

        edit->initialised = true;
    }

    if (JustPressed(input->f[6])) { WriteLevelToFile(state, edit); }
    if (JustPressed(input->f[7])) {
        World world = {};
        LoadLevelFromFile(state, &world, "Level.aml");
        ConvertToEditor(&world, edit);
    }

    // @Note: Camera controls
    if (IsPressed(input->mouse_buttons[2])) {
        v2 displacement = edit->zoom_factor * (edit->last_mouse - input->screen_mouse);
        sfView_move(edit->edit_view, displacement);
    }

    if (input->mouse_wheel_delta > 0) {
        edit->zoom_factor *= (1.0f / 1.3f);
        sfView_zoom(edit->edit_view, 1.0f / 1.3f);
    }
    else if (input->mouse_wheel_delta < 0) {
        edit->zoom_factor *= 1.3f;
        sfView_zoom(edit->edit_view, 1.3f);
    }

    edit->last_mouse = input->screen_mouse;
    sfRenderWindow_setView(state->renderer, edit->edit_view);


    if (JustPressed(input->f[1])) {
        edit->mode = EditMode_BoundingBox;
    }
    else if (JustPressed(input->f[2])) {
        edit->mode = EditMode_Entity;
    }
    else if (JustPressed(input->f[3])) {
        edit->mode = EditMode_Segment;
    }

    s32 grid_x = cast(s32) (input->mouse_position.x / segment_dim.x);
    s32 grid_y = cast(u32) (input->mouse_position.y / segment_dim.y);

    switch (edit->mode) {
        case EditMode_BoundingBox: {
            if (JustPressed(input->mouse_buttons[0])) {
                edit->first_mouse_down = input->mouse_position;
                edit->is_editing = true;
            }

            if (edit->is_editing && WasPressed(input->mouse_buttons[0])) {
                Assert(edit->is_editing);

                edit->is_editing = false;

                v2 diff = 0.5f * (input->mouse_position - edit->first_mouse_down);
                diff.x = Abs(diff.x);
                diff.y = Abs(diff.y);

                v2 min = V2(Min(input->mouse_position.x, edit->first_mouse_down.x),
                            Min(input->mouse_position.y, edit->first_mouse_down.y));

                Bounding_Box *next = &edit->current_segment->boxes[edit->current_segment->box_count];
                edit->current_segment->box_count += 1;

                next->direction_flags = Direction_Top; // (Direction_All & ~Direction_Bottom);
                next->centre   = min + diff;
                next->half_dim = diff;
            }

            if (JustPressed(input->mouse_buttons[1])) {
                Edit_Segment *current = edit->current_segment;
                for (u32 it = 0; it < current->box_count; ++it) {
                    if (Contains(&current->boxes[it], input->mouse_position)) {
                        current->box_count -= 1;
                        Swap(current->boxes[it], current->boxes[current->box_count]);
                    }
                }
            }
        }
        break;
        case EditMode_Entity: {
            if (JustPressed(input->debug_next)) {
                edit->entity_type += 1;
                if (edit->entity_type >= EntityType_Count) { edit->entity_type = 0; }
            }
            else if (JustPressed(input->debug_prev)) {
                edit->entity_type -= 1;
                if (edit->entity_type < 0) { edit->entity_type = EntityType_Count - 1; }
            }

            if (JustPressed(input->mouse_buttons[0])) {
                Entity *next = &edit->current_segment->entities[edit->current_segment->entity_count];
                edit->current_segment->entity_count += 1;

                next->type = edit->entity_type;
                next->position = input->mouse_position;
                next->scale = edit->entity_scales[next->type];
            }
        }
        break;
        case EditMode_Segment: {
            if (JustPressed(input->mouse_buttons[0])) {
                if (grid_x >= 0 && grid_x < 16) {
                    if (grid_y >= 0 && grid_y < 8) {
                        edit->current_segment = &edit->segments[grid_x][grid_y];
                        edit->current_segment->in_use = true;

                        sfFloatRect rect = {};
                        rect.top  = 0;
                        rect.left = 0;
                        rect.width = 1280;
                        rect.height = 720;
                        sfView_reset(edit->edit_view, rect);

                        v2 centre = V2(grid_x * segment_dim.x, grid_y * segment_dim.y) + (0.5f * segment_dim);
                        sfView_setCenter(edit->edit_view, centre);
                        sfView_zoom(edit->edit_view, 2.5);

                        sfRenderWindow_setView(state->renderer, edit->edit_view);

                        edit->zoom_factor = 2.5f;
                        edit->camera_pos = V2(0, 0);
                    }
                }
            }

            if (JustPressed(input->debug_next)) {
                edit->current_segment->texture_index += 1;
                if (edit->current_segment->texture_index >= 4) {
                    edit->current_segment->texture_index = 0;
                }
            }
            else if (JustPressed(input->debug_prev)) {
                edit->current_segment->texture_index -= 1;
                if (edit->current_segment->texture_index < 0) {
                    edit->current_segment->texture_index = 3;
                }
            }

            if (IsPressed(input->debug_up)) {
                edit->displacement.y -= 2;
            }

            if (IsPressed(input->debug_down)) {
                edit->displacement.y += 2;
            }
        }
        break;
    }

    v2 offset = 0.05 * segment_dim;
    sfRectangleShape *rect = sfRectangleShape_create();
    for (s32 x = 0; x < 16; ++x) {
        for (s32 y = 0; y < 8; ++y) {
            Edit_Segment *segment = &edit->segments[x][y];
            if (segment->in_use) {
                char name[256];
                snprintf(name, sizeof(name), "Location%02d", segment->texture_index);
                Asset *texture = GetAsset(&state->assets, name);

                sfRectangleShape_setOrigin(rect, V2(0, 0));
                sfRectangleShape_setTexture(rect, texture->texture, false);
                sfRectangleShape_setSize(rect, segment_dim);

                v2 pos = V2(x * segment_dim.x, y * segment_dim.y);
                sfRectangleShape_setPosition(rect, pos);
            }
            else {
                sfRectangleShape_setOrigin(rect, 0.5f * offset);
                sfRectangleShape_setTexture(rect, 0, false);
                sfRectangleShape_setSize(rect, 0.95 * segment_dim);
                sfRectangleShape_setPosition(rect, V2(x * segment_dim.x + offset.x, y * segment_dim.y + offset.y));
            }

            if ((x == grid_x) && (y == grid_y)) {
                if (edit->mode != EditMode_Segment || (segment == edit->current_segment)) {}
                else {
                    sfRectangleShape_setFillColor(rect, sfYellow);
                }
            }
            else {
                sfRectangleShape_setFillColor(rect, sfWhite);
            }

            sfRenderWindow_drawRectangleShape(state->renderer, rect, 0);
        }
    }

    sfRectangleShape_destroy(rect);

    Edit_Segment *segment = edit->current_segment;
    sfRectangleShape *shape = sfRectangleShape_create();


    sfSprite *sprite = sfSprite_create();
    for (u32 it = 0; it < segment->entity_count; ++it) {
        Entity *entity = &segment->entities[it];

        char name[256];
        snprintf(name, sizeof(name), "Entity%02d", entity->type);
        Asset *texture = GetAsset(&state->assets, name);
        Assert(texture);

        if (texture->flags & AssetFlag_Animation) {
            UpdateRenderAnimation(state, &edit->animations[entity->type],
                    entity->position, input->delta_time);
        }
        else {
            sfSprite *sprite = sfSprite_create();

            sfVector2u sizeu = sfTexture_getSize(texture->texture);
            v2 size = V2(sizeu.x, sizeu.y);

            sfSprite_setTexture(sprite, texture->texture, false);
            sfSprite_setOrigin(sprite, 0.5f * size);
            sfSprite_setScale(sprite, entity->scale);
            sfSprite_setPosition(sprite, entity->position);

            sfRenderWindow_drawSprite(state->renderer, sprite, 0);
        }
    }

    sfSprite_destroy(sprite);

    sfRectangleShape_setFillColor(shape, sfTransparent);
    sfRectangleShape_setOutlineColor(shape, sfRed);
    sfRectangleShape_setOutlineThickness(shape, 1.5);

    for (u32 it = 0; it < segment->box_count; ++it) {
        Bounding_Box *box = &segment->boxes[it];

        sfRectangleShape_setOrigin(shape, box->half_dim);
        sfRectangleShape_setPosition(shape, box->centre);
        sfRectangleShape_setSize(shape, 2 * box->half_dim);

        sfRenderWindow_drawRectangleShape(state->renderer, shape, 0);
    }

    sfRectangleShape_destroy(shape);

    switch (edit->mode) {
        case EditMode_BoundingBox: {
            if (edit->is_editing) {
                if (JustPressed(input->mouse_buttons[1])) { edit->is_editing = false; }

                v2 min = V2(Min(input->mouse_position.x, edit->first_mouse_down.x),
                            Min(input->mouse_position.y, edit->first_mouse_down.y));

                v2 max = V2(Max(input->mouse_position.x, edit->first_mouse_down.x),
                            Max(input->mouse_position.y, edit->first_mouse_down.y));

                v2 half_dim = (max - min);

                sfRectangleShape *r = sfRectangleShape_create();
                sfRectangleShape_setPosition(r, min);
                sfRectangleShape_setSize(r, half_dim);
                sfRectangleShape_setFillColor(r, sfTransparent);
                sfRectangleShape_setOutlineColor(r, sfRed);
                sfRectangleShape_setOutlineThickness(r, 2);

                sfRenderWindow_drawRectangleShape(state->renderer, r, 0);

                sfRectangleShape_destroy(r);
            }
        }
        break;
        case EditMode_Entity: {
            char name[256];
            snprintf(name, sizeof(name), "Entity%02d", edit->entity_type);
            Asset *texture = GetAsset(&state->assets, name);
            Assert(texture);

            if (texture->flags & AssetFlag_Animation) {
                UpdateRenderAnimation(state, &edit->animations[edit->entity_type],
                        input->mouse_position, input->delta_time);
            }
            else {
                sfSprite *sprite = sfSprite_create();

                sfVector2u sizeu = sfTexture_getSize(texture->texture);
                v2 size = V2(sizeu.x, sizeu.y);

                sfSprite_setTexture(sprite, texture->texture, false);
                sfSprite_setOrigin(sprite, 0.5f * size);
                sfSprite_setScale(sprite, edit->entity_scales[edit->entity_type]);
                sfSprite_setPosition(sprite, input->mouse_position);

                sfRenderWindow_drawSprite(state->renderer, sprite, 0);

                sfSprite_destroy(sprite);
            }
        }
        break;
    }
}

internal void LudumUpdateRender(Game_State *state, Game_Input *input) {
    if (!state->initialised) {
        CreateLevelState(state, LevelType_Play);

        state->player_pos = V2(250, 400);
        InitAssets(&state->assets, 64);

        LoadAsset(&state->assets, "Entity00", Asset_Texture);
        LoadAsset(&state->assets, "Entity01", Asset_Texture, AssetFlag_Animation);
        LoadAsset(&state->assets, "Entity02", Asset_Texture, AssetFlag_Animation);

        LoadAsset(&state->assets, "Location00", Asset_Texture);
        LoadAsset(&state->assets, "Location01", Asset_Texture);
        LoadAsset(&state->assets, "Location02", Asset_Texture);
        LoadAsset(&state->assets, "Location03", Asset_Texture);

        // @Temp: These are still in for backwards comp
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
        case LevelType_Edit: {
            Edit_State *edit = &current_state->edit;
            UpdateRenderEdit(state, edit, input);
        }
        break;
    }

}
