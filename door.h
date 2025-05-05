struct Door {
    SDL_Rect rect;
    SDL_Texture* texture;

    void init(SDL_Renderer* renderer, int x, int y, TextureManager& textureManager) {
        rect = { x, y, TILE_WIDTH, TILE_HEIGHT };
        texture = textureManager.doorTexture;
        if (!texture) std::cerr << "❌ Texture cửa không hợp lệ" << std::endl;
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        SDL_Rect renderRect = rect;
        renderRect.x -= static_cast<int>(cameraX);
        if (renderRect.x + renderRect.w > 0 && renderRect.x < SCREEN_WIDTH) {
            if (texture) {
                SDL_RenderCopy(renderer, texture, NULL, &renderRect);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderFillRect(renderer, &renderRect);
            }
        }
    }

    void cleanup() {}
};
