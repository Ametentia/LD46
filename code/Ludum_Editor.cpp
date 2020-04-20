internal void ConvertToEditor(World *world, Edit_State *edit) {
    for (u32 it = 0; it < 128; ++it) {
        Edit_Segment *seg = &edit->segments[it / 8][it % 8];
        Level_Segment *world_seg = &world->segments[it];

        if (!world_seg->in_use) { continue; }

        seg->in_use = true;

        seg->grid_x = it / 8;
        seg->grid_y = it % 8;

        seg->texture_index = world_seg->texture_number;
        seg->entity_count = world_seg->entity_count;
        seg->box_count    = world_seg->box_count;

        CopySize(seg->entities, &world->entities[world_seg->entity_range_start],
                seg->entity_count * sizeof(Entity));

        CopySize(seg->boxes, &world->boxes[world_seg->box_range_start], seg->box_count * sizeof(Bounding_Box));

        printf("Seg has %d, %d\n", seg->entity_count, seg->box_count);
    }

    edit->player_placed = true;
    edit->player = world->entities[0];
}

internal void WriteLevelToFile(Game_State *state, Edit_State *edit, const char *level_name) {
    FILE *handle = fopen(level_name, "wb");

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

    total_entity_count += 1;

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

    umm entity_size = sizeof(Entity);
    fwrite(&entity_size, sizeof(umm), 1, handle);

    u32 running_total_entities = 1;
    u32 running_total_boxes = 0;

    for (u32 x = 0; x < 16; ++x) {
        for (u32 y = 0; y < 8; ++y) {
            Level_Segment segment = {};
            Edit_Segment *edit_seg = &edit->segments[x][y];

            segment.grid[0] = x;
            segment.grid[1] = y;

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

    // @Note: Slot 0 is reserved for the player
    fwrite(&edit->player, sizeof(Entity), 1, handle);

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

    world->segment_dim = V2(header.segment_dim[0], header.segment_dim[1]);

    world->entity_count = header.total_entity_count;
    world->entities = cast(Entity *) Alloc(world->entity_count * sizeof(Entity));

    umm entity_size = world->entity_count * header.entity_size;
    u8 *entity_data = cast(u8 *) Alloc(entity_size);
    fread(entity_data, entity_size, 1, handle);

    for (u32 it = 0; it < world->entity_count; ++it) {
        CopySize(&world->entities[it], &entity_data[it * header.entity_size], header.entity_size);
    }

    free(entity_data);

    world->box_count = header.total_box_count;
    world->boxes = cast(Bounding_Box *) Alloc(world->box_count * sizeof(Bounding_Box));

    fread(world->boxes, world->box_count * sizeof(Bounding_Box), 1, handle);

    fclose(handle);
}

internal void EditorToWorld(Game_State *state, Edit_State *edit, World *world) {
    WriteLevelToFile(state, edit, "Temp.aml");
    LoadLevelFromFile(state, world, "Temp.aml");
}

internal void UpdateRenderEdit(Game_State *state, Edit_State *edit, Game_Input *input) {
    // Initialisation edit mode
    //
    if (!edit->initialised) {
        printf("There are %d entity types\n", EntityType_Count);

        edit->zoom_factor = 1;
        edit->last_mouse  = V2(0, 0);
        edit->camera_pos  = V2(0, 0);

        edit->view = sfView_create();
        sfView_setCenter(edit->view, edit->camera_pos);
        sfView_setSize(edit->view, V2(1280, 720));

        edit->segment_dim = V2(2880, 1440); // @Todo: Load from level if one is present
        edit->current_segment = 0;

        edit->scale = V2(1, 1);
        edit->entity_type = 0;

        edit->initialised = true;
    }

    // Camera controls
    //
    if (IsPressed(input->mouse_buttons[2])) {
        v2 displacement = edit->zoom_factor * (edit->last_mouse - input->screen_mouse);
        sfView_move(edit->view, displacement);
    }

    if (input->mouse_wheel_delta > 0) {
        edit->zoom_factor *= (1.0f / 1.3f);
        sfView_zoom(edit->view, 1.0f / 1.3f);
    }
    else if (input->mouse_wheel_delta < 0) {
        edit->zoom_factor *= 1.3f;
        sfView_zoom(edit->view, 1.3f);
    }

    edit->last_mouse = input->screen_mouse;
    sfRenderWindow_setView(state->renderer, edit->view);

    // Change edit mode
    //
    for (u32 it = 1; it <= 4; ++it) {
        if (JustPressed(input->f[it])) {
            edit->mode = cast(Edit_Mode) (it - 1);
            break;
        }
    }

    if (JustPressed(input->f[10])) {
        World world = {};
        LoadLevelFromFile(state, &world, "Level.aml");

        ConvertToEditor(&world, edit);

        free(world.entities);
        free(world.boxes);

        printf("Loaded level successfully\n");
    }

    if (JustPressed(input->f[9])) {
        WriteLevelToFile(state, edit, "Level.aml");
        printf("Successfully wrote to file\n");
    }

    if (JustPressed(input->f[11])) {
        Level_State *level_state = CreateLevelState(state, LevelType_Play);

        Play_State *play = &level_state->play;

        play->from_editor = true;
        EditorToWorld(state, edit, &play->world);
        return;
    }

    // Mouse position in grid space
    //
    s32 grid_x = cast(s32) (input->mouse_position.x / edit->segment_dim.x);
    s32 grid_y = cast(s32) (input->mouse_position.y / edit->segment_dim.y);

    // Segment grid
    //
    sfRectangleShape *grid = sfRectangleShape_create();
    v2 offset = 0.05 * edit->segment_dim;

    for (u32 x = 0; x < 16; ++x) {
        for (u32 y = 0; y < 8; ++y) {
            Edit_Segment *segment = &edit->segments[x][y];

            v2 position = V2((x * edit->segment_dim.x) + offset.x, (y * edit->segment_dim.y) + offset.y);

            sfRectangleShape_setOrigin(grid, 0.5f * offset);
            sfRectangleShape_setSize(grid, 0.95 * edit->segment_dim);
            sfRectangleShape_setPosition(grid, position);
            sfRectangleShape_setTexture(grid, 0, false);

            if (edit->mode == EditMode_Select) {
                if (grid_x == x && grid_y == y) { sfRectangleShape_setFillColor(grid, sfGreen); }
                else { sfRectangleShape_setFillColor(grid, sfWhite); }
            }

            if (segment->in_use) {
                sfRectangleShape_setOrigin(grid, V2(0, 0));
                sfRectangleShape_setSize(grid, edit->segment_dim);

                sfRectangleShape_setPosition(grid, V2(x * edit->segment_dim.x, y * edit->segment_dim.y));

                char name[256];
                snprintf(name, sizeof(name), "Location%02d", segment->texture_index);
                Asset *location = GetAsset(&state->assets, name);

                sfRectangleShape_setTexture(grid, location->texture, false);
            }

            sfRenderWindow_drawRectangleShape(state->renderer, grid, 0);
        }
    }

    if (edit->current_segment) {
        Edit_Segment *seg = edit->current_segment;
        sfRectangleShape_setOrigin(grid, 0.5f * edit->segment_dim);
        sfRectangleShape_setSize(grid, 1.1 * edit->segment_dim);

        sfRectangleShape_setFillColor(grid, sfTransparent);
        sfRectangleShape_setOutlineColor(grid, sfGreen);
        sfRectangleShape_setOutlineThickness(grid, 2.5);

        v2 pos = V2(seg->grid_x * edit->segment_dim.x, seg->grid_y * edit->segment_dim.y) +
            (0.5f * edit->segment_dim) - (0.05 * edit->segment_dim);

        sfRectangleShape_setPosition(grid, pos);


        sfRenderWindow_drawRectangleShape(state->renderer, grid, 0);
    }

    sfRectangleShape_destroy(grid);

    // Handle edit modes
    //
    switch (edit->mode) {
        case EditMode_Select: {
            if (JustPressed(input->mouse_buttons[0])) {
                if ((grid_x >= 0 && grid_x < 16) && (grid_y >= 0 && grid_y < 8)) {
                    Edit_Segment *segment = &edit->segments[grid_x][grid_y];

                    if (segment->in_use) {
                        edit->current_segment = segment;
                    }
                }
            }
        }
        break;
        case EditMode_Segment: {
            // Input handling
            //
            if (JustPressed(input->debug_next)) {
                edit->segment_type += 1;
                if (edit->segment_type >= 3) { edit->segment_type = 0; } // @Hardcoded: Will change
            }
            else if (JustPressed(input->debug_prev)) {
                edit->segment_type -= 1;
                if (edit->segment_type < 0) { edit->segment_type = 2; } // @Hardcoded: Will change
            }

            if (JustPressed(input->mouse_buttons[0])) {
                if ((grid_x >= 0 && grid_x < 16) && (grid_y >= 0 && grid_y < 8)) {
                    Edit_Segment *segment = &edit->segments[grid_x][grid_y];

                    if (segment->in_use) {
                        if (segment->texture_index != edit->segment_type) {
                            segment->entity_count = 0;
                            segment->box_count    = 0;
                        }
                    }
                    else {
                        segment->entity_count = 0;
                        segment->box_count    = 0;
                    }

                    segment->grid_x = grid_x;
                    segment->grid_y = grid_y;

                    segment->texture_index = edit->segment_type;
                    segment->in_use = true;
                }
            }

            // Drawing the correct segment type over the hovered grid
            //
            char name[256];
            snprintf(name, sizeof(name), "Location%02d", edit->segment_type);
            Asset *location = GetAsset(&state->assets, name);

            if ((grid_x >= 0 && grid_x < 16) && (grid_y >= 0 && grid_y < 8)) {
                sfColor colour = { 255, 255, 255, 144 };

                sfRectangleShape *segment = sfRectangleShape_create();
                sfRectangleShape_setFillColor(segment, colour);
                sfRectangleShape_setTexture(segment, location->texture, false);
                sfRectangleShape_setSize(segment, edit->segment_dim);
                sfRectangleShape_setPosition(segment,
                        V2(grid_x * edit->segment_dim.x, grid_y * edit->segment_dim.y));

                sfRenderWindow_drawRectangleShape(state->renderer, segment, 0);

                sfRectangleShape_destroy(segment);
            }
        }
        break;
        case EditMode_BoundingBox: {
            if (!edit->current_segment) { break; }

            if (JustPressed(input->mouse_buttons[0])) {
                edit->first_mouse_down = input->mouse_position;
                edit->is_editing = true;
                break;
            }

            Edit_Segment *current = edit->current_segment;

            v2 min = V2(Min(input->mouse_position.x, edit->first_mouse_down.x),
                        Min(input->mouse_position.y, edit->first_mouse_down.y));

            v2 max = V2(Max(input->mouse_position.x, edit->first_mouse_down.x),
                        Max(input->mouse_position.y, edit->first_mouse_down.y));

            if (edit->is_editing && WasPressed(input->mouse_buttons[0])) {
                edit->is_editing = false;

                Bounding_Box *next = &current->boxes[current->box_count];
                current->box_count += 1;

                v2 segment_centre = V2(current->grid_x * edit->segment_dim.x,
                        current->grid_y * edit->segment_dim.y);

                v2 segment_min = segment_centre - (0.05  * edit->segment_dim);
                v2 segment_max = (1.05 * edit->segment_dim) + segment_centre;

                min.x = Max(min.x, segment_min.x);
                min.y = Max(min.y, segment_min.y);

                max.x = Min(max.x, segment_max.x);
                max.y = Min(max.y, segment_max.y);

                next->direction_flags = Direction_All; // @Todo: Make these configurable
                next->centre   = 0.5f * (max + min);
                next->half_dim = 0.5f * (max - min);
            }

            if (JustPressed(input->mouse_buttons[1])) {
                for (u32 it = 0; it < current->box_count; ++it) {
                    if (Contains(&current->boxes[it], input->mouse_position)) {
                        current->box_count -= 1;
                        Swap(current->boxes[it], current->boxes[current->box_count]);
                    }
                }
            }

            sfRectangleShape *bbox = sfRectangleShape_create();
            sfRectangleShape_setFillColor(bbox, sfTransparent);
            sfRectangleShape_setOutlineColor(bbox, sfRed);
            sfRectangleShape_setOutlineThickness(bbox, 1.5);

            for (u32 it = 0; it < current->box_count; ++it) {
                Bounding_Box *box = &current->boxes[it];
                sfRectangleShape_setOrigin(bbox, box->half_dim);
                sfRectangleShape_setPosition(bbox, box->centre);
                sfRectangleShape_setSize(bbox, 2 * box->half_dim);

                sfRenderWindow_drawRectangleShape(state->renderer, bbox, 0);
            }

            if (edit->is_editing) {
                v2 dim = (max - min);

                sfRectangleShape_setOrigin(bbox, V2(0, 0));
                sfRectangleShape_setPosition(bbox, min);
                sfRectangleShape_setSize(bbox, dim);
                sfRectangleShape_setOutlineThickness(bbox, 2);

                sfRenderWindow_drawRectangleShape(state->renderer, bbox, 0);
            }

            sfRectangleShape_destroy(bbox);
        }
        break;
        case EditMode_Entity: {
            Edit_Segment *current = edit->current_segment;

            if (!current) { break; }

            if (JustPressed(input->debug_up)) {
                edit->scale += V2(0.05, 0.05);
            }
            else if (JustPressed(input->debug_down)) {
                edit->scale -= V2(0.05, 0.05);
            }

            if (JustPressed(input->debug_next)) {
                edit->entity_type += 1;
                if (edit->entity_type >= EntityType_Count) { edit->entity_type = 0; }

                printf("Type is %d\n", edit->entity_type);
            }
            else if (JustPressed(input->debug_prev)) {
                edit->entity_type -= 1;
                if (edit->entity_type < 0) { edit->entity_type = EntityType_Count - 1; }
            }

            if (JustPressed(input->mouse_buttons[0])) {
                edit->selected = -1;
                for (u32 it = 0; it < current->entity_count; ++it) {
                    Entity *e = &current->entities[it];

                    Bounding_Box box = CreateBox(e->position, e->half_dim);
                    if (Contains(&box, input->mouse_position)) {
                        edit->selected = it;
                        break;
                    }
                }

                if (edit->selected == -1) {
                    Entity *next;
                    if (edit->entity_type == EntityType_Player) {
                        next = &edit->player;
                        edit->player_placed = true;
                    }
                    else {
                        next = &current->entities[current->entity_count];
                        current->entity_count += 1;
                    }

                     char name[256];
                    snprintf(name, sizeof(name), "Entity%02d", edit->entity_type);
                    Asset *texture = GetAsset(&state->assets, name);

                    // @Todo: Set hitboxes properly
                    sfVector2u sizeu = sfTexture_getSize(texture->texture);
                    v2 size = V2(edit->scale.x * sizeu.x, edit->scale.y * sizeu.y);

                    next->type     = edit->entity_type;
                    next->position = input->mouse_position;
                    next->scale    = edit->scale;
                    next->half_dim = 0.5f * size;

                    if (next->type >= EntityType_Player) {
                        switch (next->type) {
                            case EntityType_Player: {
                                next->animation = CreatePlayerAnimation(&state->assets);
                            }
                            break;
                            case EntityType_Torch: {
                                next->animation = CreateTorchAnimation(&state->assets);
                            }
                            break;
                            case EntityType_Spirit: {
                                next->animation = CreateSpiritAnimation(&state->assets);
                            }
                            break;
                            case EntityType_Tentacle: {
                                next->animation = CreateTentacleAnimation(&state->assets);
                            }
                            break;
                        }

                        next->half_dim  = 0.5f * next->animation.frame_size;
                        next->half_dim.x *= next->animation.scale.x;
                        next->half_dim.y *= next->animation.scale.y;

                        next->scale = next->animation.scale;
                    }

                    edit->scale = V2(1, 1);

                    if (next->type == EntityType_Spirit || next->type == EntityType_Raghead) {
                        edit->mode = EditMode_Pathing;
                        edit->pathing_entity = next;

                        next->path_count = 1;
                        next->path_points[0] = next->position;
                    }
                }
                else {
                }
            }

            sfSprite *sprite = sfSprite_create();

            for (u32 it = 0; it < current->entity_count; ++it) {
                Entity *entity = &current->entities[it];

                if (entity->type < EntityType_Player) {
                    char name[256];
                    snprintf(name, sizeof(name), "Entity%02d", entity->type);
                    Asset *texture = GetAsset(&state->assets, name);

                    sfVector2u sizeu = sfTexture_getSize(texture->texture);
                    v2 size = V2(sizeu.x, sizeu.y);

                    sfSprite_setTexture(sprite, texture->texture, true);
                    sfSprite_setOrigin(sprite, 0.5f * size);
                    sfSprite_setScale(sprite, entity->scale);
                    sfSprite_setPosition(sprite, entity->position);

                    sfRenderWindow_drawSprite(state->renderer, sprite, 0);
                }
                else {
                    UpdateRenderAnimation(state, &entity->animation, entity->position, input->delta_time);
                }

#if 1
                sfRectangleShape *bbox = sfRectangleShape_create();
                sfRectangleShape_setFillColor(bbox, sfTransparent);
                sfRectangleShape_setOutlineColor(bbox, sfRed);
                sfRectangleShape_setOutlineThickness(bbox, 1.5);
                sfRectangleShape_setOrigin(bbox, entity->half_dim);
                sfRectangleShape_setPosition(bbox, entity->position);
                sfRectangleShape_setSize(bbox, 2 * entity->half_dim);

                sfRenderWindow_drawRectangleShape(state->renderer, bbox, 0);
                sfRectangleShape_destroy(bbox);
#endif
            }

            if (edit->player_placed) {
                Animation a = CreatePlayerAnimation(&state->assets);
                UpdateRenderAnimation(state, &a, edit->player.position, 0);

                sfRectangleShape *bbox = sfRectangleShape_create();
                sfRectangleShape_setFillColor(bbox, sfTransparent);
                sfRectangleShape_setOutlineColor(bbox, sfRed);
                sfRectangleShape_setOutlineThickness(bbox, 1.5);
                sfRectangleShape_setOrigin(bbox, edit->player.half_dim);
                sfRectangleShape_setPosition(bbox, edit->player.position);
                sfRectangleShape_setSize(bbox, 2 * edit->player.half_dim);

                sfRenderWindow_drawRectangleShape(state->renderer, bbox, 0);
                sfRectangleShape_destroy(bbox);
            }

            if (edit->entity_type < EntityType_Player) {
                char name[256];
                snprintf(name, sizeof(name), "Entity%02d", edit->entity_type);
                Asset *texture = GetAsset(&state->assets, name);


                sfVector2u sizeu = sfTexture_getSize(texture->texture);
                v2 size = V2(sizeu.x, sizeu.y);

                sfSprite_setTexture(sprite, texture->texture, true);
                sfSprite_setOrigin(sprite, 0.5f * size);
                sfSprite_setScale(sprite, edit->scale);
                sfSprite_setPosition(sprite, input->mouse_position);

                sfRenderWindow_drawSprite(state->renderer, sprite, 0);
            }
            else {
                switch (edit->entity_type) {
                    case EntityType_Player: {
                        Animation a = CreatePlayerAnimation(&state->assets);
                        UpdateRenderAnimation(state, &a, input->mouse_position, 0);
                    }
                    break;
                    case EntityType_Torch: {
                        Animation a = CreateTorchAnimation(&state->assets);
                        UpdateRenderAnimation(state, &a, input->mouse_position, 0);
                    }
                    break;
                    case EntityType_Spirit: {
                        Animation a = CreateSpiritAnimation(&state->assets);
                        UpdateRenderAnimation(state, &a, input->mouse_position, 0);
                    }
                    break;
                    case EntityType_Tentacle: {
                        Animation a = CreateTentacleAnimation(&state->assets);
                        UpdateRenderAnimation(state, &a, input->mouse_position, 0);
                    }
                    break;
                }
            }

            sfSprite_destroy(sprite);
        }
        break;

        case EditMode_Pathing: {
            Entity *entity = edit->pathing_entity;

            if (JustPressed(input->mouse_buttons[0])) {
                entity->path_points[entity->path_count] = input->mouse_position;
                entity->path_count += 1;

                if (entity->path_count == 6) {
                    edit->mode = EditMode_Entity;
                    edit->pathing_entity = 0;
                }
            }

            if (JustPressed(input->f[3])) {
                edit->mode = EditMode_Entity;
                edit->pathing_entity = 0;
            }

            if (entity->type == EntityType_Spirit) {
                Animation a = CreateSpiritAnimation(&state->assets);
                UpdateRenderAnimation(state, &a, entity->position, 0);
            }
            else {
                sfSprite *sprite = sfSprite_create();
                char name[256];
                snprintf(name, sizeof(name), "Entity%02d", edit->entity_type);
                Asset *texture = GetAsset(&state->assets, name);


                sfVector2u sizeu = sfTexture_getSize(texture->texture);
                v2 size = V2(sizeu.x, sizeu.y);

                sfSprite_setTexture(sprite, texture->texture, true);
                sfSprite_setOrigin(sprite, 0.5f * size);
                sfSprite_setScale(sprite, edit->scale);
                sfSprite_setPosition(sprite, input->mouse_position);

                sfRenderWindow_drawSprite(state->renderer, sprite, 0);

                sfSprite_destroy(sprite);
            }

            for (u32 it = 0; it < entity->path_count - 1; ++it) {
                sfVertex v[2] = {};
                v[0].position = entity->path_points[it];
                v[0].color    = sfGreen;

                v[1].position = entity->path_points[it + 1];
                v[1].color    = sfGreen;

                sfRenderWindow_drawPrimitives(state->renderer, v, 2, sfLines, 0);
            }

            sfVertex v[2] = {};
            v[0].position = entity->path_points[entity->path_count - 1];
            v[0].color    = sfGreen;

            v[1].position = input->mouse_position;
            v[1].color    = sfRed;

            sfRenderWindow_drawPrimitives(state->renderer, v, 2, sfLines, 0);
        }
        break;
    }
}

#if 0
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
                sfSprite_setPosition(sprite, input->mouse_position);

                sfRenderWindow_drawSprite(state->renderer, sprite, 0);

                sfSprite_destroy(sprite);
            }
        }
        break;
    }
}
#endif


