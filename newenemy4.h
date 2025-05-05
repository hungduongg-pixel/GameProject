struct NewEnemy {
    float x, y;
    float speed;
    bool movingRight;
    float min_x, max_x;
    SDL_Texture* moveTexture;
    SDL_Texture* attackTexture;
    SDL_Texture* dyingTexture;
    SDL_Texture* hurtTexture;
    SDL_Texture* idleTexture;
    int currentFrame;
    int moveFrameCount;
    int attackFrameCount;
    int dyingFrameCount;
    int hurtFrameCount;
    int idleFrameCount;
    int frameDelay;
    int frameTimer;
    bool isAttacking;
    bool isDying;
    bool toRemove;
    bool isHurt;
    bool isIdle;
    int hitCount;
    int attackCooldown;
    int attackCooldownMax;

    void init(SDL_Renderer* renderer, float start_x, float start_y, float min_x, float max_x) {
        x = start_x;
        y = start_y - 10.0f;
        speed = 1.2f;
        movingRight = true;
        this->min_x = min_x;
        this->max_x = max_x;

        std::string movePath = ASSETS_PATH + "newenemy_move_sheet.png";
        SDL_Surface* moveSurface = IMG_Load(movePath.c_str());
        moveTexture = moveSurface ? SDL_CreateTextureFromSurface(renderer, moveSurface) : nullptr;
        if (!moveSurface) std::cerr << "❌ Không tải được " << movePath << ": " << IMG_GetError() << std::endl;
        if (moveSurface) SDL_FreeSurface(moveSurface);

        std::string attackPath = ASSETS_PATH + "newenemy_attack_sheet.png";
        SDL_Surface* attackSurface = IMG_Load(attackPath.c_str());
        attackTexture = attackSurface ? SDL_CreateTextureFromSurface(renderer, attackSurface) : nullptr;
        if (!attackSurface) std::cerr << "❌ Không tải được " << attackPath << ": " << IMG_GetError() << std::endl;
        if (attackSurface) SDL_FreeSurface(attackSurface);

        std::string dyingPath = ASSETS_PATH + "newenemy_dying_sheet.png";
        SDL_Surface* dyingSurface = IMG_Load(dyingPath.c_str());
        dyingTexture = dyingSurface ? SDL_CreateTextureFromSurface(renderer, dyingSurface) : nullptr;
        if (!dyingSurface) std::cerr << "❌ Không tải được " << dyingPath << ": " << IMG_GetError() << std::endl;
        if (dyingSurface) SDL_FreeSurface(dyingSurface);

        std::string hurtPath = ASSETS_PATH + "newenemy_hurt_sheet.png";
        SDL_Surface* hurtSurface = IMG_Load(hurtPath.c_str());
        hurtTexture = hurtSurface ? SDL_CreateTextureFromSurface(renderer, hurtSurface) : nullptr;
        if (!hurtSurface) std::cerr << "❌ Không tải được " << hurtPath << ": " << IMG_GetError() << std::endl;
        if (hurtSurface) SDL_FreeSurface(hurtSurface);

        std::string idlePath = ASSETS_PATH + "newenemy_Idle_sheet.png";
        SDL_Surface* idleSurface = IMG_Load(idlePath.c_str());
        idleTexture = idleSurface ? SDL_CreateTextureFromSurface(renderer, idleSurface) : nullptr;
        if (!idleSurface) std::cerr << "❌ Không tải được " << idlePath << ": " << IMG_GetError() << std::endl;
        if (idleSurface) SDL_FreeSurface(idleSurface);

        currentFrame = 0;
        moveFrameCount = 10;
        attackFrameCount = 11;
        dyingFrameCount = 12;
        hurtFrameCount = 4;
        idleFrameCount = 8;
        frameDelay = 8;
        frameTimer = 0;
        isAttacking = false;
        isDying = false;
        toRemove = false;
        isHurt = false;
        isIdle = true;
        hitCount = 0;
        attackCooldown = 0;
        attackCooldownMax = 60;
    }

    void update(float playerX, float playerY) {
        if (isDying) {
            frameTimer++;
            if (frameTimer >= frameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= dyingFrameCount) {
                    toRemove = true;
                }
            }
        } else if (isHurt) {
            frameTimer++;
            if (frameTimer >= frameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= hurtFrameCount) {
                    isHurt = false;
                    currentFrame = 0;
                    isIdle = true;
                }
            }
        } else {
            float distance = std::abs(playerX - x);
            if (!movingRight && distance < 100 && !isAttacking) {
                isAttacking = true;
                currentFrame = 0;
                isIdle = false;
            }

            if (isAttacking) {
                frameTimer++;
                if (frameTimer >= frameDelay) {
                    frameTimer = 0;
                    currentFrame++;
                    if (currentFrame >= attackFrameCount) {
                        isAttacking = false;
                        currentFrame = 0;
                        isIdle = true;
                    }
                }
            } else {
                if (isIdle) {
                    if (movingRight) {
                        x += speed;
                        if (x >= max_x) {
                            x = max_x;
                            movingRight = false;
                        }
                    } else {
                        x -= speed;
                        if (x <= min_x) {
                            x = min_x;
                            movingRight = true;
                        }
                    }
                    frameTimer++;
                    if (frameTimer >= frameDelay) {
                        frameTimer = 0;
                        currentFrame = (currentFrame + 1) % moveFrameCount;
                    }
                }
            }
        }

        if (attackCooldown > 0) attackCooldown--;
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        SDL_Rect dstRect = { static_cast<int>(x - cameraX), static_cast<int>(y), 110, 110 };
        if (dstRect.x + dstRect.w > 0 && dstRect.x < SCREEN_WIDTH) {
            SDL_Texture* currentTexture;
            int frameWidth, frameHeight;

            if (isDying) {
                currentTexture = dyingTexture;
                frameWidth = 90;
                frameHeight = 64;
            } else if (isHurt) {
                currentTexture = hurtTexture;
                frameWidth = 90;
                frameHeight = 64;
            } else if (isAttacking) {
                currentTexture = attackTexture;
                frameWidth = 90;
                frameHeight = 64;
            } else {
                currentTexture = isIdle ? moveTexture : idleTexture;
                frameWidth = 90;
                frameHeight = 64;
            }

            SDL_Rect srcRect = { currentFrame * frameWidth, 0, frameWidth, frameHeight };
            SDL_RendererFlip flip = (isAttacking && !movingRight) ? SDL_FLIP_HORIZONTAL : (movingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            if (currentTexture) SDL_RenderCopyEx(renderer, currentTexture, &srcRect, &dstRect, 0, NULL, flip);
        }
    }

    void cleanup() {
        if (moveTexture) SDL_DestroyTexture(moveTexture);
        if (attackTexture) SDL_DestroyTexture(attackTexture);
        if (dyingTexture) SDL_DestroyTexture(dyingTexture);
        if (hurtTexture) SDL_DestroyTexture(hurtTexture);
        if (idleTexture) SDL_DestroyTexture(idleTexture);
    }
};
