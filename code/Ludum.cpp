#include "Ludum_Assets.cpp"

internal Animation CreateAnimationFromTexture(sfTexture *texture, v2 size, u32 rows, u32 columns) {
    Animation result;
    result.texture = texture;

    result.size = size;

    v2 tex_size = V2(sfTexture_getSize(texture));
    result.frame_size.x = tex_size.x / cast(f32) columns;
    result.frame_size.y = tex_size.y / cast(f32) rows;

    result.rows = rows;
    result.columns = columns;

    result.total_frames = (rows * columns);

    result.time_per_frame = 0.1f; // @Todo: make this variable?
    result.current_frame  = 0;
    result.accumulator    = 0;
    result.flip           = false;

    return result;
}

internal void UpdateRenderAnimation(Game_State *state, Animation *animation, v2 position, f32 delta_time) {
    animation->accumulator += delta_time;
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
    sfSprite_setOrigin(sprite, 0.5f * animation->size);
    sfSprite_setPosition(sprite, position);

    sfRenderWindow_drawSprite(state->renderer, sprite, 0);

    sfSprite_destroy(sprite);
}

internal void LudumUpdateRender(Game_State *state, Game_Input *input) {
    if (!state->initialised) {
        state->player_pos = V2(250, 400);
        InitAssets(&state->assets, 64);

        LoadAsset(&state->assets, "PlayerMove", Asset_Texture);
        Asset *anim = GetAsset(&state->assets, "PlayerMove");
        Assert(anim);

        state->debug_anim = CreateAnimationFromTexture(anim->texture, V2(60, 120), 1, 5);

        state->initialised = true;
    }
}
