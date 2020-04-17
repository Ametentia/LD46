#if !defined(LUDUM_MATHS_H_)
#define LUDUM_MATHS_H_ 1

internal v2 V2(f32 x, f32 y) {
    v2 result = { x, y };
    return result;
}

inline v2 operator+(v2 a, v2 b) {
    v2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

internal v2 &operator+=(v2 &a, v2 b) {
    a = a + b;
    return a;
}

internal v2 operator-(v2 a, v2 b) {
    v2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
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

#endif  // LUDUM_MATHS_H_
