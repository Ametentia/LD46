#include "Ludum_Assets.cpp"
#include "Ludum_Animation.cpp"

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

#include "Ludum_Editor.cpp"

internal sfSound *GetSound(Play_State *state, sfSoundBuffer *wanted_buffer) {
    for(u32 i = 0; i < 16; i++) {
        sfSound *sound = state->sounds[i];
        if(!sound) {
            state->sounds[i] = sfSound_create();
            sound = state->sounds[i];
        }
        const sfSoundBuffer *buffer = sfSound_getBuffer(sound);
        if(wanted_buffer == buffer) {
            return sound;
        }
    }
    for(u32 i = 0; i < 16; i++) {
        sfSound *sound = state->sounds[i];
        if(!sound) {
            sound = sfSound_create();
        }
        if(sfSound_getStatus(sound) != sfPlaying) {
            sfSound_setBuffer(sound, wanted_buffer);
            return sound;
        }
    }
    return 0;
}

// Light stuff
//
//

internal void AddLight(Play_State *state, v2 pos, f32 scale, v3 colour) {
    u32 index = state->light_count;

    if (index >= MAX_LIGHTS) {
        printf("WARNING: Too many lights have been submitted\n");
        return;
    }

    state->light_count += 1;

    state->light_positions[index] = pos;
    state->light_scales[index]    = scale;
    state->light_colours[index]   = colour;
}

internal void UploadLightInformation(Game_State *state, Play_State *play_state) {
    Asset *noise = GetAsset(&state->assets, "PNoise");
    sfShader_setTextureUniform(state->diffuse_shader, "noise", noise->texture);

    u32 count = play_state->light_count;

    sfShader_setIntUniform(state->diffuse_shader, "light_count", count);
    sfShader_setFloatUniform(state->diffuse_shader, "time", play_state->total_time);
    sfShader_setFloatUniformArray(state->diffuse_shader, "light_distance_scales", play_state->light_scales, count);
    sfShader_setVec2UniformArray(state->diffuse_shader, "light_positions", play_state->light_positions, count);
    sfShader_setVec3UniformArray(state->diffuse_shader, "light_colours", play_state->light_colours, count);

    play_state->light_count = 0;
}

//
//
//

internal void MusicHandlerUpdate(Game_State *state, MusicLayers *layers, f32 time_per_frame) {
    if(!layers->initialised) {
        layers->initialised = true;
        layers->play_time = 0;
        Asset *organ = GetAsset(&state->assets, "organ");
        // TODO: Use a settings volume
        sfMusic_setVolume(organ->music, 0);
        sfMusic_play(organ->music);
        sfMusic_setLoop(organ->music, true);
    }
    layers->play_time += time_per_frame;
    Music_State *tracks[4] = {
        &layers->swell,
        &layers->arpeggio,
        &layers->drums,
        &layers->hat
    };
    const char *assetNames[4] {
        "swell",
        "arpeggio",
        "drums",
        "hat"
    };
    for(u32 i = 0; i < 4; i++) {
        if(*tracks[i] == Music_Request) {
            Asset *organ = GetAsset(&state->assets, "organ");
            Assert(organ);
            f32 organTime = sfTime_asSeconds(sfMusic_getPlayingOffset(organ->music));
            Asset *asset = GetAsset(&state->assets, assetNames[i]);
            Assert(asset);
            sfSoundStatus soundState = sfMusic_getStatus(asset->music);
            if(soundState == sfStopped && (u32)floor(organTime)%16 == 0) {
                sfMusic_play(asset->music);
                sfMusic_setVolume(asset->music, 0);
                sfMusic_setLoop(asset->music, true);
                *tracks[i] = Music_Playing;
            } else if(soundState == sfPlaying && (u32)floor(organTime)%16 == 0) {
                sfMusic_setPlayingOffset(asset->music, sfTime_Zero);
                sfMusic_stop(asset->music);
                *tracks[i] = Music_Stopped;
            }
        }
    }
    if(layers->play_time > 32) {
        layers->play_time = 0;
    }
}

internal Entity *GetNextScratchEntity(World *world) {
    Entity *result = &world->scratch_entities[world->next_scratch_entity];

    world->next_scratch_entity += 1;
    if (world->next_scratch_entity >= ArrayCount(world->scratch_entities)) {
        world->next_scratch_entity = 0;
    }

    return result;
}

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
        if (HasFlags(output[result].flags, LevelSegment_InUse)) { result += 1; }
    }

    if (grid_x != 15) {
        output[result] = world->segments[(8 * (grid_x + 1)) + grid_y];
        if (HasFlags(output[result].flags, LevelSegment_InUse)) { result += 1; }
    }

    if (grid_y != 0) {
        output[result] = world->segments[(8 * grid_x) + (grid_y - 1)];
        if (HasFlags(output[result].flags, LevelSegment_InUse)) { result += 1; }
    }

    if (grid_y != 7) {
        output[result] = world->segments[(8 * grid_x) + (grid_y + 1)];
        if (HasFlags(output[result].flags, LevelSegment_InUse)) { result += 1; }
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

            player->velocity.y = -450;
            player->jump_time  = 0.3f;
            player->floor_time = 0;

            player->animation.pause = false;
        }
        else if (player->jump_time > 0) {
            player->velocity.y -= 1230 * dt;
        }
    }

    if (JustPressed(input->mouse_buttons[0]) && player->health > 1) {
        player->health -= 1;

        Entity *entity = GetNextScratchEntity(&play_state->world);

        v2 direction = Normalise(input->mouse_position - player->position);

        entity->type = EntityType_Fireball;
        entity->scale = V2(1, 1);
        entity->position = player->position + 10 * direction;
        entity->velocity = 100 * direction;

        AddFlags(&entity->flags, EntityState_Active | EntityState_Lit);
    }

    player->velocity.x *= (HasFlags(player->flags, EntityState_OnGround) ? 1 : 0.85);
    player->velocity.y += (dt * 1300);
    player->position += (dt * player->velocity);

    if (JustPressed(input->debug_prev)) {
        player->health -= 1;
        if (player->health > 3) { player->health = 3; }
    }

    if (JustPressed(input->debug_next)) {
        player->health += 1;
        if (player->health > 3) { player->health = 0; }
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

    u32 candle = player->health;
    if (candle != 0) {
        Animation *candle_animation = &play_state->candle[candle - 1];
        v2 offset = V2(player->animation.flip ? -40 : 40, -20);

        UpdateRenderAnimation(state, candle_animation, player->position + offset, dt);

        v3 colour = { 0.5, 0.5, 1.0 };
        v2 light_position = player->position + offset;
        AddLight(play_state, light_position, (3 - (player->health - 1)) * 0.06, colour);
    }
    else {
        sfSprite *sprite = sfSprite_create();

        Asset *asset = GetAsset(&state->assets, "CandleDead");
        v2 offset = V2(player->animation.flip ? -40 : 40, -20);

        sfSprite_setPosition(sprite, player->position + offset);
        sfSprite_setScale(sprite, V2(0.16, 0.16));
        sfSprite_setTexture(sprite, asset->texture, true);

        sfRenderWindow_drawSprite(state->renderer, sprite, 0);

        sfSprite_destroy(sprite);
    }

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

    v2 scale = V2(1, 1);
    if (HasFlags(segment->flags, LevelSegment_FlippedX)) { scale.x = -1; }
    if (HasFlags(segment->flags, LevelSegment_FlippedY)) { scale.y = -1; }

    sfRectangleShape_setOrigin(level, 0.5f * segment_dim);
    sfRectangleShape_setPosition(level, V2(segment->grid[0] * segment_dim.x, segment->grid[1] * segment_dim.y) + 0.5f * segment_dim);
    sfRectangleShape_setSize(level, segment_dim);
    sfRectangleShape_setScale(level, scale);
    sfRectangleShape_setTexture(level, location->texture, true);

    sfRenderWindow_drawRectangleShape(state->renderer, level, 0);
}

internal void DrawStaticEntity(Game_State *state, Entity *entity, Asset *texture) {
    sfSprite *sprite = sfSprite_create();

    //sfSprite_setOrigin(sprite, entity->half_dim);
    sfSprite_setPosition(sprite, entity->position - (entity->half_dim));
    sfSprite_setScale(sprite, entity->scale);
    sfSprite_setTexture(sprite, texture->texture, true);

    sfRenderWindow_drawSprite(state->renderer, sprite, 0);

    sfSprite_destroy(sprite);
}

internal void DrawStaticEntity(Game_State *state, Entity *entity) {
    // All static entities preceed Player in the type enum
    Assert(entity->type < EntityType_Player);

    char name[256];
    snprintf(name, sizeof(name), "Entity%02d", entity->type);
    Asset *texture = GetAsset(&state->assets, name);
    Assert(texture->type == Asset_Texture);

    DrawStaticEntity(state, entity, texture);
}

internal void UpdateChaseBubbles(Game_State *state, Play_State *playState, Game_Input *input, v2 spawnlines[8]) {
    f32 dt = input->delta_time;

    sfCircleShape *r = sfCircleShape_create();
    sfColor boarderColour = { 67, 23, 16, 255 };
    sfCircleShape_setOutlineColor(r, boarderColour);
    sfCircleShape_setOutlineThickness(r, 2);
    sfCircleShape_setFillColor(r, sfBlack);

    for(u32 i = 0; i < 1000; i++) {
        Chase_Bubble *bubble = &playState->chase_bubbles[i];
        if(!bubble->active) {
            bubble->active = true;
            u32 offset = ((u32)(i / 250)) * 2;
            v2 dir = spawnlines[0 + offset] - spawnlines[1 + offset];
            f32 len = Length(dir);
            v2 spawnpoint = spawnlines[0 + offset] - (dir * cast (f32) (1.0/len) * random(0, len));
            bubble->position = spawnpoint;
            bubble->radius = random(50, 60);
        }

        bubble->position += V2(random(-3, 3)*dt, random(-3, 3)*dt);
        bubble->radius -= random(40, 50)*dt;
        if(bubble->radius < 15) {
            bubble->active = false;
            continue;
        }

        sfCircleShape_setOrigin(r, V2(bubble->radius, bubble->radius));
        sfCircleShape_setRadius(r, bubble->radius);
        sfCircleShape_setPosition(r, bubble->position);
        sfRenderWindow_drawCircleShape(state->renderer, r, 0);
    }

    sfCircleShape_destroy(r);
}

internal void UpdateRenderPlayState(Game_State *state, Play_State *playState, Game_Input *input) {
    World *world = &playState->world;

    if(!playState->initialised) {
        if (!playState->from_editor) {
            LoadLevelFromFile(state, world, "Level.aml");
        }

        Entity *player = &world->entities[0]; // @Note: Slot 0 is reserved for the player
        player->velocity = V2(0, 0);
        Assert(player->type == EntityType_Player);

        playState->last_checkpoint = player->position;

        player->animation = CreatePlayerAnimation(&state->assets);
        player->health    = 3;

        player->half_dim = V2(35, 50); // @Hack: Should be set by file

        for (u32 it = 1; it < world->entity_count; ++it) {
            Entity *entity = &world->entities[it];
            AddFlags(&entity->flags, EntityState_Active);

            switch (entity->type) {
                case EntityType_Torch: {
                    entity->animation = CreateTorchAnimation(&state->assets);
                    AddFlags(&entity->flags, EntityState_Unchecked);
                }
                break;
                case EntityType_Spirit: {
                    entity->animation = CreateSpiritAnimation(&state->assets);
                }
                break;
                case EntityType_Tentacle: {
                    entity->animation = CreateTentacleAnimation(&state->assets);
                }
                break;
                case EntityType_Goal: {
                    entity->animation = CreateGoalAnimation(&state->assets);
                    AddFlags(&entity->flags, EntityState_Unchecked);
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
        playState->player_spawn = player->position;

        MusicLayers *music = &playState->music[0];


        playState->initialised = true;
    }

    if (JustPressed(input->f[11])) {
        RemoveLevelState(state);
        return;
    }

    f32 dt = input->delta_time;
    Game_Controller *controller = &input->controllers[0];

    MusicHandlerUpdate(state, &playState->music[0], dt);

    v2 view_size          = sfView_getSize(state->view);
    v2 screen_segment_dim = V2(2880, 1440); // @Todo: This will come from the world

    Entity *player = &world->entities[0];

    Level_Segment active[5]; // @Note: The segment the player is currently in and the 4 adjacent segments
    u32 count = GetActiveLevelSegments(world, active);

    if (player->health != 0 || playState->current_segment_light_count > 0) {
        sfShader_bind(state->diffuse_shader);
    }
    else {
        sfShader_bind(state->ambient_shader);
    }

    playState->current_segment_light_count = 0;

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

            if (HasFlags(entity->flags, EntityState_Lit)) {
                AddLight(playState, entity->position, 0.03, V3(1, 1, 1));
                if (it == 0) { playState->current_segment_light_count += 1; }
            }

            switch (entity->type) {
                case EntityType_Barrel:
                case EntityType_Painting:
                case EntityType_Wall:
                case EntityType_Carpet:
                case EntityType_Fireplace:
                case EntityType_Window:
                case EntityType_Banner:
                case EntityType_Platform:
                case EntityType_Pipe:
                case EntityType_Rocks: {
                    DrawStaticEntity(state, entity);
                }
                break;
                case EntityType_Statue: {
                    Bounding_Box player_box = CreateBox(player->position, player->half_dim);

                    v2 statue_dim = V2(entity->half_dim.x * 4, entity->half_dim.y);
                    Bounding_Box statue_box = CreateBox(entity->position, statue_dim);

                    if (Overlaps(&player_box, &statue_box)) {
                        Entity *e = GetNextScratchEntity(world);
                        e->type = EntityType_DarkWall;
                        e->half_dim = V2(10,10);
                        music->drums = Music_Request;
                        music->hat = Music_Request;
                        music->arpeggio = Music_Request;

                        AddFlags(&e->flags, EntityState_Active);

                        v2 dir = playState->player_spawn - player->position;

                        f32 len = Length(dir);
                        e->position = entity->position;
                    }

                    DrawStaticEntity(state, entity);
                }
                break;
                case EntityType_Torch: {
                    if (HasFlags(entity->flags, EntityState_Unchecked)) {
                        Asset *asset = GetAsset(&state->assets, "TorchOff");
                        DrawStaticEntity(state, entity, asset);

                        Bounding_Box torch_box  = CreateBox(entity->position, entity->half_dim);
                        Bounding_Box player_box = CreateBox(player->position, player->half_dim);

                        if (JustPressed(controller->interact)) {
                            player->health = 3;
                            playState->last_checkpoint = entity->position;

                            if (Overlaps(&torch_box, &player_box)) {
                                RemoveFlags(&entity->flags, EntityState_Unchecked);
                                AddFlags(&entity->flags, EntityState_Lit);
                            }

                            for (u32 a = 0; a < ArrayCount(world->scratch_entities); ++a) {
                                Entity *e = &world->scratch_entities[a];
                                if (e->type == EntityType_DarkWall) {
                                    RemoveFlags(&e->flags, EntityState_Active);
                                }
                            }
                        }
                    }
                    else {
                        UpdateRenderAnimation(state, &entity->animation, entity->position, dt);
                    }
                }
                break;
                case EntityType_Goal: {
                    if (HasFlags(entity->flags, EntityState_Unchecked)) {
                        Asset *asset = GetAsset(&state->assets, "GoalOff");
                        DrawStaticEntity(state, entity, asset);

                        Bounding_Box torch_box  = CreateBox(entity->position, entity->half_dim);
                        Bounding_Box player_box = CreateBox(player->position, player->half_dim);

                        if (JustPressed(controller->interact)) {
                            if (Overlaps(&torch_box, &player_box)) {
                                RemoveFlags(&entity->flags, EntityState_Unchecked);
                                AddFlags(&entity->flags, EntityState_Lit);

                                playState->goals_activated += 1;
                                if (playState->goals_activated == 4) {
                                    Level_State *over = CreateLevelState(state, LevelType_GameOver);
                                    Game_Over_State *overover = &over->over;
                                    overover->message = "You Won!";
                                    overover->submessage = "Press space to close the game!";

                                    return;
                                }
                            }
                        }
                    }
                    else {
                        UpdateRenderAnimation(state, &entity->animation, entity->position, dt);
                    }
                }
                break;
                case EntityType_Raghead: {
                    local f32 attack_cooldown = 2;

                    attack_cooldown -= dt;

                    v2 player_dir = player->position - entity->position;
                    f32 player_dist = Length(player_dir);

                    if ((player_dist < 200) && attack_cooldown < 0) {
                        Entity *attack = GetNextScratchEntity(world);
                        attack->type = EntityType_Wind;
                        attack->scale = V2(1, 1);
                        attack->half_dim = V2(10, 10);
                        AddFlags(&attack->flags, EntityState_Active);

                        player_dir *= (1.0f / player_dist);

                        attack->position = entity->position;
                        attack->velocity = 80 * player_dir;

                        attack_cooldown = 2;
                    }
                    else {
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
                    }

                    DrawStaticEntity(state, entity);
                    f32 pLength = Length(player->position - entity->position);
                    if(pLength - 400 < 5) {
                        Asset *rag_sound = GetAsset(&state->assets, "SackSound");
                        sfSound *sound = GetSound(playState, rag_sound->sound);
                        Assert(sound);
                        sfSoundStatus soundState = sfSound_getStatus(sound);
                        if(soundState != sfPlaying) {
                            sfSound_setVolume(sound, 15);
                            sfSound_play(sound);
                        }
                    }
                }
                break;
                case EntityType_Spirit: {
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
                case EntityType_Tentacle: {
                    UpdateRenderAnimation(state, &entity->animation, entity->position, dt);
                }
                break;
            }
        }
    }

    for (u32 it = 0; it < ArrayCount(world->scratch_entities); ++it) {
        Entity *entity = &world->scratch_entities[it];
        if (!HasFlags(entity->flags, EntityState_Active)) { continue; }

        if (HasFlags(entity->flags, EntityState_Lit)) {
            switch (entity->type) {
                case EntityType_Fireball: {
                    AddLight(playState, entity->position, 0.03 / entity->scale.x, V3(0.6, 0.6, 1));
                }
                break;
            }

            if (it == 0) { playState->current_segment_light_count += 1; }
        }

        switch (entity->type) {
            case EntityType_Wind: {
                // @Hack: Don't have texture for wind yet
                entity->type = EntityType_Fireball;
                DrawStaticEntity(state, entity);
                entity->type = EntityType_Wind;

                entity->position += (dt * entity->velocity);
            }
            break;
            case EntityType_DarkWall: {
                sfShader_bind(0);
                if(HasFlags(entity->flags, EntityState_Active)) {

                    entity->half_dim += V2(75, 75) * dt;
                    Bounding_Box player_box = CreateBox(player->position, player->half_dim);
                    Bounding_Box wall_box = CreateBox(entity->position, entity->half_dim);
                    if(Overlaps(&wall_box, &player_box)) {

                        entity->type = EntityType_Barrel;
                        RemoveFlags(&entity->flags, EntityState_Active);

                        player->position = playState->last_checkpoint;

                        Level_State *over = CreateLevelState(state, LevelType_GameOver);
                        Game_Over_State *overover = &over->over;
                        overover->message = "You Died!";
                        overover->submessage = "Press space to respawn at last checkpoint!";
                        overover->died = true;

                        return;
                    }
                    v2 pos = entity->position;
                    v2 size = entity->half_dim;
                    v2 line[8] = {
                        V2(pos.x - size.x, pos.y - size.y),
                        V2(pos.x - size.x, pos.y + size.y),

                        V2(pos.x + size.x, pos.y - size.y),
                        V2(pos.x + size.x, pos.y + size.y),

                        V2(pos.x - size.x, pos.y + size.y),
                        V2(pos.x + size.x, pos.y + size.y),

                        V2(pos.x - size.x, pos.y - size.y),
                        V2(pos.x + size.x, pos.y - size.y),
                    };
                    sfRectangleShape *dis_rect = sfRectangleShape_create();
                    sfRectangleShape_setOrigin(dis_rect, entity->half_dim);
                    sfRectangleShape_setPosition(dis_rect, entity->position);
                    sfRectangleShape_setSize(dis_rect, entity->half_dim*2);
                    sfColor rect_col = {0,0,0,255};
                    sfRectangleShape_setFillColor(dis_rect, rect_col);
                    sfRenderWindow_drawRectangleShape(state->renderer, dis_rect, NULL);
                    sfRectangleShape_destroy(dis_rect);
                    UpdateChaseBubbles(state, playState, input, line);
                } else if(Length(entity->half_dim) > 1){
                    entity->half_dim -= V2(500, 500) * dt;
                    v2 pos = entity->position;
                    v2 size = entity->half_dim;
                    v2 line[8] = {
                        V2(pos.x - size.x, pos.y - size.y),
                        V2(pos.x - size.x, pos.y + size.y),

                        V2(pos.x + size.x, pos.y - size.y),
                        V2(pos.x + size.x, pos.y + size.y),

                        V2(pos.x - size.x, pos.y + size.y),
                        V2(pos.x + size.x, pos.y + size.y),

                        V2(pos.x - size.x, pos.y - size.y),
                        V2(pos.x + size.x, pos.y - size.y),
                    };

                    sfRectangleShape *dis_rect = sfRectangleShape_create();
                    sfRectangleShape_setOrigin(dis_rect, entity->half_dim);
                    sfRectangleShape_setPosition(dis_rect, entity->position);
                    sfRectangleShape_setSize(dis_rect, entity->half_dim*2);
                    sfColor rect_col = {0,0,0,255};
                    sfRectangleShape_setFillColor(dis_rect, rect_col);
                    sfRenderWindow_drawRectangleShape(state->renderer, dis_rect, NULL);
                    sfRectangleShape_destroy(dis_rect);
                    UpdateChaseBubbles(state, playState, input, line);
                }

                if (player->health != 0 || playState->current_segment_light_count > 0) {
                    sfShader_bind(state->diffuse_shader);
                }
                else {
                    sfShader_bind(state->ambient_shader);
                }
            }
            break;
            case EntityType_Fireball: {
                entity->position += (dt * entity->velocity);
                entity->scale -= dt * V2(0.18, 0.18);

                DrawStaticEntity(state, entity);

                if (entity->scale.x < 0.1) {
                    RemoveFlags(&entity->flags, EntityState_Active | EntityState_Lit);
                    break;
                }

                Bounding_Box fireball_box = CreateBox(entity->position, entity->half_dim);
                for (u32 it = 0; it < count; ++it) {
                    Level_Segment *segment = &active[it];

                    b32 stop = false;
                    for (u32 range = segment->entity_range_start;
                            range < segment->entity_range_one_past_last;
                            ++range)
                    {
                        Entity *other = &world->entities[range];
                        Bounding_Box other_box = CreateBox(other->position, other->half_dim);

                        if (other->type == EntityType_Raghead) {
                            Bounding_Box other_box = CreateBox(other->position, other->half_dim);
                            if (Overlaps(&fireball_box, &other_box)) {
                                RemoveFlags(&entity->flags, EntityState_Active);
                                RemoveFlags(&other->flags, EntityState_Active);
                            }
                        }
                        else if (other->type == EntityType_Spirit) {
                            if (Overlaps(&fireball_box, &other_box)) {
                                RemoveFlags(&entity->flags, EntityState_Active);
                            }

                            stop = true;
                            break;
                        }
                    }

                    if (stop) { break; } // @Hack: To get out of nested loop
                }
            }
            break;
        }
    }

    UpdateRenderPlayer(state, playState, input, player);

    UploadLightInformation(state, playState);

    sfShader_bind(0);

    if(JustPressed(controller->interact)) {
    }

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

internal void UpdateRenderCredits(Game_State *state, Credits_State *credits, Game_Input *input) {
    sfRenderWindow_clear(state->renderer, sfBlack);
    v2 view_size = sfView_getSize(state->view);
    if(!credits->initialised) {
        credits->initialised = true;
    }
    v2 mouse_pos = input->mouse_position;
    Bounding_Box mouse = {};
    mouse.centre = mouse_pos;
    mouse.half_dim = V2(2, 2);
    sfFont *font = GetAsset(&state->assets, "ubuntu")->font;
    sfText *text = sfText_create();
    sfText_setFont(text, font);
    sfText_setCharacterSize(text, 36);

    sfText_setString(text, "Back To Menu");
    sfFloatRect bounds = sfText_getLocalBounds(text);
    Bounding_Box *b1 = &credits->buttons[0];
    b1->centre = V2(view_size.x - bounds.width/2 - 20, view_size.y - 20);
    b1->half_dim = V2(bounds.width/2, bounds.height/2);
    sfText_setColor(text, sfWhite);
    if(Overlaps(b1, &mouse)) {
        sfText_setColor(text, sfRed);
        if (WasPressed(input->mouse_buttons[0])) {
            free(RemoveLevelState(state));
        }
    }

    sfText_setOrigin(text, V2(0, 0));
    sfText_setPosition(text, V2(view_size.x - bounds.width - 20, view_size.y - bounds.height - 20));
    sfRenderWindow_drawText(state->renderer, text, 0);


    sfText_setString(text, "James Bulman: Programming");
    bounds = sfText_getLocalBounds(text);
    sfText_setColor(text, sfWhite);

    sfText_setOrigin(text, V2(0, 0));
    sfText_setPosition(text, V2(20, view_size.y/2 - bounds.height - 20));
    sfRenderWindow_drawText(state->renderer, text, 0);

    sfText_setString(text, "Cameron Thronton: Art");
    sfText_setColor(text, sfWhite);

    sfText_setOrigin(text, V2(0, 0));
    sfText_setPosition(text, V2(20, view_size.y/2));
    sfRenderWindow_drawText(state->renderer, text, 0);

    sfText_setString(text, "Alex Goldsack: Music");
    sfText_setColor(text, sfWhite);

    sfText_setOrigin(text, V2(0, 0));
    sfText_setPosition(text, V2(20, view_size.y/2 + bounds.height + 20));
    sfRenderWindow_drawText(state->renderer, text, 0);

    sfText_setString(text, "Matt Threlfall: Programming and Bugs");
    sfText_setColor(text, sfWhite);

    sfText_setOrigin(text, V2(0, 0));
    sfText_setPosition(text, V2(20, view_size.y/2 + bounds.height*2 + 40));
    sfRenderWindow_drawText(state->renderer, text, 0);

    sfText_setColor(text, sfWhite);
    sfText_setString(text, "Credits");
    sfText_setCharacterSize(text, 72);
    bounds = sfText_getLocalBounds(text);
    sfText_setOrigin(text, V2(bounds.width / 2, bounds.height / 2));
    sfText_setPosition(text, V2(view_size.x/2, view_size.y/10));
    sfRenderWindow_drawText(state->renderer, text, 0);
    sfText_destroy(text);
}

internal void UpdateRenderMenu(Game_State *state, Menu_State *menu, Game_Input *input) {
    sfRenderWindow_clear(state->renderer, sfBlack);
    v2 view_size = sfView_getSize(state->view);
    if(!menu->initialised) {
        menu->initialised = true;
    }

    v2 mouse_pos = input->mouse_position;
    Bounding_Box mouse = {};
    mouse.centre = mouse_pos;
    mouse.half_dim = V2(2, 2);

    sfFont *font = GetAsset(&state->assets, "ubuntu")->font;
    sfText *text = sfText_create();
    sfText_setFont(text, font);
    sfText_setCharacterSize(text, 36);

    sfText_setString(text, "Quit Game");
    f32 text_loc = 9*view_size.y/10;
    sfFloatRect bounds = sfText_getLocalBounds(text);
    f32 spacing = bounds.height/2 + 30;

    Bounding_Box *b1 = &menu->buttons[0];
    b1->centre = V2(30 + bounds.width/2, text_loc + bounds.height/2);
    b1->half_dim = V2(100, bounds.height/2);
    sfText_setColor(text, sfWhite);
    if(Overlaps(b1, &mouse)) {
        sfText_setColor(text, sfRed);
        if (WasPressed(input->mouse_buttons[0])) {
            printf("Quit\n");
        }
    }

    sfText_setOrigin(text, V2(0, bounds.height/2));
    sfText_setPosition(text, V2(30, text_loc));
    sfRenderWindow_drawText(state->renderer, text, 0);

    sfText_setString(text, "Credits");
    bounds = sfText_getLocalBounds(text);
    text_loc -= spacing;
    b1->centre = V2(30 + bounds.width/2, text_loc + bounds.height/2);
    b1->half_dim = V2(100, bounds.height/2);
    sfText_setColor(text, sfWhite);
    if(Overlaps(b1, &mouse)) {
        sfText_setColor(text, sfRed);
        if (WasPressed(input->mouse_buttons[0])) {
            CreateLevelState(state, LevelType_Credits);
        }
    }

    sfText_setOrigin(text, V2(0, bounds.height/2));
    sfText_setPosition(text, V2(30, text_loc));
    sfRenderWindow_drawText(state->renderer, text, 0);

    sfText_setString(text, "Start");
    bounds = sfText_getLocalBounds(text);
    text_loc -= spacing;

    b1->centre = V2(30 + bounds.width/2, text_loc + bounds.height/2);
    b1->half_dim = V2(100, bounds.height/2);

    sfText_setColor(text, sfWhite);
    if(Overlaps(b1, &mouse)) {
        sfText_setColor(text, sfRed);
        if (WasPressed(input->mouse_buttons[0])) {
            free(RemoveLevelState(state));
        }
    }

    sfText_setOrigin(text, V2(0, bounds.height/2));
    sfText_setPosition(text, V2(30, text_loc));
    sfRenderWindow_drawText(state->renderer, text, 0);

    sfText_setColor(text, sfWhite);
    sfText_setString(text, "Candle Fright");
    sfText_setCharacterSize(text, 72);
    bounds = sfText_getLocalBounds(text);
    sfText_setOrigin(text, V2(bounds.width / 2, bounds.height / 2));
    sfText_setPosition(text, V2(view_size.x/2, view_size.y/10));
    sfRenderWindow_drawText(state->renderer, text, 0);
    sfText_destroy(text);
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

internal void UpdateRenderGameOver(Game_State *state, Game_Over_State *over, Game_Input *input) {
    if (!over->inti) {
        sfFloatRect rect;
        rect.top = 0;
        rect.left = 0;
        rect.width = 1280;
        rect.height = 720;
        sfView_reset(state->view, rect);

        sfView_setCenter(state->view, V2(640, 360));
        sfView_setSize(state->view, V2(1280, 720));

        sfRenderWindow_setView(state->renderer, state->view);

        printf("KLFJD:LKSFJ;kl\n");

        over->inti = true;
    }

    Asset *font = GetAsset(&state->assets, "ubuntu");

    sfShader_bind(0);

    v2 view_size = sfView_getSize(state->view);

    sfText *text = sfText_create();


    sfText_setColor(text, sfWhite);
    sfText_setFillColor(text, sfWhite);
    sfText_setString(text, over->message);
    sfText_setCharacterSize(text, 72);


    Game_Controller *controller = &input->controllers[0];

    sfFloatRect bounds = sfText_getLocalBounds(text);
    sfText_setOrigin(text, V2(bounds.width / 2, bounds.height / 2));
    sfText_setPosition(text, V2(view_size.x/2, view_size.y/10));
    sfRenderWindow_drawText(state->renderer, text, 0);

    sfText_destroy(text);

    if (JustPressed(controller->jump)) {
        exit(1);
    }
}

internal void LudumUpdateRender(Game_State *state, Game_Input *input) {
    if (!state->initialised) {
        CreateLevelState(state, LevelType_Edit);
        CreateLevelState(state, LevelType_Menu);
        CreateLevelState(state, LevelType_Logo);
        // TODO RELEASE: Enable logo

        InitAssets(&state->assets, 64);

        for (u32 it = 0; it < EntityType_Count; ++it) {
            char buf[256];
            umm len = snprintf(buf, sizeof(buf), "Entity%02d", it);

            char *name = cast(char *) Alloc(len + 1);
            CopySize(name, buf, len + 1);

            u32 flags = 0;
            if (it >= EntityType_Player) { flags = AssetFlag_Animation; }

            printf("Loading %s ...\n", name);

            LoadAsset(&state->assets, name, Asset_Texture, flags);
        }

        LoadAsset(&state->assets, "Location00", Asset_Texture);
        LoadAsset(&state->assets, "Location01", Asset_Texture);
        LoadAsset(&state->assets, "Location02", Asset_Texture);
        LoadAsset(&state->assets, "Location03", Asset_Texture);
        LoadAsset(&state->assets, "Location04", Asset_Texture);
        LoadAsset(&state->assets, "Location05", Asset_Texture);

        LoadAsset(&state->assets, "DarkClouds", Asset_Texture);

        LoadAsset(&state->assets, "TorchOff", Asset_Texture);
        LoadAsset(&state->assets, "GoalOff", Asset_Texture);

        LoadAsset(&state->assets, "CandleLow", Asset_Texture, AssetFlag_Animation);
        LoadAsset(&state->assets, "CandleMid", Asset_Texture, AssetFlag_Animation);
        LoadAsset(&state->assets, "CandleHigh", Asset_Texture, AssetFlag_Animation);
        LoadAsset(&state->assets, "CandleDead", Asset_Texture);

        LoadAsset(&state->assets, "PNoise", Asset_Texture);

        LoadAsset(&state->assets, "Rain", Asset_Texture);
		LoadAsset(&state->assets, "logo", Asset_Texture);

        // Music
        LoadAsset(&state->assets, "organ", Asset_Music);
        LoadAsset(&state->assets, "swell", Asset_Music);
        LoadAsset(&state->assets, "arpeggio", Asset_Music);
        LoadAsset(&state->assets, "drums", Asset_Music);
        LoadAsset(&state->assets, "hat", Asset_Music);

        // Sounds
        LoadAsset(&state->assets, "SackSound", Asset_Sound);

        // Fonts
        LoadAsset(&state->assets, "ubuntu", Asset_Font);

        const char *vertex_code = R"VERT(
            varying out vec2 frag_position;

            void main() {
                gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
                gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

                frag_position = vec2(gl_ModelViewMatrix * gl_Vertex);
            }
        )VERT";

        const char *diffuse_frag_code = R"FRAG(
            in vec2 frag_position;

            #define MAX_LIGHTS 16

            varying out vec4 final_colour;

            uniform sampler2D image;

            // Light Information
            uniform int light_count;
            uniform vec2 light_positions[MAX_LIGHTS];
            uniform vec3 light_colours[MAX_LIGHTS];
            uniform float light_distance_scales[MAX_LIGHTS];

            uniform sampler2D noise;
            uniform float time;

            void main() {
                vec3 diffuse = vec3(0, 0, 0);
                for (int it = 0; it < light_count; ++it) {
                    vec2 dir = light_positions[it] - frag_position;

                    vec2 noise_uv = vec2(abs(sin(0.1 * time)), abs(cos(0.1 * time)));
                    float dist = (light_distance_scales[it] - (0.01 * texture2D(noise, noise_uv))) * length(dir);

                    float attenuation = 1.0 / (1.0 + (0.09 * dist) + (0.032 * (dist * dist)));

                    diffuse += attenuation * light_colours[it];
                }

                final_colour = vec4(diffuse, 1) * texture2D(image, gl_TexCoord[0].xy);
            }
        )FRAG";

        const char *ambient_frag_code = R"FRAG(
            in vec2 frag_position;

            varying out vec4 final_colour;

            uniform sampler2D image;

            void main() {
                vec4 ambient = vec4(0.05, 0.05, 0.05, 1);
                final_colour = ambient * texture2D(image, gl_TexCoord[0].xy);
            }
        )FRAG";

        state->diffuse_shader = sfShader_createFromMemory(vertex_code, 0, diffuse_frag_code);
        state->ambient_shader = sfShader_createFromMemory(vertex_code, 0, ambient_frag_code);
        Assert(state->diffuse_shader && state->ambient_shader);

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
        case LevelType_Menu: {
            Menu_State *menu = &current_state->menu;
            UpdateRenderMenu(state, menu, input);
        }
        break;
        case LevelType_Credits: {
            Credits_State *credits = &current_state->credits;
            UpdateRenderCredits(state, credits, input);
        }
        break;
        case LevelType_GameOver: {
            Game_Over_State *over = &current_state->over;
            UpdateRenderGameOver(state, over, input);
        }
        break;
    }

}
