struct Player {
    float x, y;
    float velocityY;
    bool isJumping;
    bool isIdle;
    bool isMovingLeft;
    bool isMovingRight;
    bool isAttacking;
    bool isJumpingStart;
    bool isJumpingMid;
    bool isJumpingEnd;
    bool facingLeft;
    bool isDying;
    bool deathAnimationComplete;
    int attackFrameCounter;
    SDL_Rect rect;
    SDL_Texture* idleTexture;
    SDL_Texture* runTexture;
    SDL_Texture* attackTexture;
    SDL_Texture* jumpStartTexture;
    SDL_Texture* jumpMidTexture;
    SDL_Texture* jumpEndTexture;
    SDL_Texture* deadTexture;
    SDL_Texture* healthBarTexture;
    SDL_Texture* healthBarEmptyTexture;
    int currentFrame;
    int frameCount;
    int deadFrameCount;
    int frameDelay;
    int deadFrameDelay;
    int frameTimer;
    int deadFrameTimer;
    float gameStartX, gameStartY;
    bool shouldQuit;
    int lives;
    float lastDeathX, lastDeathY;
    int health;
    int maxHealth;
    float displayHealth;

    void init(SDL_Renderer* renderer, int startX, int startY) {
        x = startX;
        y = startY;
        gameStartX = startX;
        gameStartY = startY;
        lastDeathX = startX;
        lastDeathY = startY;
        velocityY = 0;
        isJumping = false;
        isJumpingStart = false;
        isJumpingMid = false;
        isJumpingEnd = false;
        isIdle = true;
        isMovingLeft = false;
        isMovingRight = false;
        isAttacking = false;
        facingLeft = false;
        isDying = false;
        deathAnimationComplete = false;
        attackFrameCounter = 0;
        rect = { static_cast<int>(x), static_cast<int>(y), 70, 70 };
        lives = 3;
        maxHealth = 100;
        health = maxHealth;
        displayHealth = static_cast<float>(health);

        SDL_Surface* idleSurface = IMG_Load((ASSETS_PATH + "Idle-Sheet1.png").c_str());
        idleTexture = idleSurface ? SDL_CreateTextureFromSurface(renderer, idleSurface) : nullptr;
        if (!idleSurface) std::cerr << "❌ Không tải được Idle-Sheet1.png: " << IMG_GetError() << std::endl;
        if (idleSurface) SDL_FreeSurface(idleSurface);

        SDL_Surface* runSurface = IMG_Load((ASSETS_PATH + "RunRight-Sheet1.png").c_str());
        runTexture = runSurface ? SDL_CreateTextureFromSurface(renderer, runSurface) : nullptr;
        if (!runSurface) std::cerr << "❌ Không tải được RunRight-Sheet1.png: " << IMG_GetError() << std::endl;
        if (runSurface) SDL_FreeSurface(runSurface);

        SDL_Surface* attackSurface = IMG_Load((ASSETS_PATH + "Attack-Sheet1.png").c_str());
        attackTexture = attackSurface ? SDL_CreateTextureFromSurface(renderer, attackSurface) : nullptr;
        if (!attackSurface) std::cerr << "❌ Không tải được Attack-Sheet1.png: " << IMG_GetError() << std::endl;
        if (attackSurface) SDL_FreeSurface(attackSurface);

        SDL_Surface* jumpStartSurface = IMG_Load((ASSETS_PATH + "Jump-Start-Sheet.png").c_str());
        jumpStartTexture = jumpStartSurface ? SDL_CreateTextureFromSurface(renderer, jumpStartSurface) : nullptr;
        if (!jumpStartSurface) std::cerr << "❌ Không tải được Jump-Start-Sheet.png: " << IMG_GetError() << std::endl;
        if (jumpStartSurface) SDL_FreeSurface(jumpStartSurface);

        SDL_Surface* jumpMidSurface = IMG_Load((ASSETS_PATH + "Jump-Mid-Sheet.png").c_str());
        jumpMidTexture = jumpMidSurface ? SDL_CreateTextureFromSurface(renderer, jumpMidSurface) : nullptr;
        if (!jumpMidSurface) std::cerr << "❌ Không tải được Jump-Mid-Sheet.png: " << IMG_GetError() << std::endl;
        if (jumpMidSurface) SDL_FreeSurface(jumpMidSurface);

        SDL_Surface* jumpEndSurface = IMG_Load((ASSETS_PATH + "Jump-End-Sheet.png").c_str());
        jumpEndTexture = jumpEndSurface ? SDL_CreateTextureFromSurface(renderer, jumpEndSurface) : nullptr;
        if (!jumpEndSurface) std::cerr << "❌ Không tải được Jump-End-Sheet.png: " << IMG_GetError() << std::endl;
        if (jumpEndSurface) SDL_FreeSurface(jumpEndSurface);

        SDL_Surface* deadSurface = IMG_Load((ASSETS_PATH + "Dead-Sheet.png").c_str());
        deadTexture = deadSurface ? SDL_CreateTextureFromSurface(renderer, deadSurface) : nullptr;
        if (!deadSurface) std::cerr << "❌ Không tải được Dead-Sheet.png: " << IMG_GetError() << std::endl;
        if (deadSurface) SDL_FreeSurface(deadSurface);

        healthBarTexture = IMG_LoadTexture(renderer, (ASSETS_PATH + "health_bar_full.png").c_str());
        if (!healthBarTexture) std::cerr << "❌ Không tải được health_bar_full.png: " << IMG_GetError() << std::endl;

        healthBarEmptyTexture = IMG_LoadTexture(renderer, (ASSETS_PATH + "health_bar_empty.png").c_str());
        if (!healthBarEmptyTexture) std::cerr << "❌ Không tải được health_bar_empty.png: " << IMG_GetError() << std::endl;

        frameCount = 8;
        deadFrameCount = 8;
        frameDelay = 4;
        deadFrameDelay = 6;
        frameTimer = 0;
        deadFrameTimer = 0;
    }

    void resetToNearestCheckpoint(const std::vector<Platform>& platforms) {
        if (platforms.empty()) {
            x = gameStartX;
            y = gameStartY;
        } else {
            float minDistance = std::numeric_limits<float>::max();
            Platform nearestPlatform = platforms[0];
            for (const auto& platform : platforms) {
                float platformCenterX = platform.rect.x + platform.rect.w / 2.0f;
                float distance = std::abs(platformCenterX - lastDeathX);
                if (distance < minDistance) {
                    minDistance = distance;
                    nearestPlatform = platform;
                }
            }
            x = nearestPlatform.rect.x + (nearestPlatform.rect.w - rect.w) / 2.0f;
            y = nearestPlatform.rect.y - rect.h;
        }
        rect.x = static_cast<int>(x);
        rect.y = static_cast<int>(y);
        velocityY = 0;
        health = maxHealth;
        displayHealth = static_cast<float>(health);
        isJumping = false;
        isJumpingStart = false;
        isJumpingMid = false;
        isJumpingEnd = false;
        isIdle = true;
        isMovingLeft = false;
        isMovingRight = false;
        isAttacking = false;
        isDying = false;
        deathAnimationComplete = false;
        facingLeft = false;
        attackFrameCounter = 0;
        currentFrame = 0;
        deadFrameTimer = 0;
        std::cout << "Nhân vật hồi sinh tại x=" << x << ", y=" << y << ", health=" << health << std::endl;
    }

    void update(const std::vector<Platform>& platforms) {
        if (isDying) {
            deadFrameTimer++;
            if (deadFrameTimer >= deadFrameDelay) {
                deadFrameTimer = 0;
                currentFrame++;
                if (currentFrame >= deadFrameCount) {
                    isDying = false;
                    deathAnimationComplete = true;
                    currentFrame = 0;
                    lastDeathX = x;
                    lastDeathY = y;
                    lives--;
                    if (lives > 0) {
                        resetToNearestCheckpoint(platforms);
                    } else {
                        shouldQuit = true;
                    }
                }
            }
            return;
        }

        const float LERP_SPEED = 0.1f;
        displayHealth += (health - displayHealth) * LERP_SPEED;
        displayHealth = std::max(0.0f, std::min(static_cast<float>(maxHealth), displayHealth));

        float newX = x;
        if (isMovingLeft) {
            newX = std::max(0.0f, x - 4);
        }
        if (isMovingRight) {
            newX = std::min(static_cast<float>(MAP_WIDTH * TILE_WIDTH - rect.w), x + 4);
        }

        SDL_Rect newRect = rect;
        newRect.x = static_cast<int>(newX);
        bool canMove = true;
        for (const auto& platform : platforms) {
            if (SDL_HasIntersection(&newRect, &platform.rect)) {
                canMove = false;
                if (isMovingLeft) {
                    x = platform.rect.x + platform.rect.w;
                } else if (isMovingRight) {
                    x = platform.rect.x - rect.w;
                }
                break;
            }
        }
        if (canMove) {
            x = newX;
            if (isMovingLeft) facingLeft = true;
            else if (isMovingRight) facingLeft = false;
        }

        velocityY += GRAVITY;
        y += velocityY;

        if (y > SCREEN_HEIGHT && !isDying) {
            lastDeathX = x;
            lastDeathY = y;
            isDying = true;
            currentFrame = 0;
            deadFrameTimer = 0;
            return;
        }

        bool onPlatform = false;
        for (const auto& platform : platforms) {
            if (rect.x + rect.w > platform.rect.x && rect.x < platform.rect.x + platform.rect.w &&
                y + rect.h >= platform.rect.y && y + rect.h <= platform.rect.y + platform.rect.h &&
                velocityY >= 0) {
                y = platform.rect.y - rect.h;
                velocityY = 0;
                onPlatform = true;
                if (isJumping || isJumpingMid) {
                    isJumpingEnd = true;
                    isJumpingMid = false;
                    isJumping = false;
                    currentFrame = 0;
                }
                break;
            }
        }

        if (!onPlatform && !isJumping && !isJumpingMid) {
            isJumping = true;
            isJumpingStart = true;
            currentFrame = 0;
        }

        frameTimer++;
        if (frameTimer >= frameDelay) {
            frameTimer = 0;
            if (isAttacking) {
                currentFrame++;
                attackFrameCounter++;
                if (attackFrameCounter >= 8) {
                    isAttacking = false;
                    attackFrameCounter = 0;
                    currentFrame = 0;
                }
            } else if (isJumpingStart) {
                currentFrame++;
                if (currentFrame >= 4) {
                    isJumpingStart = false;
                    isJumpingMid = true;
                    currentFrame = 0;
                }
            } else if (isJumpingMid) {
                currentFrame++;
                if (currentFrame >= 8) {
                    currentFrame = 8;
                }
            } else if (isJumpingEnd) {
                currentFrame++;
                if (currentFrame >= 4) {
                    isJumpingEnd = false;
                    currentFrame = 0;
                }
            } else if (isMovingLeft || isMovingRight) {
                currentFrame = (currentFrame + 1) % 8;
            } else {
                currentFrame = (currentFrame + 1) % 4;
            }
        }

        rect.x = static_cast<int>(x);
        rect.y = static_cast<int>(y);
    }

    void moveLeft() { if (!isDying) { isMovingLeft = true; isIdle = false; } }
    void moveRight() { if (!isDying) { isMovingRight = true; isIdle = false; } }
    void jump() {
        if (!isDying && !isJumping && !isJumpingStart && !isJumpingMid && !isJumpingEnd) {
            velocityY = JUMP_STRENGTH;
            isJumping = true;
            isJumpingStart = true;
            currentFrame = 0;
        }
    }
    void stop() {
        if (!isDying && !isMovingLeft && !isMovingRight && !isJumping) isIdle = true;
    }
    void attack() {
        if (!isDying && !isAttacking) {
            isAttacking = true;
            currentFrame = 0;
            attackFrameCounter = 0;
            frameCount = 8;
            frameTimer = 0;
            isMovingLeft = false;
            isMovingRight = false;
        }
    }

    void renderHealthBar(SDL_Renderer* renderer) {
        const int HEART_WIDTH = 33;
        const int BAR_WIDTH = 160;
        const int HEIGHT = 68;

        SDL_Rect heartRect = {10, 50, HEART_WIDTH, HEIGHT};
        if (healthBarTexture) {
            SDL_Rect heartSrcRect = {0, 0, HEART_WIDTH, HEIGHT};
            SDL_RenderCopy(renderer, healthBarTexture, &heartSrcRect, &heartRect);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &heartRect);
        }

        int healthWidth = static_cast<int>(BAR_WIDTH * (displayHealth / maxHealth));
        if (healthWidth < 0) healthWidth = 0;
        SDL_Rect healthBarRect = {10 + HEART_WIDTH, 50, healthWidth, HEIGHT};
        if (healthBarTexture && healthWidth > 0) {
            SDL_Rect srcRect = {HEART_WIDTH, 0, healthWidth, HEIGHT};
            SDL_RenderCopy(renderer, healthBarTexture, &srcRect, &healthBarRect);
        } else if (healthWidth > 0) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &healthBarRect);
        }

        if (healthWidth < BAR_WIDTH) {
            SDL_Rect emptyBarRect = {10 + HEART_WIDTH + healthWidth, 50, BAR_WIDTH - healthWidth, HEIGHT};
            if (healthBarEmptyTexture) {
                SDL_RenderCopy(renderer, healthBarEmptyTexture, NULL, &emptyBarRect);
            } else {
                SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
                SDL_RenderFillRect(renderer, &emptyBarRect);
            }
        }
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        SDL_Rect renderRect = rect;
        renderRect.x -= static_cast<int>(cameraX);
        SDL_RendererFlip flip = facingLeft ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

        if (isDying) {
            SDL_Rect srcRect = { (currentFrame % 8) * 80, 0, 80, 47 };
            SDL_RenderCopyEx(renderer, deadTexture, &srcRect, &renderRect, 0, NULL, flip);
        } else if (isJumpingStart) {
            SDL_Rect srcRect = { (currentFrame % 4) * 64, 0, 64, 64 };
            SDL_RenderCopyEx(renderer, jumpStartTexture, &srcRect, &renderRect, 0, NULL, flip);
        } else if (isJumpingMid) {
            SDL_Rect srcRect = { (currentFrame % 8) * 64, 0, 64, 60 };
            SDL_RenderCopyEx(renderer, jumpMidTexture, &srcRect, &renderRect, 0, NULL, flip);
        } else if (isJumpingEnd) {
            SDL_Rect srcRect = { (currentFrame % 3) * 64, 0, 64, 64 };
            SDL_RenderCopyEx(renderer, jumpEndTexture, &srcRect, &renderRect, 0, NULL, flip);
        } else if (isAttacking) {
            SDL_Rect srcRect = { (currentFrame % 8) * 96, 0, 96, 64 };
            SDL_RenderCopyEx(renderer, attackTexture, &srcRect, &renderRect, 0, NULL, flip);
        } else if (isMovingLeft || isMovingRight) {
            SDL_Rect srcRect = { (currentFrame % 8) * 80, 0, 80, 66 };
            SDL_RenderCopyEx(renderer, runTexture, &srcRect, &renderRect, 0, NULL, flip);
        } else {
            SDL_Rect srcRect = { (currentFrame % 4) * 64, 0, 64, 64 };
            SDL_RenderCopyEx(renderer, idleTexture, &srcRect, &renderRect, 0, NULL, flip);
        }
    }

    void cleanup() {
        if (idleTexture) SDL_DestroyTexture(idleTexture);
        if (runTexture) SDL_DestroyTexture(runTexture);
        if (attackTexture) SDL_DestroyTexture(attackTexture);
        if (jumpStartTexture) SDL_DestroyTexture(jumpStartTexture);
        if (jumpMidTexture) SDL_DestroyTexture(jumpMidTexture);
        if (jumpEndTexture) SDL_DestroyTexture(jumpEndTexture);
        if (deadTexture) SDL_DestroyTexture(deadTexture);
        if (healthBarTexture) SDL_DestroyTexture(healthBarTexture);
        if (healthBarEmptyTexture) SDL_DestroyTexture(healthBarEmptyTexture);
    }
};
