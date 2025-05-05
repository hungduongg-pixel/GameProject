struct Transition {
    bool isTransitioning;
    int fadeAlpha;
    int fadeSpeed;
    int targetLevel;

    void init() {
        isTransitioning = false;
        fadeAlpha = 0;
        fadeSpeed = 5;
        targetLevel = 0;
    }

    void start(int level) {
        isTransitioning = true;
        fadeAlpha = 0;
        targetLevel = level;
    }

    bool update() {
        if (!isTransitioning) return false;
        fadeAlpha += fadeSpeed;
        if (fadeAlpha >= 255) {
            fadeAlpha = 255;
            isTransitioning = false;
            return true;
        }
        return false;
    }

    void render(SDL_Renderer* renderer) {
        if (isTransitioning) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, fadeAlpha);
            SDL_Rect rect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
};
