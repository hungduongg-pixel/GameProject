struct Boss {
    float x, y;
    float speed;
    bool movingRight;
    float min_x, max_x;
    std::vector<SDL_Texture*> moveTextures;
    std::vector<SDL_Texture*> attackTextures;
    std::vector<SDL_Texture*> dyingTextures;
    SDL_Texture* hurtTexture;
    SDL_Texture* idleTexture;
    int currentFrame;
    int moveFrameCount;
    int attackFrameCount;
    int dyingFrameCount;
    int hurtFrameCount;
    int idleFrameCount;
    int moveFrameDelay;
    int attackFrameDelay;
    int dyingFrameDelay;
    int hurtFrameDelay;
    int idleFrameDelay;
    int frameTimer;
    bool isAttacking;
    bool isDying;
    bool toRemove;
    bool isHurt;
    bool isIdle;
    bool isMoving;
    int hitCount;
    int attackCooldown;
    int attackCooldownMax;
    int currentMoveSheetIndex;
    int currentAttackSheetIndex;
    int currentDyingSheetIndex;
    static int lastAttackSheetIndex;
    static int lastDyingSheetIndex;
    float scale;
    int health;
    int maxHealth;
    bool shouldShake;
    int shakeDelayTimer;

    void init(SDL_Renderer* renderer, float start_x, float start_y, float min_x, float max_x) {
        float offset = 150.0f;
        x = start_x + offset;
        y = start_y - 10.0f;
        speed = 1.2f;
        movingRight = true;
        this->min_x = min_x + offset;
        this->max_x = max_x + offset;

        scale = 1.5f;

        moveTextures.resize(2, nullptr);
        for (int i = 0; i < 2; ++i) {
            std::string movePath = ASSETS_PATH + "boss_move" + std::to_string(i + 1) + "_sheet.png";
            SDL_Surface* moveSurface = IMG_Load(movePath.c_str());
            moveTextures[i] = moveSurface ? SDL_CreateTextureFromSurface(renderer, moveSurface) : nullptr;
            if (!moveSurface) std::cerr << "❌ Không tải được " << movePath << ": " << IMG_GetError() << std::endl;
            if (moveSurface) SDL_FreeSurface(moveSurface);
        }

        attackTextures.resize(3, nullptr);
        for (int i = 0; i < 3; ++i) {
            std::string attackPath = ASSETS_PATH + "boss_attack" + std::to_string(i + 1) + "_sheet.png";
            SDL_Surface* attackSurface = IMG_Load(attackPath.c_str());
            attackTextures[i] = attackSurface ? SDL_CreateTextureFromSurface(renderer, attackSurface) : nullptr;
            if (!attackSurface) std::cerr << "❌ Không tải được " << attackPath << ": " << IMG_GetError() << std::endl;
            if (attackSurface) SDL_FreeSurface(attackSurface);
        }

        dyingTextures.resize(3, nullptr);
        for (int i = 0; i < 3; ++i) {
            std::string dyingPath = ASSETS_PATH + "boss_dying" + std::to_string(i + 1) + "_sheet.png";
            SDL_Surface* dyingSurface = IMG_Load(dyingPath.c_str());
            dyingTextures[i] = dyingSurface ? SDL_CreateTextureFromSurface(renderer, dyingSurface) : nullptr;
            if (!dyingSurface) std::cerr << "❌ Không tải được " << dyingPath << ": " << IMG_GetError() << std::endl;
            if (dyingSurface) SDL_FreeSurface(dyingSurface);
        }

        std::string hurtPath = ASSETS_PATH + "boss_hurt_sheet.png";
        SDL_Surface* hurtSurface = IMG_Load(hurtPath.c_str());
        hurtTexture = hurtSurface ? SDL_CreateTextureFromSurface(renderer, hurtSurface) : nullptr;
        if (!hurtSurface) std::cerr << "❌ Không tải được " << hurtPath << ": " << IMG_GetError() << std::endl;
        if (hurtSurface) SDL_FreeSurface(hurtSurface);

        std::string idlePath = ASSETS_PATH + "boss_Idle_sheet.png";
        SDL_Surface* idleSurface = IMG_Load(idlePath.c_str());
        idleTexture = idleSurface ? SDL_CreateTextureFromSurface(renderer, idleSurface) : nullptr;
        if (!idleSurface) std::cerr << "❌ Không tải được " << idlePath << ": " << IMG_GetError() << std::endl;
        if (idleSurface) SDL_FreeSurface(idleSurface);

        currentFrame = 0;
        moveFrameCount = 6;
        attackFrameCount = 5;
        dyingFrameCount = 6;
        hurtFrameCount = 5;
        idleFrameCount = 6;
        moveFrameDelay = 8;
        attackFrameDelay = 8;
        dyingFrameDelay = 9;
        hurtFrameDelay = 8;
        idleFrameDelay = 8;
        frameTimer = 0;
        isAttacking = false;
        isDying = false;
        toRemove = false;
        isHurt = false;
        isIdle = true;
        isMoving = true;
        hitCount = 0;
        attackCooldown = 0;
        attackCooldownMax = 60;
        currentMoveSheetIndex = 0;
        currentAttackSheetIndex = (lastAttackSheetIndex + 1) % 3;
        lastAttackSheetIndex = currentAttackSheetIndex;
        currentDyingSheetIndex = (lastDyingSheetIndex + 1) % 3;
        lastDyingSheetIndex = currentDyingSheetIndex;

        health = 100;
        maxHealth = 100;
        shouldShake = false;
        shakeDelayTimer = 0;
    }

    void update(float playerX, float playerY, Camera& camera) {
        int currentFrameDelay = isDying ? dyingFrameDelay :
                                isHurt ? hurtFrameDelay :
                                isAttacking ? attackFrameDelay :
                                isMoving ? moveFrameDelay : idleFrameDelay;

        if (isDying) {
            isMoving = false;
            frameTimer++;
            if (frameTimer >= currentFrameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= dyingFrameCount) {
                    currentFrame = 0;
                    currentDyingSheetIndex++;
                    if (currentDyingSheetIndex >= 3) {
                        toRemove = true;
                    }
                }
            }
        } else if (isHurt) {
            isMoving = false;
            frameTimer++;
            if (frameTimer >= currentFrameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= hurtFrameCount) {
                    isHurt = false;
                    currentFrame = 0;
                    isIdle = true;
                    isMoving = true;
                }
            }
        } else {
            float distance = std::abs(playerX - x);
            if (distance < 80 && !isAttacking && !isHurt) {
                isAttacking = true;
                isMoving = false;
                currentFrame = 0;
                isIdle = false;
                currentAttackSheetIndex = (lastAttackSheetIndex + 1) % 3;
                lastAttackSheetIndex = currentAttackSheetIndex;
                shouldShake = false;
                shakeDelayTimer = 120;
            }

            if (isAttacking) {
                frameTimer++;
                if (frameTimer >= currentFrameDelay) {
                    frameTimer = 0;
                    currentFrame++;
                    if (currentFrame >= attackFrameCount) {
                        if (currentAttackSheetIndex == 1) { // Sheet thứ 2 (index 1)
                            camera.startShake(10,20);
                        }
                        isAttacking = false;
                        currentFrame = 0;
                        isIdle = true;
                        isMoving = true;
                        shouldShake = false;
                    }
                }
                if (shakeDelayTimer > 0) {
                    shakeDelayTimer--;
                }
            } else if (isIdle && isMoving) {
                if (movingRight) {
                    x += speed;
                    if (x >= max_x) {
                        x = max_x;
                        movingRight = false;
                        currentMoveSheetIndex = (currentMoveSheetIndex + 1) % 2;
                    }
                } else {
                    x -= speed;
                    if (x <= min_x) {
                        x = min_x;
                        movingRight = true;
                        currentMoveSheetIndex = (currentMoveSheetIndex + 1) % 2;
                    }
                }
                frameTimer++;
                if (frameTimer >= currentFrameDelay) {
                    frameTimer = 0;
                    currentFrame = (currentFrame + 1) % moveFrameCount;
                }
            } else {
                frameTimer++;
                if (frameTimer >= currentFrameDelay) {
                    frameTimer = 0;
                    currentFrame = (currentFrame + 1) % idleFrameCount;
                }
            }
        }

        if (attackCooldown > 0) attackCooldown--;

        if (!isDying && health <= 0) {
            isHurt = false;
            isDying = true;
            isMoving = false;
            currentFrame = 0;
            currentDyingSheetIndex = 0;
        }
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        int frameWidth, frameHeight;
        if (isDying) {
            frameWidth = 292;
            frameHeight = 122;
        } else if (isHurt) {
            frameWidth = 293;
            frameHeight = 121;
        } else if (isAttacking) {
            frameWidth = 290;
            frameHeight = 120;
        } else if (isMoving) {
            frameWidth = 288;
            frameHeight = 118;
        } else {
            frameWidth = 292;
            frameHeight = 121;
        }

        int newWidth = static_cast<int>(frameWidth * scale);
        int newHeight = static_cast<int>(frameHeight * scale);
        int renderY = static_cast<int>(y) - newHeight + frameHeight;

        float offset = 150.0f;
        SDL_Rect dstRect = {
            static_cast<int>(x - cameraX - offset),
            renderY,
            newWidth,
            newHeight
        };

        if (dstRect.x + dstRect.w > 0 && dstRect.x < SCREEN_WIDTH) {
            SDL_Texture* currentTexture = nullptr;
            if (isDying) {
                currentTexture = dyingTextures[currentDyingSheetIndex];
            } else if (isHurt) {
                currentTexture = hurtTexture;
            } else if (isAttacking) {
                currentTexture = attackTextures[currentAttackSheetIndex];
            } else if (isMoving) {
                currentTexture = moveTextures[currentMoveSheetIndex];
            } else {
                currentTexture = idleTexture;
            }

            SDL_Rect srcRect = { currentFrame * frameWidth, 0, frameWidth, frameHeight };
            SDL_RendererFlip flip = isAttacking ? (movingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL) : (movingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

            if (currentTexture) {
                SDL_RenderCopyEx(renderer, currentTexture, &srcRect, &dstRect, 0, NULL, flip);
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &dstRect);
            }
        }
    }

    int getCurrentFrameWidth() const {
        if (isDying) return 292;
        else if (isHurt) return 293;
        else if (isAttacking) return 290;
        else if (isMoving) return 288;
        else return 292; // idle
    }

    void renderHealthBar(SDL_Renderer* renderer, float cameraX) {
        if (isDying || toRemove) return;

        float offset = 150.0f;
        int frameWidth = getCurrentFrameWidth();
        int newWidth = static_cast<int>(frameWidth * scale);
        int dstX = static_cast<int>(x - cameraX - offset);

        // Chỉ render thanh máu nếu boss hiển thị trên màn hình
        if (dstX + newWidth > 0 && dstX < SCREEN_WIDTH) {
            const int BAR_WIDTH = 1180;
            const int BAR_HEIGHT = 20;
            const int BAR_X = (SCREEN_WIDTH - BAR_WIDTH) / 2;
            const int BAR_Y = SCREEN_HEIGHT - BAR_HEIGHT - 10;

            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_Rect bgRect = { BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT };
            SDL_RenderFillRect(renderer, &bgRect);

            int healthWidth = static_cast<int>((static_cast<float>(health) / maxHealth) * BAR_WIDTH);
            if (healthWidth < 0) healthWidth = 0;
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_Rect healthRect = { BAR_X, BAR_Y, healthWidth, BAR_HEIGHT };
            SDL_RenderFillRect(renderer, &healthRect);
        }
    }

    void cleanup() {
        for (auto& texture : moveTextures) {
            if (texture) SDL_DestroyTexture(texture);
        }
        for (auto& texture : attackTextures) {
            if (texture) SDL_DestroyTexture(texture);
        }
        for (auto& texture : dyingTextures) {
            if (texture) SDL_DestroyTexture(texture);
        }
        if (hurtTexture) SDL_DestroyTexture(hurtTexture);
        if (idleTexture) SDL_DestroyTexture(idleTexture);
    }
};

int Boss::lastAttackSheetIndex = -1;
int Boss::lastDyingSheetIndex = -1;
