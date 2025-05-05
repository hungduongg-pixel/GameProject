struct TextureManager {
    SDL_Texture* map1Texture;
    SDL_Texture* map2Texture;
    SDL_Texture* doorTexture;

    void init(SDL_Renderer* renderer) {
        map1Texture = IMG_LoadTexture(renderer, (ASSETS_PATH + "map1.png").c_str());
        if (!map1Texture) std::cerr << "❌ Không tải được map1.png: " << IMG_GetError() << std::endl;
        map2Texture = IMG_LoadTexture(renderer, (ASSETS_PATH + "map2.png").c_str());
        if (!map2Texture) std::cerr << "❌ Không tải được map2.png: " << IMG_GetError() << std::endl;
        doorTexture = IMG_LoadTexture(renderer, (ASSETS_PATH + "door.png").c_str());
        if (!doorTexture) std::cerr << "❌ Không tải được door.png: " << IMG_GetError() << std::endl;
    }

    void cleanup() {
        if (map1Texture) SDL_DestroyTexture(map1Texture);
        if (map2Texture) SDL_DestroyTexture(map2Texture);
        if (doorTexture) SDL_DestroyTexture(doorTexture);
    }
};
