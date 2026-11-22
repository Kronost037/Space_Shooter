#include "flight.h"

float distanceToSegment(Vector2 point, Vector2 start, Vector2 end) {
    Vector2 span = Vector2Subtract(end, start);
    float lengthSquared = Vector2LengthSqr(span);
    if (lengthSquared < 0.0001f) return Vector2Distance(point, start);
    float t = Clamp(Vector2DotProduct(Vector2Subtract(point, start), span) / lengthSquared,
                    0.0f, 1.0f);
    return Vector2Distance(point, Vector2Add(start, Vector2Scale(span, t)));
}

bool onScreen(Camera2D camera, Vector2 point, float margin) {
    Vector2 screen = GetWorldToScreen2D(point, camera);
    return screen.x >= margin && screen.x <= SCREEN_WIDTH - margin &&
           screen.y >= margin && screen.y <= SCREEN_HEIGHT - margin;
}
