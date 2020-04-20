#include "Ludum_Assets.cpp"
#include "Ludum_Animation.cpp"
#include "Ludum_Editor.cpp"

#if 0
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

internal void UpdateRenderParticleSpawner(Game_State *state, ParticleSpawner *spawner, f32 delta_time) {
    u32 i;
    f32 particle_allowance = 0;
    for(i = 0; i < spawner->max_particles; i++) {
        Particle *p = &spawner->particles[i];
        if(!p->active && particle_allowance < spawner->rate * delta_time) {
            p->active = true;
            v2 minSpawnRange = spawner->centre - spawner->half_size;
            v2 maxSpawnRange = spawner->centre + spawner->half_size;
            p->position = random(minSpawnRange, maxSpawnRange);
            p->rotation = random(spawner->rotation_range.x, spawner->rotation_range.y);
            p->velocity = random(spawner->velocity_min, spawner->velocity_max);
            p->size = random(spawner->size_min, spawner->size_max);
            p->life_time = random(spawner->life_time_range.x, spawner->life_time_range.y);
            particle_allowance++;
        }
        p->life_time -= delta_time;
        if(p->life_time < 0 || p->position.y > 2000) {
            p->active = false;
            continue;
        }
        p->position += p->velocity * delta_time;
        if(spawner->strict_x) {
            if(p->position.x > spawner->centre.x + spawner->half_size.x) {
                p->position.x -= spawner->half_size.x*2;
            }
            else if(p->position.x < spawner->centre.x - spawner->half_size.x) {
                p->position.x += spawner->half_size.x*2;
            }
        }
        sfRectangleShape *rs = sfRectangleShape_create();
        sfRectangleShape_setTexture(rs, GetAsset(&state->assets, spawner->assetName)->texture, true);
        sfRectangleShape_setSize(rs, p->size);
        sfRectangleShape_setPosition(rs, p->position);
        sfRectangleShape_setRotation(rs, p->rotation);
        sfColor col = {
             255,
             255,
             255,
             spawner->transparency
        };
        sfRectangleShape_setFillColor(rs, col);
        sfRenderWindow_drawRectangleShape(state->renderer, rs, 0);
        sfRectangleShape_destroy(rs);
    }
    return;
}

internal Particle *InitialiseParticlePool(ParticleSpawner *spawner) {
    Particle *result = cast(Particle *) Alloc(sizeof(Particle) * spawner->max_particles);
    memset(result, 0, sizeof(Particle) * spawner->max_particles);

    spawner->particles = result;
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

            Animation *playerAnim = &player->animation;
            f32 fireball_offset = playerAnim->flip ? -40 : 40;
            fireball->position = V2(player->position.x + fireball_offset, player->position.y+5);
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

        ParticleSpawner *rainSpawner = &playState->rain[0];
        rainSpawner->centre = V2(340, 300);
        rainSpawner->half_size = V2(1440, 20);
        rainSpawner->velocity_min = V2(-200, 350);
        rainSpawner->velocity_max = V2(-180, 450);
        rainSpawner->size_min = V2(4, 6);
        rainSpawner->size_max = V2(4, 6);
        rainSpawner->rotation_range = V2(33, 30);
        rainSpawner->life_time_range = V2(40, 120);
        rainSpawner->max_particles = 800;
        rainSpawner->rate = 20;
        rainSpawner->assetName = "Rain";
        rainSpawner->strict_x = true;
        rainSpawner->transparency = 150;
        InitialiseParticlePool(rainSpawner);

        ParticleSpawner *windSpawner = &playState->wind[0];
        windSpawner->centre = V2(340, 300);
        windSpawner->half_size = V2(10, 10);
        windSpawner->velocity_min = V2(-10, -10);
        windSpawner->velocity_max = V2(10, 10);
        windSpawner->size_min = V2(3, 6);
        windSpawner->size_max = V2(4, 7);
        windSpawner->rotation_range = V2(0, 360);
        windSpawner->life_time_range = V2(0.8, 1);
        windSpawner->max_particles = 300;
        windSpawner->rate = 20;
        windSpawner->assetName = "Fireball";
        windSpawner->transparency = 255;
        InitialiseParticlePool(windSpawner);

    ParticleSpawner *rain = &playState->rain[0];
    rain->centre.x = player->position.x;
    UpdateRenderParticleSpawner(state, playState->rain, dt);
    ParticleSpawner *wind = &playState->wind[0];
    wind->centre = input->mouse_position;
    UpdateRenderParticleSpawner(state, playState->wind, dt);


#endif

//
// @LevelState: State code
//
internal Level_State *CreateLevelState(Game_State *state, Level_Type type) {
    Level_State *result = cast(Level_State *) Alloc(sizeof(Level_State));
    memset(result, 0, sizeof(Level_State));

    result->type = type;
    result->next = state->current_state;
    state->current_state = result;

    return result;
}

internal Level_State *RemoveLevelState(Game_State *state) {
    Level_State *result = state->current_state;
    Assert(result);
    state->current_state = result->next;

    return result;
}

//
//
//

internal u32 GetActiveLevelSegments(World *world, Level_Segment *output) {
    u32 result = 0;

    Entity *player = &world->entities[0];

    u32 grid_x = cast(u32) (player->position.x / world->segment_dim.x);
    u32 grid_y = cast(u32) (player->position.y / world->segment_dim.y);

    Assert(grid_x >= 0 && grid_x < 16);
    Assert(grid_y >= 0 && grid_y < 8);

    output[0] = world->segments[(8 * grid_x) + grid_y];
    result += 1;

    if (grid_x != 0) {
        output[result] = world->segments[(8 * (grid_x - 1)) + grid_y];
        if (output[result].in_use) { result += 1; }
    }

    if (grid_x != 15) {
        output[result] = world->segments[(8 * (grid_x + 1)) + grid_y];
        if (output[result].in_use) { result += 1; }
    }

    if (grid_y != 0) {
        output[result] = world->segments[(8 * grid_x) + (grid_y - 1)];
        if (output[result].in_use) { result += 1; }
    }

    if (grid_y != 7) {
        output[result] = world->segments[(8 * grid_x) + (grid_y + 1)];
        if (output[result].in_use) { result += 1; }
    }

    return result;
}

internal void UpdateRenderPlayer(Game_State *state, Play_State *play_state, Game_Input *input, Entity *player) {
    World *world = &play_state->world;

    f32 dt = input->delta_time;
    Game_Controller *controller = &input->controllers[0];

    player->velocity.x = 0;
    if (IsPressed(controller->move_left)) {
        player->velocity.x      = -220;
        player->animation.flip  = true;
        player->animation.pause = false;
    }
    else if (IsPressed(controller->move_right)) {
        player->velocity.x      = 220;
        player->animation.flip  = false;
        player->animation.pause = false;
    }

    player->jump_time -= dt;

    if (IsPressed(controller->jump)) {
        if (HasFlags(player->flags, EntityState_OnGround) &&
                player->fall_time < 0.3 && player->floor_time > 0.04f)
        {
            RemoveFlags(&player->flags, EntityState_OnGround);
            AddFlags(&player->flags, EntityState_Falling);

            player->velocity.y = -350;
            player->jump_time  = 0.3f;
            player->floor_time = 0;

            player->animation.pause = false;
        }
        else if (player->jump_time > 0) {
            player->velocity.y -= 830 * dt;
        }
    }

    player->velocity.x *= (HasFlags(player->flags, EntityState_OnGround) ? 1 : 0.85);
    player->velocity.y += (dt * 980);
    player->position += (dt * player->velocity);

    if (JustPressed(input->debug_prev)) {
        player->health -= 1;
        if (player->health > 2) { player->health = 2; }
    }

    if (JustPressed(input->debug_next)) {
        player->health += 1;
        if (player->health > 2) { player->health = 0; }
    }

    // @Todo: Better camera code
    v2 new_view_pos = {};
    new_view_pos.x = player->position.x; //Max(player->position.x, -800);
    new_view_pos.y = player->position.y; //360;

    sfView_setCenter(state->view, new_view_pos);
    sfRenderWindow_setView(state->renderer, state->view);

    UpdateRenderAnimation(state, &player->animation, player->position, dt);

    if (HasFlags(player->flags, EntityState_Falling)) { player->fall_time += dt; }
    else { player->animation.pause = true; }

    u32 candle = Max(Min(player->health, 2), 0);
    Animation *candle_animation = &play_state->candle[candle];
    v2 offset = V2(player->animation.flip ? -40 : 40, -20);

    UpdateRenderAnimation(state, candle_animation, player->position + offset, dt);

    Asset *noise = GetAsset(&state->assets, "PNoise");
    sfShader_setTextureUniform(state->lighting_shader, "noise", noise->texture);

    f32 light_distance_scale = ((3 - player->health) * 0.08);
    v3 colour = { 0.5, 0.5, 1.0 };
    v2 light_position = player->position + offset;

    sfShader_setFloatUniform(state->lighting_shader, "time", input->delta_time);
    sfShader_setFloatUniformArray(state->lighting_shader, "light_distance_scales", &light_distance_scale, 1);
    sfShader_setVec2UniformArray(state->lighting_shader, "light_positions", &light_position, 1);
    sfShader_setVec3UniformArray(state->lighting_shader, "light_colours", &colour, 1);

    Level_Segment active[5];
    u32 count = GetActiveLevelSegments(world, active); // @Duplicate: We did this before

    Bounding_Box player_box = CreateBox(player->position, player->half_dim);

    for (u32 it = 0; it < count; ++it) {
        Level_Segment *segment = &active[it];

        for (u32 range = segment->box_range_start;
                range < segment->box_range_one_past_last;
                ++range)
        {
            Bounding_Box *box = &world->boxes[range];
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

                            AddFlags(&player->flags, EntityState_OnGround);
                            RemoveFlags(&player->flags, EntityState_Falling);

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
    }

    // @Todo: Collision detection
    // - World bounding boxes for level terrain
    // - Other entities
}

internal void DrawLevelSegment(Game_State *state, Level_Segment *segment, v2 segment_dim) {
    sfRectangleShape *level = sfRectangleShape_create();

    Asset *clouds = GetAsset(&state->assets, "DarkClouds");

    sfRectangleShape_setPosition(level, V2(segment->grid[0] * segment_dim.x, segment->grid[1] * segment_dim.y));
    sfRectangleShape_setSize(level, segment_dim);
    sfRectangleShape_setTexture(level, clouds->texture, true);

    sfRenderWindow_drawRectangleShape(state->renderer, level, 0);

    char name[256];
    snprintf(name, sizeof(name), "Location%02d", segment->texture_number);
    Asset *location = GetAsset(&state->assets, name);
    Assert(location->type == Asset_Texture);

    sfRectangleShape_setPosition(level, V2(segment->grid[0] * segment_dim.x, segment->grid[1] * segment_dim.y));
    sfRectangleShape_setSize(level, segment_dim);
    sfRectangleShape_setTexture(level, location->texture, true);

    sfRenderWindow_drawRectangleShape(state->renderer, level, 0);
}

internal void DrawStaticEntity(Game_State *state, Entity *entity) {
    // All static entities preceed Player in the type enum
    Assert(entity->type < EntityType_Player);

    char name[256];
    snprintf(name, sizeof(name), "Entity%02d", entity->type);
    Asset *texture = GetAsset(&state->assets, name);
    Assert(texture->type == Asset_Texture);

    sfSprite *sprite = sfSprite_create();

    //sfSprite_setOrigin(sprite, entity->half_dim);
    sfSprite_setPosition(sprite, entity->position - ( entity->half_dim));
    sfSprite_setScale(sprite, entity->scale);
    sfSprite_setTexture(sprite, texture->texture, true);

    sfRenderWindow_drawSprite(state->renderer, sprite, 0);
}

internal void UpdateRenderPlayState(Game_State *state, Play_State *playState, Game_Input *input) {
    World *world = &playState->world;

    if(!playState->initialised) {
        LoadLevelFromFile(state, world, "Level.aml");

        Entity *player = &world->entities[0]; // @Note: Slot 0 is reserved for the player
        player->velocity = V2(0, 0);
        Assert(player->type == EntityType_Player);

        player->animation = CreatePlayerAnimation(&state->assets);
        player->health    = 2;

        player->half_dim = V2(35, 50); // @Hack: Should be set by file

        for (u32 it = 1; it < world->entity_count; ++it) {
            Entity *entity = &world->entities[it];
            AddFlags(&entity->flags, EntityState_Active);

            switch (entity->type) {
                case EntityType_Torch: {
                    entity->animation = CreateTorchAnimation(&state->assets);
                }
                break;
                case EntityType_Wind: {
                    entity->animation = CreateWindAnimation(&state->assets);
                }
                break;
            }
        }

        // @Todo: Not sure what to do with this for now
        Asset *candle_low    = GetAsset(&state->assets, "CandleLow");
        Asset *candle_medium = GetAsset(&state->assets, "CandleMid");
        Asset *candle_high   = GetAsset(&state->assets, "CandleHigh");

        playState->candle[0] = CreateAnimationFromTexture(candle_low->texture,    V2(0.16, 0.16), 1, 3, 0.08f);
        playState->candle[1] = CreateAnimationFromTexture(candle_medium->texture, V2(0.16, 0.16), 1, 3, 0.08f);
        playState->candle[2] = CreateAnimationFromTexture(candle_high->texture,   V2(0.16, 0.16), 1, 3, 0.08f);

        playState->initialised = true;
    }

    f32 dt = input->delta_time;

    v2 view_size          = sfView_getSize(state->view);
    v2 screen_segment_dim = V2(2880, 1440); // @Todo: This will come from the world

    Entity *player = &world->entities[0];

    Level_Segment active[5]; // @Note: The segment the player is currently in and the 4 adjacent segments
    u32 count = GetActiveLevelSegments(world, active);

    sfShader_bind(state->lighting_shader);

    // Prepass to draw all of the segments to make sure they don't overlap entity parts
    for (u32 it = 0; it < count; ++it) {
        Level_Segment *segment = &active[it];
        DrawLevelSegment(state, segment, screen_segment_dim);
    }

    for (u32 it = 0; it < count; ++it) {
        Level_Segment *segment = &active[it];

        for (u32 range = segment->entity_range_start;
                range < segment->entity_range_one_past_last;
                ++range)
        {
            Entity *entity = &world->entities[range];
            if (!HasFlags(entity->flags, EntityState_Active)) { continue; } // Don't update or render inactive

            switch (entity->type) {
                // case EntityType_Barrel: etc.
                case EntityType_Painting:
                case EntityType_Rocks: {
                    DrawStaticEntity(state, entity);
                }
                break;
                case EntityType_Fireball: {
                    // @Todo: Update direction, size etc.
                }
                break;
                case EntityType_Torch: {
                    // @Todo: Check if the player is within range and activate
                    UpdateRenderAnimation(state, &entity->animation, entity->position, dt);
                }
                break;
                case EntityType_Raghead: {
                    // @Todo: Vary position based on pathing
                    //


                    v2 dir = entity->path_points[entity->next_point] - entity->position;
                    f32 len = Length(dir);
                    if (len < 0.5) {
                        if (HasFlags(entity->flags, EntityState_ReversedPathing)) {
                            entity->next_point -= 1;

                            if (entity->next_point == 0) {
                                RemoveFlags(&entity->flags, EntityState_ReversedPathing);
                            }
                        }
                        else {
                            entity->next_point += 1;
                            if (entity->next_point >= entity->path_count) {
                                entity->next_point -= 1;
                                AddFlags(&entity->flags, EntityState_ReversedPathing);
                            }
                        }

                    }
                    else {
                        dir *= (1.0f / len);
                        entity->velocity = 80 * dir;
                        entity->position += (dt * entity->velocity);
                    }

                    DrawStaticEntity(state, entity);
                }
                break;
                case EntityType_Wind: {
                    // @Todo: Pathing
                    // @Todo: Make inactive if reached end of path

                    v2 dir = entity->path_points[entity->next_point] - entity->position;
                    f32 len = Length(dir);
                    if (len < 0.5) {
                        entity->next_point += 1;
                        if (entity->next_point >= entity->path_count) { entity->next_point = 0; }
                    }
                    else {
                        dir *= (1.0f / len);
                        entity->velocity = 80 * dir;
                        entity->position += (dt * entity->velocity);
                    }

                    UpdateRenderAnimation(state, &entity->animation, entity->position, dt);
                }
                break;
            }
        }

#if 0
        sfRectangleShape *bbox = sfRectangleShape_create();
        sfRectangleShape_setFillColor(bbox, sfTransparent);
        sfRectangleShape_setOutlineColor(bbox, sfRed);
        sfRectangleShape_setOutlineThickness(bbox, 1.5);
        for (u32 range = segment->box_range_start;
                range < segment->box_range_one_past_last;
                ++range)
        {
            Bounding_Box *box = &world->boxes[it];
            sfRectangleShape_setOrigin(bbox, box->half_dim);
            sfRectangleShape_setPosition(bbox, box->centre);
            sfRectangleShape_setSize(bbox, 2 * box->half_dim);

            sfRenderWindow_drawRectangleShape(state->renderer, bbox, 0);
        }

        sfRectangleShape_destroy(bbox);
#endif

    }

    UpdateRenderPlayer(state, playState, input, player);

    sfShader_bind(0);

    // @Todo: Reenable lighting
    // Asset *noise = GetAsset(&state->assets, "PNoise");
    // sfShader_bind(playState->shader);
    // sfShader_bind(0);

    // playState->total_time += dt;
    // sfShader_setTextureUniform(playState->shader, "noise", noise->texture);
    // sfShader_setFloatUniform(playState->shader, "time", playState->total_time);
    // sfShader_setFloatUniformArray(playState->shader, "light_distance_scales", light_distance_scales, 3);
    // sfShader_setVec2UniformArray(playState->shader, "light_positions", light_positions, 3);
    // sfShader_setVec3UniformArray(playState->shader, "light_colours", light_colours, 3);


    // @Todo: Reenable candle.. It should probably be its own entity
    // u32 candle = Max(Min(player->health, 2), 0);
    // Animation *playerAnim = &player->animation;
    // f32 candleAdd = playerAnim->flip ? -40 : 40;
    // UpdateRenderAnimation(state, &playState->candle[candle], player->position + V2(candleAdd, -20), dt);

#if 0
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
    sfRectangleShape_destroy(r);
#endif

    // @Todo: Reenable UpdateRenderFireBalls(state, playState, input);
    //
    // if(JustPressed(controller->interact) && !player->holding_fireball && !player->fireball_break){
    //    player->holding_fireball = true;
    //    AddFireBall(playState);
    //    player->health--;
    // }
    //else {
    //    player->fireball_break = false;
    //}

}

internal void UpdateRenderLogoState(Game_State *state, Logo_State *logo, Game_Input *input) {
    sfRenderWindow_clear(state->renderer, sfBlack);
	if(!logo->initialised) {
		logo->delta_rate = 1.0;
		logo->rate = 0;
		logo->opacity = 0;
		logo->initialised = true;
	}

	logo->rate += logo->delta_rate;
	logo->opacity = Clamp(logo->opacity + input->delta_time * 2.5 * logo->rate, -0.1f, 255.0);

    v2 view_size = sfView_getSize(state->view);
	sfRectangleShape *logo_rect = sfRectangleShape_create();
	sfRectangleShape_setPosition(logo_rect, V2((view_size.x-view_size.y)/2, 0));
	sfRectangleShape_setTexture(logo_rect, GetAsset(&state->assets,"logo")->texture, false);
	sfRectangleShape_setSize(logo_rect, V2(view_size.y, view_size.y));
    sfColor logoColour = {
        255,
        255,
        255,
        cast(u8) logo->opacity
    };
	sfRectangleShape_setFillColor(logo_rect, logoColour);
	sfRenderWindow_drawRectangleShape(state->renderer, logo_rect, NULL);
	sfRectangleShape_destroy(logo_rect);
	if(logo->opacity < 0 || IsPressed(input->controllers[0].jump)) {
		free(RemoveLevelState(state));
	}
	else if (logo->rate > 75) {
        logo->delta_rate = -logo->delta_rate;
    }
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

internal void LudumUpdateRender(Game_State *state, Game_Input *input) {
    if (!state->initialised) {
        //CreateLevelState(state, LevelType_Edit);
        CreateLevelState(state, LevelType_Play);
        // TODO RELEASE: Enable logo
        //CreateLevelState(state, LevelType_Logo);

        state->player_pos = V2(250, 400);
        InitAssets(&state->assets, 64);

        LoadAsset(&state->assets, "Entity00", Asset_Texture);
        LoadAsset(&state->assets, "Entity01", Asset_Texture);
        LoadAsset(&state->assets, "Entity02", Asset_Texture);
        LoadAsset(&state->assets, "Entity03", Asset_Texture);
        LoadAsset(&state->assets, "Entity04", Asset_Texture, AssetFlag_Animation);
        LoadAsset(&state->assets, "Entity05", Asset_Texture, AssetFlag_Animation);
        LoadAsset(&state->assets, "Entity06", Asset_Texture, AssetFlag_Animation);

        LoadAsset(&state->assets, "Location00", Asset_Texture);
        LoadAsset(&state->assets, "Location01", Asset_Texture);
        LoadAsset(&state->assets, "Location02", Asset_Texture);
        LoadAsset(&state->assets, "Location03", Asset_Texture);

        LoadAsset(&state->assets, "DarkClouds", Asset_Texture);

        LoadAsset(&state->assets, "CandleLow", Asset_Texture);
        LoadAsset(&state->assets, "CandleMid", Asset_Texture);
        LoadAsset(&state->assets, "CandleHigh", Asset_Texture);

        LoadAsset(&state->assets, "PNoise", Asset_Texture);

        LoadAsset(&state->assets, "Rain", Asset_Texture);
		LoadAsset(&state->assets, "logo", Asset_Texture);

        const char *vertex_code = R"VERT(
            varying out vec2 frag_position;

            void main() {
                gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
                gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

                frag_position = vec2(gl_ModelViewMatrix * gl_Vertex); // vec2(gl_Position.x, gl_Vertex.y);
            }
        )VERT";

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

        state->lighting_shader = sfShader_createFromMemory(vertex_code, 0, frag_code);
        Assert(state->lighting_shader);

        state->initialised = true;
    }

    Level_State *current_state = state->current_state;
    switch(current_state->type) {
        case LevelType_Play: {
            Play_State *play = &current_state->play;
            UpdateRenderPlayState(state, play, input);
        }
        break;
        case LevelType_Logo: {
            Logo_State *logo = &current_state->logo;
            UpdateRenderLogoState(state, logo, input);
        }
        break;
        case LevelType_Edit: {
            Edit_State *edit = &current_state->edit;
            UpdateRenderEdit(state, edit, input);
        }
        break;
    }

}
