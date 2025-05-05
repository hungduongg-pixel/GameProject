
struct Camera {
    float x;
    float y;
    int mapWidthPixels;
    bool isShaking;       // Trạng thái rung
    int shakeTimer;       // Thời gian rung
    int shakeIntensity;   // Độ mạnh của rung

    void init(int mapWidth) {
        x = 0;
        y = 0;
        mapWidthPixels = mapWidth * TILE_WIDTH;
        isShaking = false;
        shakeTimer = 0;
        shakeIntensity = 0;
    }

    void update(float playerX) {
        x = playerX - SCREEN_WIDTH / 2;
        if (x < 0) x = 0;
        if (x > mapWidthPixels - SCREEN_WIDTH) x = mapWidthPixels - SCREEN_WIDTH;

        if (isShaking && shakeTimer > 0) {
            float shakeProgress = static_cast<float>(shakeTimer) / 10.0f;
            x += sin(SDL_GetTicks() * 0.1f) * shakeIntensity * shakeProgress;
            y += cos(SDL_GetTicks() * 0.1f) * shakeIntensity * shakeProgress;
            shakeTimer--;
            if (shakeTimer <= 0) {
                isShaking = false;
            }
        }
    }

    void startShake(int intensity, int duration) {
        isShaking = true;
        shakeIntensity = intensity;
        shakeTimer = duration;
    }
};
