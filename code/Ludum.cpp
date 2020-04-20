#include "Ludum_Assets.cpp"
#include "Ludum_Animation.cpp"
#include "Ludum_Editor.cpp"

#if 0
internal void AddFireBall(Play_State *playState) {
=======
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

internal void AddFireBall(Play_State *playState, Game_Input *input) {
>>>>>>> remotes/origin/matt
    for(u8 i = 0; i < 3; i++) {
        Fire_Ball *fireball = &playState->fire_balls[i];
        Player *player = &playState->player[0];
        if(!fireball->active) {
            fireball->active = true;
            fireball->radius = 15;
            fireball->position = player->position;
            v2 dir = input->mouse_position - fireball->position;
            f32 len = length(dir);
            fireball->velocity = 100 * dir * (1/sqrt(len));
            Player *player = &playState->player[0];
            player->fireball_break = true;
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

        fireball->rotation-=360*input->delta_time;
        if(length(fireball->velocity) < 30000) {
            fireball->velocity += fireball->velocity * 300 * input->delta_time;
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

            player->velocity.y = -450;
            player->jump_time  = 0.3f;
            player->floor_time = 0;

            player->animation.pause = false;
        }
        else if (player->jump_time > 0) {
            player->velocity.y -= 1230 * dt;
        }
    }

    player->velocity.x *= (HasFlags(player->flags, EntityState_OnGround) ? 1 : 0.85);
    player->velocity.y += (dt * 1300);
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

    f32 light_distance_scale = ((3 - player->health) * 0.06);
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

internal void UpdateChaseBubbles(Game_State *state, Play_State *playState, Game_Input *input, v2 spawnlines[8]) {
    f32 dt = input->delta_time;
    for(u32 i = 0; i < 2000; i++) {
        Chase_Bubble *bubble = playState->chase_bubbles[i];
        if(!bubble->active) {
            bubble->active = true;
            u32 offset = (u32)floor(i/500)*2;
            v2 dir = spawnlines[0 + offset] - spawnlines[1 + offset];
            f32 len = Length(dir); 
            v2 spawnpoint = spawnlines[0] - (dir * cast (f32) (1.0/sqrt(len)) * random(0, len));
            bubble->position = spawnpoint;
            bubble->radius = random(60, 90);
        }
        bubble->position += V2(random(-3, 3)*dt, random(-3, 3)*dt);
        bubble->radius -= random(50, 60)*dt;
        if(bubble->radius < 10) {
            bubble->active = false;
            continue;
        }
        sfCircleShape *r = sfCircleShape_create();
        sfCircleShape_setOrigin(r, V2(bubble->radius + 1, bubble->radius + 1));
        sfCircleShape_setRadius(r, bubble->radius + 1);
        sfColor boarderColour = {
               67,
               23,
               16,
               255
        };
        sfCircleShape_setPosition(r, bubble->position);
        sfCircleShape_setFillColor(r, boarderColour);
        sfRenderWindow_drawCircleShape(state->renderer, r, 0);
        sfCircleShape_destroy(r);
    }
    for(u32 i = 0; i < 2000; i++) {
        Chase_Bubble *bubble = playState->chase_bubbles[i];
        sfCircleShape *r = sfCircleShape_create();
        sfCircleShape_setOrigin(r, V2(bubble->radius, bubble->radius));
        sfCircleShape_setRadius(r, bubble->radius);
        sfCircleShape_setPosition(r, bubble->position);
        sfCircleShape_setFillColor(r, sfBlack);
        sfRenderWindow_drawCircleShape(state->renderer, r, 0);
        sfCircleShape_destroy(r);
    }
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
        MusicLayers *music = &playState->music[0];
        music->drums = Music_Request;
        music->hat = Music_Request;
        music->arpeggio = Music_Request;

        for(u32 i = 0; i < 2000; i++) {
            playState->chase_bubbles[i] = cast(Chase_Bubble *) Alloc(sizeof(Chase_Bubble));
        }

        playState->initialised = true;
    }

    f32 dt = input->delta_time;

    MusicHandlerUpdate(state, &playState->music[0], dt);

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
                case EntityType_DarkWall: {
                    if(HasFlags(entity->flags, EntityState_Active)) {
                        /* @TODO Matt: When the screen is activated run this
                        v2 dir = playState->player_spawn - player->position;
                        f32 len = length(dir);
                        entity->location = view_size.x/2 * dir * (1/sqrt(len));
                        */

                        entity->half_dim += V2(200, 200) * dt;
                        Bounding_Box player_box = CreateBox(player->position, player->half_dim);
                        Bounding_Box wall_box = CreateBox(entity->position, entity->half_dim);
                        if(Overlaps(&wall_box, &player_box)) {
                            // TODO : Player dead!
                            Assert(false);
                        }
                        v2 pos = entity->position;
                        v2 size = entity->half_dim;
                        v2 line[8] = {
                            V2(pos.x+size.x, pos.y+size.y),
                            V2(pos.x+size.x, pos.y-size.y),
                            V2(pos.x-size.x, pos.y-size.y),
                            V2(pos.x-size.x, pos.y+size.y),
                            V2(pos.x-size.x, pos.y+size.y),
                            V2(pos.x+size.x, pos.y+size.y),
                            V2(pos.x+size.x, pos.y-size.y),
                            V2(pos.x-size.x, pos.y-size.y),
                        };
                        sfRectangleShape *dis_rect = sfRectangleShape_create();
                        sfRectangleShape_setPosition(dis_rect, entity->position);
                        sfRectangleShape_setTexture(dis_rect, GetAsset(&state->assets,"logo")->texture, false);
                        sfRectangleShape_setSize(dis_rect, entity->half_dim*2);
                        sfRectangleShape_setOrigin(dis_rect, entity->half_dim);
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
                            V2(pos.x+size.x, pos.y+size.y),
                            V2(pos.x+size.x, pos.y-size.y),
                            V2(pos.x-size.x, pos.y-size.y),
                            V2(pos.x-size.x, pos.y+size.y),
                            V2(pos.x-size.x, pos.y+size.y),
                            V2(pos.x+size.x, pos.y+size.y),
                            V2(pos.x+size.x, pos.y-size.y),
                            V2(pos.x-size.x, pos.y-size.y),
                        };
                        sfRectangleShape *dis_rect = sfRectangleShape_create();
                        sfRectangleShape_setPosition(dis_rect, entity->position);
                        sfRectangleShape_setTexture(dis_rect, GetAsset(&state->assets,"logo")->texture, false);
                        sfRectangleShape_setSize(dis_rect, entity->half_dim*2);
                        sfRectangleShape_setOrigin(dis_rect, entity->half_dim);
                        sfColor rect_col = {0,0,0,255};
                        sfRectangleShape_setFillColor(dis_rect, rect_col);
                        sfRenderWindow_drawRectangleShape(state->renderer, dis_rect, NULL);
                        sfRectangleShape_destroy(dis_rect);
                        UpdateChaseBubbles(state, playState, input, line);
                    }
                }
            }
        }
    }

    UpdateRenderPlayer(state, playState, input, player);
    sfShader_bind(0);

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
    sfFont *font = GetAsset(&state->assets, "ubuntu")->font;
    sfText *text = sfText_create();
    sfText_setFont(text, font);
    sfText_setCharacterSize(text, 36);

    sfText_setString(text, "Back To Menu");
    sfFloatRect bounds = sfText_getLocalBounds(text);
    sfText_setColor(text, sfWhite);

    sfText_setOrigin(text, V2(0, 0));
    sfText_setPosition(text, V2(bounds.width-bounds.width - 10, bounds.height - 10));
    sfRenderWindow_drawText(state->renderer, text, 0);
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
            CreateLevelState(state, LevelType_Logo);
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
    sfText_setString(text, "Candle Light");
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

internal void LudumUpdateRender(Game_State *state, Game_Input *input) {
    if (!state->initialised) {
        //CreateLevelState(state, LevelType_Edit);
        CreateLevelState(state, LevelType_Play);
        // TODO RELEASE: Enable logo
        CreateLevelState(state, LevelType_Menu);
        //CreateLevelState(state, LevelType_Logo);

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
    }

}
