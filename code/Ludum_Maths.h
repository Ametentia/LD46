#if !defined(LUDUM_MATHS_H_)
#define LUDUM_MATHS_H_ 1

#define Clamp(x, min, max) (Min(Max(x, min), max))

internal v2 V2(f32 x, f32 y) {
    v2 result = { x, y };
    return result;
}

inline s32 random(s32 min, s32 max) {
    if(min == max) { return min; }
	return (rand() % (min - max)) + min;
}

inline v2 random(v2 min, v2 max) {
    f32 choice_x = random(min.x, max.x);
    f32 choice_y = random(min.y, max.y);
    v2 choice = V2(choice_x, choice_y);
	return choice;
}

internal v2 V2(sfVector2u v) {
    v2 result;
    result.x = cast(f32) v.x;
    result.y = cast(f32) v.y;

    return result;
}

inline v2 operator+(v2 a, v2 b) {
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal v2 operator-(v2 a, v2 b) {
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

inline f32 length(v2 a, v2 b) {
    v2 result = a - b;
    result.x = result.x * result.x;
    result.y = result.y * result.y;

    return result.x + result.y;
}

inline f32 length(v2 a) {
    a.x = a.x * a.x;
    a.y = a.y * a.y;

    return a.x + a.y;
}

internal v2 &operator+=(v2 &a, v2 b) {
    a = a + b;
    return a;
}

internal v2 operator-(v2 a) {
    v2 result;
    result.x = -a.x;
    result.y = -a.y;

    return result;
}

internal v2 &operator-=(v2 &a, v2 b) {
    a = a - b;
    return a;
}

internal v2 operator*(v2 a, f32 b) {
    v2 result;
    result.x = b * a.x;
    result.y = b * a.y;

    return result;
}

internal v2 operator*(f32 a, v2 b) {
    v2 result;
    result.x = a * b.x;
    result.y = a * b.y;

    return result;
}

internal v2 &operator*=(v2 &a, f32 b) {
    a = b * a;
    return a;
}


internal v3 V3(f32 x, f32 y, f32 z) {
    v3 result = { x, y, z };
    return result;
}

#endif  // LUDUM_MATHS_H_
