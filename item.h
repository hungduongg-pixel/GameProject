struct Item {
    SDL_Rect rect;
    SDL_Texture* texture;
    bool isCollected;

    void init(SDL_Renderer* renderer, int x, int y) {
        int offsetX = 6;
        int offsetY = 15;
        rect = { x + offsetX, y + offsetY, 40, 40 };
        isCollected = false;
        texture = IMG_LoadTexture(renderer, (ASSETS_PATH + "item.png").c_str());
        if (!texture) {
            std::cerr << "❌ Không tải được item.png: " << IMG_GetError() << std::endl;
        }
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        if (isCollected) return;
        SDL_Rect renderRect = rect;
        renderRect.x -= static_cast<int>(cameraX);
        if (renderRect.x + renderRect.w > 0 && renderRect.x < SCREEN_WIDTH) {
            if (texture) {
                SDL_RenderCopy(renderer, texture, NULL, &renderRect);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderFillRect(renderer, &renderRect);
            }
        }
    }

    void cleanup() {
        if (texture) {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
};
