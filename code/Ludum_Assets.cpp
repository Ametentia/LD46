internal void InitAssets(Asset_Manager *assets, u32 table_size) {
    assets->hash_table_size = table_size;
    assets->assets = cast(Asset *) malloc(assets->hash_table_size * sizeof(Asset));
    ClearSize(assets->assets, assets->hash_table_size * sizeof(Asset));
}

internal u32 HashAssetName(const char *name) {
    u32 result = 5381;

    u32 it = 0;
    while (name[it]) {
        result = ((result << 5) + result) + name[it];
        it += 1;
    }

    return result;
}

internal b32 LoadAsset(Asset_Manager *assets, const char *name, Asset_Type type, u32 flags = 0) {
    b32 result = false;

    u32 hash_index = HashAssetName(name);
    hash_index %= assets->hash_table_size;

    Asset *slot = &assets->assets[hash_index];
    if (slot->occupied) {
        slot = slot->next;
        while (slot) {
            if (!slot->occupied) { break; }
            slot = slot->next;
        }

        if (!slot) {
            slot = cast(Asset *) malloc(sizeof(Asset));
            slot->next = assets->assets[hash_index].next;
            assets->assets[hash_index].next = slot;
        }
    }

    Assert(slot);

    slot->type = type;
    slot->name = name;

    slot->flags = flags;

    char buf[1024];

    switch (type) {
        case Asset_Texture: {
            snprintf(buf, sizeof(buf), "%s.png", name);
            slot->texture = sfTexture_createFromFile(buf, 0);
        }
        break;
        case Asset_Sound: {
            snprintf(buf, sizeof(buf), "%s.wav", name);
            slot->sound = sfSoundBuffer_createFromFile(buf);
        }
        break;
        case Asset_Music: {
            snprintf(buf, sizeof(buf), "%s.wav", name);
            slot->music = sfMusic_createFromFile(buf);
        }
        break;
        case Asset_Font: {
            snprintf(buf, sizeof(buf), "%s.ttf", name);
            slot->font = sfFont_createFromFile(buf);
        }
        break;
    }

    result = (slot->__checker != 0);
    slot->occupied = result;

    return result;
}

internal Asset *GetAsset(Asset_Manager *assets, const char *name) {
    Asset *result = 0;

    u32 hash_index = HashAssetName(name) % assets->hash_table_size;
    result = &assets->assets[hash_index];

    while (result && !StringsEqual(result->name, name)) { result = result->next; }

    // @Note: May be 0
    return result;
}


