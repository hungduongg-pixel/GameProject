struct NewEnemy5 {
    float x, y;
    float min_x, max_x;
    SDL_Texture* idleTexture;
    SDL_Texture* moveTexture;
    int currentFrame;
    int idleFrameCount;
    int moveFrameCount;
    int frameDelay;
    int frameTimer;
    bool isMoving;
    bool toRemove;
    bool isHit;
    bool isEndScreen;
    SDL_Texture* endScreenTexture;
    SDL_Texture* endTextTexture;
    SDL_Texture* playerIdleTexture;
    SDL_Texture* helloTextTexture;
    SDL_Rect endTextRect;
    SDL_Rect playerRect;
    SDL_Rect helloTextRect;

    void init(SDL_Renderer* renderer, float start_x, float start_y, float min_x, float max_x) {
        x = start_x;
        y = start_y;
        this->min_x = min_x;
        this->max_x = max_x;

        std::string idlePath = "assets/newenemy5_idle_sheet.png";
        SDL_Surface* idleSurface = IMG_Load(idlePath.c_str());
        idleTexture = idleSurface ? SDL_CreateTextureFromSurface(renderer, idleSurface) : nullptr;
        if (!idleSurface) std::cerr << "❌ Không tải được " << idlePath << ": " << IMG_GetError() << std::endl;
        if (idleSurface) SDL_FreeSurface(idleSurface);

        std::string movePath = "assets/newenemy5_move_sheet.png";
        SDL_Surface* moveSurface = IMG_Load(movePath.c_str());
        moveTexture = moveSurface ? SDL_CreateTextureFromSurface(renderer, moveSurface) : nullptr;
        if (!moveSurface) std::cerr << "❌ Không tải được " << movePath << ": " << IMG_GetError() << std::endl;
        if (moveSurface) SDL_FreeSurface(moveSurface);

        endScreenTexture = IMG_LoadTexture(renderer, "assets/menu.png");
        if (!endScreenTexture) std::cerr << "❌ Không tải được menu.png: " << IMG_GetError() << std::endl;

        playerIdleTexture = IMG_LoadTexture(renderer, "assets/Idle-Sheet1.png");
        if (!playerIdleTexture) std::cerr << "❌ Không tải được Idle-Sheet1.png: " << IMG_GetError() << std::endl;

        TTF_Font* endFont = TTF_OpenFont("assets/Legacy.ttf", 30);
        if (!endFont) std::cerr << "❌ Không tải được Legacy.ttf: " << TTF_GetError() << std::endl;

        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Surface* endSurface = TTF_RenderUTF8_Blended(endFont, "YOU WIN", yellow);
        endTextTexture = endSurface ? SDL_CreateTextureFromSurface(renderer, endSurface) : nullptr;
        endTextRect = endSurface ? SDL_Rect{(SCREEN_WIDTH - endSurface->w) / 2, 0, endSurface->w, endSurface->h} : SDL_Rect{0, 0, 0, 0};
        if (endSurface) SDL_FreeSurface(endSurface);

        SDL_Surface* helloSurface = TTF_RenderUTF8_Blended(endFont, "CONGRUTULATIONS!", yellow);
        helloTextTexture = helloSurface ? SDL_CreateTextureFromSurface(renderer, helloSurface) : nullptr;
        helloTextRect = helloSurface ? SDL_Rect{0, 0, helloSurface->w, helloSurface->h} : SDL_Rect{0, 0, 0, 0};
        if (helloSurface) SDL_FreeSurface(helloSurface);

        if (endFont) TTF_CloseFont(endFont);

        playerRect = { (SCREEN_WIDTH - 85) / 2, (SCREEN_HEIGHT - 85) / 2, 85, 85 };

        currentFrame = 0;
        idleFrameCount = 10;
        moveFrameCount = 6;
        frameDelay = 8;
        frameTimer = 0;
        isMoving = false;
        toRemove = false;
        isHit = false;
        isEndScreen = false;
    }

    void update() {
        if (isHit || isEndScreen) {
            frameTimer++;
            if (frameTimer >= frameDelay) {
                frameTimer = 0;
                currentFrame = (currentFrame + 1) % 4;
            }
            return;
        }

        frameTimer++;
        if (frameTimer >= frameDelay) {
            frameTimer = 0;
            if (isMoving) {
                currentFrame = (currentFrame + 1) % moveFrameCount;
            } else {
                currentFrame = (currentFrame + 1) % idleFrameCount;
            }
            if (currentFrame == 0 && rand() % 10 < 3) {
                isMoving = !isMoving;
            }
        }
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        if (isEndScreen) {
            SDL_RenderCopy(renderer, endScreenTexture, NULL, NULL);
            SDL_Rect srcRect = { (currentFrame % 4) * 64, 0, 64, 64 };
            SDL_RenderCopyEx(renderer, playerIdleTexture, &srcRect, &playerRect, 0, NULL, SDL_FLIP_NONE);
            if (helloTextTexture) {
                helloTextRect.x = playerRect.x + (playerRect.w - helloTextRect.w) / 2;
                helloTextRect.y = playerRect.y - helloTextRect.h - 10;
                SDL_RenderCopy(renderer, helloTextTexture, NULL, &helloTextRect);
            }
            endTextRect.y = playerRect.y + playerRect.h + 10;
            if (endTextTexture) SDL_RenderCopy(renderer, endTextTexture, NULL, &endTextRect);
        } else if (!isHit) {
            SDL_Rect dstRect = { static_cast<int>(x - cameraX), static_cast<int>(y), 64, 64 };
            if (dstRect.x + dstRect.w > 0 && dstRect.x < SCREEN_WIDTH) {
                SDL_Texture* currentTexture = isMoving ? moveTexture : idleTexture;
                int frameWidth = isMoving ? 48 : 48;
                int frameHeight = isMoving ? 33 : 35;
                SDL_Rect srcRect = { currentFrame * frameWidth, 0, frameWidth, frameHeight };
                if (currentTexture) {
                    SDL_RenderCopy(renderer, currentTexture, &srcRect, &dstRect);
                } else {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                    SDL_RenderFillRect(renderer, &dstRect);
                }
            }
        }
    }

    void hit() {
        isHit = true;
        isEndScreen = true;
        currentFrame = 0;
    }

    void cleanup() {
        if (idleTexture) SDL_DestroyTexture(idleTexture);
        if (moveTexture) SDL_DestroyTexture(moveTexture);
        if (endScreenTexture) SDL_DestroyTexture(endScreenTexture);
        if (endTextTexture) SDL_DestroyTexture(endTextTexture);
        if (playerIdleTexture) SDL_DestroyTexture(playerIdleTexture);
        if (helloTextTexture) SDL_DestroyTexture(helloTextTexture);
    }
};
