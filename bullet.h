struct Bullet {
    float x, y;
    float velocityX;
    bool toRemove;
    int width, height;
    SDL_Texture* bulletTexture;

    void init(SDL_Renderer* renderer, float startX, float startY, bool movingRight) {
        x = startX + (movingRight ? 64 : -48);
        y = startY + 16;
        velocityX = movingRight ? 5.0f : -5.0f;
        toRemove = false;
        width = 36;
        height = 18;

        bulletTexture = IMG_LoadTexture(renderer, "bullet.png");
        if (!bulletTexture) {
            std::cerr << "❌ Không tải được bullet.png: " << IMG_GetError() << std::endl;
        }
        if (debugBullet) {
            std::cout << "Bullet initialized at x: " << x << ", y: " << y << std::endl;
        }
    }

    void update() {
        x += velocityX;
        if (x < -width || x > MAP_WIDTH * TILE_WIDTH + width) {
            toRemove = true;
        }
        if (debugBullet && toRemove) {
            std::cout << "Bullet removed at x: " << x << ", y: " << y << std::endl;
        }
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        int renderX = static_cast<int>(x - cameraX);
        SDL_Rect dstRect = { renderX, static_cast<int>(y), width, height };
        if (debugBullet && renderX + width > 0 && renderX < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
            std::cout << "Bullet rendered at x: " << renderX << ", y: " << y << std::endl;
        }
        if (bulletTexture) {
            SDL_RenderCopy(renderer, bulletTexture, NULL, &dstRect);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &dstRect);
        }
    }

    void cleanup() {
        if (bulletTexture) {
            SDL_DestroyTexture(bulletTexture);
            bulletTexture = nullptr;
        }
    }
};
