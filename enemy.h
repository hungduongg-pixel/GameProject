struct Enemy {
    float x, y;
    float speed;
    bool movingRight;
    float min_x, max_x;
    SDL_Texture* texture;
    SDL_Texture* attackTexture;
    SDL_Texture* hurtTexture;
    SDL_Texture* dyingTexture;
    int currentFrame;
    int frameCount;
    int attackFrameCount;
    int hurtFrameCount;
    int dyingFrameCount;
    int frameDelay;
    int attackFrameDelay;
    int hurtFrameDelay;
    int dyingFrameDelay;
    int frameTimer;
    bool isAttacking;
    bool isHurt;
    bool isDying;
    bool toRemove;
    int hitCount;
    std::vector<Bullet> bullets;
    int shootTimer;
    int shootDelay;
    int attackCooldown;
    int attackCooldownMax;

    void init(SDL_Renderer* renderer, float start_x, float start_y, float min_x, float max_x) {
        x = start_x;
        y = start_y;
        speed = 1.2f;
        movingRight = true;
        this->min_x = min_x;
        this->max_x = max_x;

        std::string imagePath = ASSETS_PATH + "enemy_sheet.png";
        SDL_Surface* surface = IMG_Load(imagePath.c_str());
        texture = surface ? SDL_CreateTextureFromSurface(renderer, surface) : nullptr;
        if (!surface) std::cerr << "❌ Không tải được " << imagePath << ": " << IMG_GetError() << std::endl;
        if (surface) SDL_FreeSurface(surface);

        std::string attackPath = ASSETS_PATH + "enemy_attack_sheet.png";
        SDL_Surface* attackSurface = IMG_Load(attackPath.c_str());
        attackTexture = attackSurface ? SDL_CreateTextureFromSurface(renderer, attackSurface) : nullptr;
        if (!attackSurface) std::cerr << "❌ Không tải được " << attackPath << ": " << IMG_GetError() << std::endl;
        if (attackSurface) SDL_FreeSurface(attackSurface);

        std::string hurtPath = ASSETS_PATH + "enemy_hurt_sheet.png";
        SDL_Surface* hurtSurface = IMG_Load(hurtPath.c_str());
        hurtTexture = hurtSurface ? SDL_CreateTextureFromSurface(renderer, hurtSurface) : nullptr;
        if (!hurtSurface) std::cerr << "❌ Không tải được " << hurtPath << ": " << IMG_GetError() << std::endl;
        if (hurtSurface) SDL_FreeSurface(hurtSurface);

        std::string dyingImagePath = ASSETS_PATH + "enemy_dying_sheet.png";
        SDL_Surface* dyingSurface = IMG_Load(dyingImagePath.c_str());
        dyingTexture = dyingSurface ? SDL_CreateTextureFromSurface(renderer, dyingSurface) : nullptr;
        if (!dyingSurface) std::cerr << "❌ Không tải được " << dyingImagePath << ": " << IMG_GetError() << std::endl;
        if (dyingSurface) SDL_FreeSurface(dyingSurface);

        currentFrame = 0;
        frameCount = 4;
        attackFrameCount = 8;
        hurtFrameCount = 4;
        dyingFrameCount = 6;
        frameDelay = 8;
        attackFrameDelay = 6;
        hurtFrameDelay = 4;
        dyingFrameDelay = 4;
        frameTimer = 0;
        isAttacking = false;
        isHurt = false;
        isDying = false;
        toRemove = false;
        hitCount = 0;
        shootTimer = 0;
        shootDelay = 1.9;
        attackCooldown = 0;
        attackCooldownMax = 60;
    }

    void update(float playerX, float playerY, TextureManager& textureManager, SDL_Renderer* renderer) {
        if (isDying) {
            frameTimer++;
            if (frameTimer >= dyingFrameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= dyingFrameCount) {
                    toRemove = true;
                }
            }
        } else if (isHurt) {
            frameTimer++;
            if (frameTimer >= hurtFrameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= hurtFrameCount) {
                    isHurt = false;
                    currentFrame = 0;
                    if (hitCount >= 2) {
                        isDying = true;
                        currentFrame = 0;
                    }
                }
            }
        } else if (isAttacking) {
            frameTimer++;
            if (frameTimer >= attackFrameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= attackFrameCount) {
                    isAttacking = false;
                    currentFrame = 0;
                }
            }
        } else {
            float distance = std::abs(playerX - x);
            if (!movingRight && distance < 200) {
                isAttacking = true;
                currentFrame = 0;
                shootTimer++;
                if (shootTimer >= shootDelay && bullets.size() < 10) {
                    shootTimer = 0;
                    Bullet bullet;
                    bullet.init(renderer, x, y, movingRight);
                    bullets.push_back(bullet);
                }
            } else {
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
                    currentFrame = (currentFrame + 1) % frameCount;
                }
            }
        }

        if (attackCooldown > 0) attackCooldown--;

        for (auto& bullet : bullets) {
            bullet.update();
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return b.toRemove; }), bullets.end());
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        SDL_Rect dstRect = { static_cast<int>(x - cameraX), static_cast<int>(y), 64, 64 };
        if (dstRect.x + dstRect.w > 0 && dstRect.x < SCREEN_WIDTH) {
            SDL_Texture* currentTexture;
            int frameWidth = 81;
            int frameHeight = 71;
            if (isDying) {
                currentTexture = dyingTexture;
            } else if (isHurt) {
                currentTexture = hurtTexture;
            } else if (isAttacking) {
                currentTexture = attackTexture;
            } else {
                currentTexture = texture;
            }
            SDL_Rect srcRect = { currentFrame * frameWidth, 0, frameWidth, frameHeight };
            SDL_RendererFlip flip = movingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            if (currentTexture) {
                SDL_RenderCopyEx(renderer, currentTexture, &srcRect, &dstRect, 0, NULL, flip);
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &dstRect);
            }
        }
    }

    void cleanup() {
        if (texture) SDL_DestroyTexture(texture);
        if (attackTexture) SDL_DestroyTexture(attackTexture);
        if (hurtTexture) SDL_DestroyTexture(hurtTexture);
        if (dyingTexture) SDL_DestroyTexture(dyingTexture);
        for (auto& bullet : bullets) {
            bullet.cleanup();
        }
    }
};
