internal Animation CreateAnimationFromTexture(sfTexture *texture, v2 scale, u32 rows, u32 columns,
        f32 time_per_frame = 0.1f)
{
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

internal void UpdateRenderAnimation(Game_State *state, Animation *animation, v2 position, f32 delta_time) {
    if (!animation->pause) { animation->accumulator += delta_time; }

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
    if (animation->flip) { scale.x *= -1; }

    sfSprite_setScale(sprite, scale);

    sfRenderWindow_drawSprite(state->renderer, sprite, 0);

    sfSprite_destroy(sprite);
}

internal Animation CreatePlayerAnimation(Asset_Manager *assets) {
    Animation result;

    char name[256];
    snprintf(name, sizeof(name), "Entity%02d", EntityType_Player);
    Asset *asset = GetAsset(assets, name);
    Assert(asset->type == Asset_Texture);

    result = CreateAnimationFromTexture(asset->texture, V2(0.16, 0.16), 2, 3, 0.09f);
    return result;
}

internal Animation CreateTorchAnimation(Asset_Manager *assets) {
    Animation result;

    char name[256];
    snprintf(name, sizeof(name), "Entity%02d", EntityType_Torch);
    Asset *asset = GetAsset(assets, name);
    Assert(asset->type == Asset_Texture);

    result = CreateAnimationFromTexture(asset->texture, V2(0.6, 0.6), 4, 4, 0.13f);
    return result;
}

internal Animation CreateWindAnimation(Asset_Manager *assets) {
    Animation result;

    char name[256];
    snprintf(name, sizeof(name), "Entity%02d", EntityType_Wind);
    Asset *asset = GetAsset(assets, name);
    Assert(asset->type == Asset_Texture);

    result = CreateAnimationFromTexture(asset->texture, V2(1, 1), 2, 3, 0.15f);
    return result;
}


