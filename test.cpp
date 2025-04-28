
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>

const int SCREEN_WIDTH = 1180;
const int SCREEN_HEIGHT = 748;
const float GRAVITY = 0.25f;
const float JUMP_STRENGTH = -8.3f;
bool isGameStarted = false;
bool musicStarted = false;
bool debugBullet = false;

const int TILE_WIDTH = 59;
const int TILE_HEIGHT = 68;
const int MAP_HEIGHT = 11;
const int MAP_WIDTH = 160;

const std::string ASSETS_PATH = "assets/";

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

bool loadLevelMap(const std::string& filePath, int levelMap[MAP_HEIGHT][MAP_WIDTH]) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "❌ Không mở được tệp: " << filePath << std::endl;
        return false;
    }

    std::string line;
    int row = 0;
    while (std::getline(file, line) && row < MAP_HEIGHT) {
        std::istringstream iss(line);
        int col = 0;
        int value;
        while (iss >> value && col < MAP_WIDTH) {
            levelMap[row][col] = value;
            col++;
        }
        if (col != MAP_WIDTH) {
            std::cerr << "❌ Số cột không hợp lệ ở hàng " << row + 1 << std::endl;
            file.close();
            return false;
        }
        row++;
    }
    if (row != MAP_HEIGHT) {
        std::cerr << "❌ Số hàng không hợp lệ trong " << filePath << std::endl;
        file.close();
        return false;
    }
    file.close();
    return true;
}

struct Camera {
    float x;
    float y;
    int mapWidthPixels;

    void init(int mapWidth) {
        x = 0;
        y = 0;
        mapWidthPixels = mapWidth * TILE_WIDTH;
    }

    void update(float playerX) {
        x = playerX - SCREEN_WIDTH / 2;
        if (x < 0) x = 0;
        if (x > mapWidthPixels - SCREEN_WIDTH) x = mapWidthPixels - SCREEN_WIDTH;
    }
};

struct Platform {
    SDL_Rect rect;
    SDL_Texture* texture;

    void init(SDL_Renderer* renderer, int x, int y, int type, TextureManager& textureManager) {
        rect = { x, y, TILE_WIDTH, TILE_HEIGHT };
        texture = (type == 1) ? textureManager.map1Texture : textureManager.map2Texture;
        if (!texture) std::cerr << "❌ Texture nền không hợp lệ cho loại " << type << std::endl;
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        SDL_Rect renderRect = rect;
        renderRect.x -= static_cast<int>(cameraX);
        if (renderRect.x + renderRect.w > 0 && renderRect.x < SCREEN_WIDTH) {
            if (texture) {
                SDL_RenderCopy(renderer, texture, NULL, &renderRect);
            } else {
                SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
                SDL_RenderFillRect(renderer, &renderRect);
            }
        }
    }

    void cleanup() {}
};

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
        // Chỉ ghi log khi khởi tạo nếu debugBullet là true
        if (debugBullet) {
            std::cout << "Bullet initialized at x: " << x << ", y: " << y << std::endl;
        }
    }

    void update() {
        x += velocityX;
        if (x < -width || x > MAP_WIDTH * TILE_WIDTH + width) {
            toRemove = true;
        }
        // Chỉ ghi log khi viên đạn bị xóa
        if (debugBullet && toRemove) {
            std::cout << "Bullet removed at x: " << x << ", y: " << y << std::endl;
        }
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        int renderX = static_cast<int>(x - cameraX);
        SDL_Rect dstRect = { renderX, static_cast<int>(y), width, height };

        // Chỉ ghi log khi viên đạn được vẽ trong màn hình
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
        SDL_Surface* endSurface = TTF_RenderUTF8_Blended(endFont, "BẠN THẮNG", yellow);
        endTextTexture = endSurface ? SDL_CreateTextureFromSurface(renderer, endSurface) : nullptr;
        endTextRect = endSurface ? SDL_Rect{(SCREEN_WIDTH - endSurface->w) / 2, 0, endSurface->w, endSurface->h} : SDL_Rect{0, 0, 0, 0};
        if (endSurface) SDL_FreeSurface(endSurface);

        SDL_Surface* helloSurface = TTF_RenderUTF8_Blended(endFont, "CHÚC MỪNG!", yellow);
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
        shootDelay = 1,9;
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
        // Chỉ tấn công khi quay sang trái (movingRight == false) và khoảng cách đủ gần
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
            // Di chuyển bình thường nếu không tấn công
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
        // Dịch lên trên bằng cách tăng y thêm 10 pixel
        y = start_y - 10.0f; // Giảm y để dịch lên trên (tăng -10 tức là di chuyển lên trên)
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

    // Hàm update và render giữ nguyên như trước
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
        y = nearestPlatform.rect.y - rect.h; // Đặt nhân vật trực tiếp trên nền tảng
    }
    rect.x = static_cast<int>(x);
    rect.y = static_cast<int>(y);
    velocityY = 0;
    health = maxHealth;
    displayHealth = static_cast<float>(health);
    isJumping = false; // Không kích hoạt trạng thái nhảy
    isJumpingStart = false;
    isJumpingMid = false;
    isJumpingEnd = false;
    isIdle = true; // Đặt trạng thái nghỉ
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

        // Vẽ biểu tượng trái tim
        SDL_Rect heartRect = {10, 50, HEART_WIDTH, HEIGHT};
        if (healthBarTexture) {
            SDL_Rect heartSrcRect = {0, 0, HEART_WIDTH, HEIGHT};
            SDL_RenderCopy(renderer, healthBarTexture, &heartSrcRect, &heartRect);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &heartRect);
        }

        // Vẽ phần máu còn lại
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

        // Vẽ phần máu đã mất
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

struct Boss {
    float x, y;
    float speed;
    bool movingRight;
    float min_x, max_x;
    std::vector<SDL_Texture*> moveTextures; // 2 sprite sheets for move
    std::vector<SDL_Texture*> attackTextures; // 3 sprite sheets for attack
    std::vector<SDL_Texture*> dyingTextures; // 3 sprite sheets for dying
    SDL_Texture* hurtTexture;
    SDL_Texture* idleTexture;
    int currentFrame;
    int moveFrameCount; // 10 frames
    int attackFrameCount; // 11 frames
    int dyingFrameCount; // 12 frames
    int hurtFrameCount; // 4 frames
    int idleFrameCount; // 8 frames
    int moveFrameDelay; // Delay for move
    int attackFrameDelay; // Delay for attack
    int dyingFrameDelay; // Delay for dying
    int hurtFrameDelay; // Delay for hurt
    int idleFrameDelay; // Delay for idle
    int frameTimer;
    bool isAttacking;
    bool isDying;
    bool toRemove;
    bool isHurt;
    bool isIdle;
    bool isMoving; // State for moving
    int hitCount;
    int attackCooldown;
    int attackCooldownMax;
    int currentMoveSheetIndex; // Index of current move sprite sheet
    int currentAttackSheetIndex; // Index of current attack sprite sheet
    int currentDyingSheetIndex; // Index of current dying sprite sheet
    static int lastAttackSheetIndex; // Track last used attack sheet
    static int lastDyingSheetIndex; // Track last used dying sheet

    void init(SDL_Renderer* renderer, float start_x, float start_y, float min_x, float max_x) {
        x = start_x;
        y = start_y - 10.0f; // Same upward offset as NewEnemy
        speed = 1.2f;
        movingRight = true;
        this->min_x = min_x;
        this->max_x = max_x;

        // Load move textures (2 sheets)
        moveTextures.resize(2, nullptr);
        for (int i = 0; i < 2; ++i) {
            std::string movePath = ASSETS_PATH + "boss_move" + std::to_string(i + 1) + "_sheet.png";
            SDL_Surface* moveSurface = IMG_Load(movePath.c_str());
            moveTextures[i] = moveSurface ? SDL_CreateTextureFromSurface(renderer, moveSurface) : nullptr;
            if (!moveSurface) std::cerr << "❌ Không tải được " << movePath << ": " << IMG_GetError() << std::endl;
            if (moveSurface) SDL_FreeSurface(moveSurface);
        }

        // Load attack textures (3 sheets)
        attackTextures.resize(3, nullptr);
        for (int i = 0; i < 3; ++i) {
            std::string attackPath = ASSETS_PATH + "boss_attack" + std::to_string(i + 1) + "_sheet.png";
            SDL_Surface* attackSurface = IMG_Load(attackPath.c_str());
            attackTextures[i] = attackSurface ? SDL_CreateTextureFromSurface(renderer, attackSurface) : nullptr;
            if (!attackSurface) std::cerr << "❌ Không tải được " << attackPath << ": " << IMG_GetError() << std::endl;
            if (attackSurface) SDL_FreeSurface(attackSurface);
        }

        // Load dying textures (3 sheets)
        dyingTextures.resize(3, nullptr);
        for (int i = 0; i < 3; ++i) {
            std::string dyingPath = ASSETS_PATH + "boss_dying" + std::to_string(i + 1) + "_sheet.png";
            SDL_Surface* dyingSurface = IMG_Load(dyingPath.c_str());
            dyingTextures[i] = dyingSurface ? SDL_CreateTextureFromSurface(renderer, dyingSurface) : nullptr;
            if (!dyingSurface) std::cerr << "❌ Không tải được " << dyingPath << ": " << IMG_GetError() << std::endl;
            if (dyingSurface) SDL_FreeSurface(dyingSurface);
        }

        // Load hurt texture (1 sheet)
        std::string hurtPath = ASSETS_PATH + "boss_hurt_sheet.png";
        SDL_Surface* hurtSurface = IMG_Load(hurtPath.c_str());
        hurtTexture = hurtSurface ? SDL_CreateTextureFromSurface(renderer, hurtSurface) : nullptr;
        if (!hurtSurface) std::cerr << "❌ Không tải được " << hurtPath << ": " << IMG_GetError() << std::endl;
        if (hurtSurface) SDL_FreeSurface(hurtSurface);

        // Load idle texture (1 sheet)
        std::string idlePath = ASSETS_PATH + "boss_Idle_sheet.png";
        SDL_Surface* idleSurface = IMG_Load(idlePath.c_str());
        idleTexture = idleSurface ? SDL_CreateTextureFromSurface(renderer, idleSurface) : nullptr;
        if (!idleSurface) std::cerr << "❌ Không tải được " << idlePath << ": " << IMG_GetError() << std::endl;
        if (idleSurface) SDL_FreeSurface(idleSurface);

        currentFrame = 0;
        moveFrameCount = 6; // Same as NewEnemy
        attackFrameCount = 5; // Same as NewEnemy
        dyingFrameCount = 6; // Same as NewEnemy
        hurtFrameCount = 5; // Same as NewEnemy
        idleFrameCount = 6; // Same as NewEnemy
        moveFrameDelay = 8; // Same as NewEnemy
        attackFrameDelay = 8; // Same as NewEnemy
        dyingFrameDelay = 8; // Same as NewEnemy
        hurtFrameDelay = 8; // Same as NewEnemy
        idleFrameDelay = 8; // Same as NewEnemy
        frameTimer = 0;
        isAttacking = false;
        isDying = false;
        toRemove = false;
        isHurt = false;
        isIdle = true;
        isMoving = true; // Initialize as moving
        hitCount = 0;
        attackCooldown = 0;
        attackCooldownMax = 60;
        currentMoveSheetIndex = 0;
        currentAttackSheetIndex = (lastAttackSheetIndex + 1) % 3; // Start with next attack sheet
        lastAttackSheetIndex = currentAttackSheetIndex;
        currentDyingSheetIndex = (lastDyingSheetIndex + 1) % 3; // Start with next dying sheet
        lastDyingSheetIndex = currentDyingSheetIndex;
    }

    void update(float playerX, float playerY) {
        int currentFrameDelay = isDying ? dyingFrameDelay :
                                isHurt ? hurtFrameDelay :
                                isAttacking ? attackFrameDelay :
                                isMoving ? moveFrameDelay : idleFrameDelay;

        if (isDying) {
            isMoving = false; // Not moving when dying
            frameTimer++;
            if (frameTimer >= currentFrameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= dyingFrameCount) {
                    toRemove = true;
                }
            }
        } else if (isHurt) {
            isMoving = false; // Not moving when hurt
            frameTimer++;
            if (frameTimer >= currentFrameDelay) {
                frameTimer = 0;
                currentFrame++;
                if (currentFrame >= hurtFrameCount) {
                    isHurt = false;
                    currentFrame = 0;
                    isIdle = true;
                    isMoving = true; // Resume moving after hurt
                }
            }
        } else {
            float distance = std::abs(playerX - x);
            if (!movingRight && distance < 100 && !isAttacking) {
                isAttacking = true;
                isMoving = false; // Stop moving when attacking
                currentFrame = 0;
                isIdle = false;
                // Chọn sprite sheet tấn công tuần tự
                currentAttackSheetIndex = (lastAttackSheetIndex + 1) % 3;
                lastAttackSheetIndex = currentAttackSheetIndex;
            }

            if (isAttacking) {
                frameTimer++;
                if (frameTimer >= currentFrameDelay) {
                    frameTimer = 0;
                    currentFrame++;
                    if (currentFrame >= attackFrameCount) {
                        isAttacking = false;
                        currentFrame = 0;
                        isIdle = true;
                        isMoving = true; // Resume moving after attack
                    }
                }
            } else if (isIdle && isMoving) {
                if (movingRight) {
                    x += speed;
                    if (x >= max_x) {
                        x = max_x;
                        movingRight = false;
                        // Chuyển đổi move sheet khi đổi hướng
                        currentMoveSheetIndex = (currentMoveSheetIndex + 1) % 2;
                    }
                } else {
                    x -= speed;
                    if (x <= min_x) {
                        x = min_x;
                        movingRight = true;
                        // Chuyển đổi move sheet khi đổi hướng
                        currentMoveSheetIndex = (currentMoveSheetIndex + 1) % 2;
                    }
                }
                frameTimer++;
                if (frameTimer >= currentFrameDelay) {
                    frameTimer = 0;
                    currentFrame = (currentFrame + 1) % moveFrameCount;
                }
            } else {
                // Idle state (not moving)
                frameTimer++;
                if (frameTimer >= currentFrameDelay) {
                    frameTimer = 0;
                    currentFrame = (currentFrame + 1) % idleFrameCount;
                }
            }
        }

        if (attackCooldown > 0) attackCooldown--;

        // Khi bị trúng đòn và chuyển sang trạng thái chết, chọn sprite sheet tuần tự
        if (!isDying && isHurt && hitCount >= 3) {
            isHurt = false;
            isDying = true;
            isMoving = false; // Not moving when dying
            currentFrame = 0;
            currentDyingSheetIndex = (lastDyingSheetIndex + 1) % 3;
            lastDyingSheetIndex = currentDyingSheetIndex;
        }
    }

    void render(SDL_Renderer* renderer, float cameraX) {
        SDL_Rect dstRect = { static_cast<int>(x - cameraX), static_cast<int>(y), 220, 200 };
        if (dstRect.x + dstRect.w > 0 && dstRect.x < SCREEN_WIDTH) {
            SDL_Texture* currentTexture = nullptr;
            int frameWidth = 260;
            int frameHeight = 110;

            if (isDying) {
                currentTexture = dyingTextures[currentDyingSheetIndex];
                frameWidth = 272;
                frameHeight = 132;
            } else if (isHurt) {
                currentTexture = hurtTexture;
                frameWidth = 262;
                frameHeight = 109;
            } else if (isAttacking) {
                currentTexture = attackTextures[currentAttackSheetIndex];
                frameWidth = 270;
                frameHeight = 118;
            } else if (isMoving) {
                currentTexture = moveTextures[currentMoveSheetIndex];
                frameWidth = 262;
                frameHeight = 110;
            } else {
                currentTexture = idleTexture;
                frameWidth = 262;
                frameHeight = 110;
            }

            SDL_Rect srcRect = { currentFrame * frameWidth, 0, frameWidth, frameHeight };
            SDL_RendererFlip flip = (isAttacking && !movingRight) ? SDL_FLIP_HORIZONTAL : (movingRight ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
            if (currentTexture) {
                SDL_RenderCopyEx(renderer, currentTexture, &srcRect, &dstRect, 0, NULL, flip);
            } else {
                // Fallback rendering if texture is missing
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &dstRect);
            }
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

// Initialize static members
int Boss::lastAttackSheetIndex = -1;
int Boss::lastDyingSheetIndex = -1;

SDL_Rect getAttackRect(SDL_Rect playerRect, float cameraX, bool facingLeft) {
    SDL_Rect attackRect = playerRect;
    attackRect.x -= static_cast<int>(cameraX);
    attackRect.w = 4;
    if (facingLeft) attackRect.x -= 50;
    else attackRect.x += playerRect.w;
    return attackRect;
}

SDL_Rect getEnemyAttackRect(const Enemy& enemy) {
    SDL_Rect attackRect = { static_cast<int>(enemy.x), static_cast<int>(enemy.y), 100, 64 };
    if (enemy.movingRight) {
        attackRect.x += 20;
    } else {
        attackRect.x -= 100;
    }
    return attackRect;
}

SDL_Rect getNewEnemyAttackRect(const NewEnemy& newEnemy) {
    SDL_Rect attackRect = { static_cast<int>(newEnemy.x), static_cast<int>(newEnemy.y), 40, 40 }; // Giảm kích thước
    if (newEnemy.movingRight) {
        attackRect.x += 15; // Giảm dịch chuyển sang phải
    } else {
        attackRect.x -= 60; // Giảm dịch chuyển sang trái
    }
    return attackRect;
}

SDL_Rect getBossAttackRect(const Boss& boss) {
    SDL_Rect attackRect = { static_cast<int>(boss.x), static_cast<int>(boss.y), 40, 40 };
    if (boss.movingRight) {
        attackRect.x += 15;
    } else {
        attackRect.x -= 60;
    }
    return attackRect;
}

// Modified initializeLevel function to support Boss (type 8)
void initializeLevel(SDL_Renderer* renderer, const int (&levelMap)[MAP_HEIGHT][MAP_WIDTH],
                     std::vector<Platform>& platforms, std::vector<Enemy>& enemies,
                     std::vector<NewEnemy>& newEnemies, std::vector<NewEnemy5>& newEnemies5,
                     std::vector<Boss>& bosses, std::vector<Door>& doors,
                     std::vector<Item>& items, Player& player, TextureManager& textureManager) {
    // Clear existing entities
    for (auto& platform : platforms) platform.cleanup();
    platforms.clear();
    for (auto& enemy : enemies) enemy.cleanup();
    enemies.clear();
    for (auto& newEnemy : newEnemies) newEnemy.cleanup();
    newEnemies.clear();
    for (auto& newEnemy5 : newEnemies5) newEnemy5.cleanup();
    newEnemies5.clear();
    for (auto& boss : bosses) boss.cleanup();
    bosses.clear();
    for (auto& door : doors) door.cleanup();
    doors.clear();
    for (auto& item : items) item.cleanup();
    items.clear();

    // Initialize platforms, doors, and items
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            if (levelMap[i][j] == 1 || levelMap[i][j] == 3) {
                Platform platform;
                platform.init(renderer, j * TILE_WIDTH, i * TILE_HEIGHT, levelMap[i][j], textureManager);
                platforms.push_back(platform);
            } else if (levelMap[i][j] == 6) {
                Door door;
                door.init(renderer, j * TILE_WIDTH, i * TILE_HEIGHT, textureManager);
                doors.push_back(door);
            } else if (levelMap[i][j] == 7) {
                Item item;
                item.init(renderer, j * TILE_WIDTH, i * TILE_HEIGHT);
                items.push_back(item);
            }
        }
    }

    // Initialize enemies and bosses
    for (int i = 0; i < MAP_HEIGHT; i++) {
        int j = 0;
        while (j < MAP_WIDTH) {
            if (levelMap[i][j] == 2 || levelMap[i][j] == 4 || levelMap[i][j] == 5 || levelMap[i][j] == 8) {
                int enemyType = levelMap[i][j];
                int start_col = j;
                while (j < MAP_WIDTH && levelMap[i][j] == enemyType) j++;
                int end_col = j - 1;
                float min_x = start_col * TILE_WIDTH;
                float max_x = (end_col + 1) * TILE_WIDTH;

                int foundPlatformRow = -1;
                for (int row = i + 1; row < MAP_HEIGHT; row++) {
                    bool hasPlatform = false;
                    for (int col = start_col; col <= end_col; col++) {
                        if (levelMap[row][col] == 1 || levelMap[row][col] == 3) {
                            hasPlatform = true;
                            break;
                        }
                    }
                    if (hasPlatform) {
                        foundPlatformRow = row;
                        break;
                    }
                }

                if (foundPlatformRow == -1) {
                    std::cerr << "Không tìm thấy nền tảng bên dưới kẻ địch ở hàng " << i << std::endl;
                    continue;
                }

                float y = foundPlatformRow * TILE_HEIGHT - 64;
                float start_x = min_x;
                if (enemyType == 4 || enemyType == 8) { // Apply same offset for NewEnemy and Boss
                    y -= 35;
                }

                if (enemyType == 2) {
                    Enemy enemy;
                    enemy.init(renderer, start_x, y, min_x, max_x);
                    enemies.push_back(enemy);
                } else if (enemyType == 4) {
                    NewEnemy newEnemy;
                    newEnemy.init(renderer, start_x, y, min_x, max_x);
                    newEnemies.push_back(newEnemy);
                } else if (enemyType == 5) {
                    NewEnemy5 newEnemy5;
                    newEnemy5.init(renderer, start_x, y, min_x, max_x);
                    newEnemies5.push_back(newEnemy5);
                } else if (enemyType == 8) {
                    Boss boss;
                    boss.init(renderer, start_x, y, min_x, max_x);
                    bosses.push_back(boss);
                }
            } else {
                j++;
            }
        }
    }

    // Initialize player
    player.x = 2 * TILE_WIDTH;
    player.y = 2 * TILE_HEIGHT - 85;
    player.gameStartX = player.x;
    player.gameStartY = player.y;
    player.lastDeathX = player.x;
    player.lastDeathY = player.y;
    player.rect.x = static_cast<int>(player.x);
    player.rect.y = static_cast<int>(player.y);
    player.velocityY = 0;
    player.isJumping = false;
    player.isJumpingStart = false;
    player.isJumpingMid = false;
    player.isJumpingEnd = false;
    player.isIdle = true;
    player.isMovingLeft = false;
    player.isMovingRight = false;
    player.isAttacking = false;
    player.facingLeft = false;
    player.attackFrameCounter = 0;
    player.currentFrame = 0;
    player.lives = 3;
    player.health = player.maxHealth;
}

void renderBackground(SDL_Renderer* renderer, SDL_Texture* mapTexture, float cameraX, int mapWidthPixels) {
    int backgroundWidth = SCREEN_WIDTH;
    int numRepeats = 8;

    for (int i = 0; i < numRepeats; i++) {
        int bgX = (i * backgroundWidth) - static_cast<int>(cameraX) % backgroundWidth;
        SDL_Rect dstRect = { bgX, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
        SDL_RenderCopy(renderer, mapTexture, NULL, &dstRect);
    }
}

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

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "❌ Khởi tạo SDL thất bại: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        std::cerr << "❌ Khởi tạo IMG thất bại: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() == -1) {
        std::cerr << "❌ Khởi tạo TTF thất bại: " << TTF_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "❌ Khởi tạo Mix thất bại: " << Mix_GetError() << std::endl;
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Legacy Fantacy Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "❌ Tạo cửa sổ thất bại: " << SDL_GetError() << std::endl;
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "❌ Tạo renderer thất bại: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Texture* mapTexture = IMG_LoadTexture(renderer, (ASSETS_PATH + "map.png").c_str());
    if (!mapTexture) {
        std::cerr << "❌ Không tải được map.png: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Mix_Music* backgroundMusic = Mix_LoadMUS((ASSETS_PATH + "backgroundmusic.mp3").c_str());
    if (!backgroundMusic) {
        std::cerr << "❌ Không tải được backgroundmusic.mp3: " << Mix_GetError() << std::endl;
        SDL_DestroyTexture(mapTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* startScreenTexture = IMG_LoadTexture(renderer, (ASSETS_PATH + "menu.png").c_str());
    if (!startScreenTexture) std::cerr << "❌ Không tải được menu.png: " << IMG_GetError() << std::endl;

    TTF_Font* titleFont = TTF_OpenFont((ASSETS_PATH + "Legacy.ttf").c_str(), 54);
    if (!titleFont) std::cerr << "❌ Không tải được Legacy.ttf: " << TTF_GetError() << std::endl;

    TTF_Font* startFont = TTF_OpenFont((ASSETS_PATH + "PixelFont.ttf").c_str(), 30);
    if (!startFont) std::cerr << "❌ Không tải được PixelFont.ttf: " << TTF_GetError() << std::endl;

    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Surface* titleSurface = TTF_RenderText_Blended(titleFont, "Legacy Fantasy", yellow);
    SDL_Texture* titleTexture = titleSurface ? SDL_CreateTextureFromSurface(renderer, titleSurface) : nullptr;
    SDL_Rect titleRect = titleSurface ? SDL_Rect{0, 0, titleSurface->w, titleSurface->h} : SDL_Rect{0, 0, 0, 0};
    if (titleSurface) SDL_FreeSurface(titleSurface);

    SDL_Surface* startSurface = TTF_RenderText_Blended(startFont, "S to start", yellow);
    SDL_Texture* startTexture = startSurface ? SDL_CreateTextureFromSurface(renderer, startSurface) : nullptr;
    SDL_Rect startRect = startSurface ? SDL_Rect{0, 0, startSurface->w, startSurface->h} : SDL_Rect{0, 0, 0, 0};
    if (startSurface) SDL_FreeSurface(startSurface);

    SDL_Texture* heartTexture = IMG_LoadTexture(renderer, (ASSETS_PATH + "heart.png").c_str());
    if (!heartTexture) std::cerr << "❌ Không tải được heart.png: " << IMG_GetError() << std::endl;

    int level1Map[MAP_HEIGHT][MAP_WIDTH];
    int level2Map[MAP_HEIGHT][MAP_WIDTH];
    if (!loadLevelMap(ASSETS_PATH + "level1.dat", level1Map) || !loadLevelMap(ASSETS_PATH + "level2.dat", level2Map)) {
        std::cerr << "❌ Không tải được level map. Thoát..." << std::endl;
        Mix_FreeMusic(backgroundMusic);
        SDL_DestroyTexture(mapTexture);
        SDL_DestroyTexture(startScreenTexture);
        SDL_DestroyTexture(titleTexture);
        SDL_DestroyTexture(startTexture);
        SDL_DestroyTexture(heartTexture);
        if (titleFont) TTF_CloseFont(titleFont);
        if (startFont) TTF_CloseFont(startFont);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    Camera camera;
    camera.init(MAP_WIDTH);

    TextureManager textureManager;
    textureManager.init(renderer);

    std::vector<Platform> platforms;
    std::vector<Enemy> enemies;
    std::vector<NewEnemy> newEnemies;
    std::vector<NewEnemy5> newEnemies5;
    std::vector<Boss> bosses; // Thêm vector cho Boss
    std::vector<Door> doors;
    std::vector<Item> items;
    Player player;
    player.init(renderer, 0, 0);

    Transition transition;
    transition.init();

    initializeLevel(renderer, level1Map, platforms, enemies, newEnemies, newEnemies5, bosses, doors, items, player, textureManager);

    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                bool isAnyEndScreen = false;
                for (const auto& newEnemy5 : newEnemies5) {
                    if (newEnemy5.isEndScreen) {
                        isAnyEndScreen = true;
                        break;
                    }
                }
                if (isAnyEndScreen && event.key.keysym.sym == SDLK_s) {
                    running = false;
                } else if (!isGameStarted && event.key.keysym.sym == SDLK_s) {
                    isGameStarted = true;
                    if (!musicStarted) {
                        Mix_PlayMusic(backgroundMusic, -1);
                        musicStarted = true;
                    }
                }
                if (isGameStarted && !transition.isTransitioning && !player.isDying) {
                    switch (event.key.keysym.sym) {
                        case SDLK_LEFT: player.moveLeft(); break;
                        case SDLK_RIGHT: player.moveRight(); break;
                        case SDLK_UP: player.jump(); break;
                        case SDLK_d: player.attack(); break;
                    }
                }
            }
            if (event.type == SDL_KEYUP && isGameStarted) {
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        player.isMovingLeft = false;
                        if (!player.isMovingLeft && !player.isMovingRight) player.stop();
                        break;
                    case SDLK_RIGHT:
                        player.isMovingRight = false;
                        if (!player.isMovingLeft && !player.isMovingRight) player.stop();
                        break;
                }
            }
        }

        if (isGameStarted) {
            if (!transition.isTransitioning) {
                player.update(platforms);
                if (player.shouldQuit) {
                    running = false;
                }
                camera.update(player.x);
                for (auto& enemy : enemies) {
                    enemy.update(player.x, player.y, textureManager, renderer);
                    for (auto& bullet : enemy.bullets) {
                        bullet.update();
                        SDL_Rect bulletRect = { static_cast<int>(bullet.x), static_cast<int>(bullet.y), bullet.width, bullet.height };
                        SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), player.rect.w, player.rect.h };
                        if (SDL_HasIntersection(&bulletRect, &playerRect)) {
                            player.health -= 10;
                            bullet.toRemove = true;
                            std::cout << "Bullet hit player, health: " << player.health << std::endl;
                            if (player.health <= 0 && !player.isDying) {
                                player.isDying = true;
                                player.currentFrame = 0;
                                player.deadFrameTimer = 0;
                            }
                        }
                    }
                }
                for (auto& newEnemy : newEnemies) newEnemy.update(player.x, player.y);
                for (auto& newEnemy5 : newEnemies5) newEnemy5.update();
                for (auto& boss : bosses) boss.update(player.x, player.y); // Cập nhật Boss

                // Kiểm tra va chạm với Item
                for (auto& item : items) {
                    if (!item.isCollected && SDL_HasIntersection(&player.rect, &item.rect)) {
                        item.isCollected = true;
                        player.health += 10;
                        if (player.health > player.maxHealth) {
                            player.health = player.maxHealth;
                        }
                        std::cout << "Player collected item, health: " << player.health << std::endl;
                    }
                }

                for (const auto& door : doors) {
                    if (SDL_HasIntersection(&player.rect, &door.rect)) {
                        transition.start(2);
                        break;
                    }
                }

                if (player.isAttacking) {
                    SDL_Rect attackRect = getAttackRect(player.rect, camera.x, player.facingLeft);
                    for (auto& enemy : enemies) {
                        if (enemy.isDying || enemy.isHurt) continue;
                        SDL_Rect enemyRect = { static_cast<int>(enemy.x - camera.x), static_cast<int>(enemy.y), 64, 64 };
                        if (SDL_HasIntersection(&attackRect, &enemyRect)) {
                            enemy.isHurt = true;
                            enemy.currentFrame = 0;
                            enemy.hitCount++;
                            if (enemy.hitCount >= 2) {
                                enemy.isHurt = false;
                                enemy.isDying = true;
                                enemy.currentFrame = 0;
                            }
                        }
                    }
                    for (auto& newEnemy : newEnemies) {
                        if (newEnemy.isDying || newEnemy.isHurt) continue;
                        SDL_Rect newEnemyRect = { static_cast<int>(newEnemy.x - camera.x), static_cast<int>(newEnemy.y), 64, 64 };
                        if (SDL_HasIntersection(&attackRect, &newEnemyRect)) {
                            newEnemy.isHurt = true;
                            newEnemy.currentFrame = 0;
                            newEnemy.hitCount++;
                            if (newEnemy.hitCount >= 3) {
                                newEnemy.isHurt = false;
                                newEnemy.isDying = true;
                                newEnemy.currentFrame = 0;
                            }
                        }
                    }
                    for (auto& newEnemy5 : newEnemies5) {
                        if (newEnemy5.isHit) continue;
                        SDL_Rect newEnemy5Rect = { static_cast<int>(newEnemy5.x - camera.x), static_cast<int>(newEnemy5.y), 64, 64 };
                        if (SDL_HasIntersection(&attackRect, &newEnemy5Rect)) {
                            newEnemy5.hit();
                        }
                    }
                    for (auto& boss : bosses) { // Kiểm tra va chạm tấn công của người chơi với Boss
                        if (boss.isDying || boss.isHurt) continue;
                        SDL_Rect bossRect = { static_cast<int>(boss.x - camera.x), static_cast<int>(boss.y), 64, 64 };
                        if (SDL_HasIntersection(&attackRect, &bossRect)) {
                            boss.isHurt = true;
                            boss.currentFrame = 0;
                            boss.hitCount++;
                            if (boss.hitCount >= 3) {
                                boss.isHurt = false;
                                boss.isDying = true;
                                boss.currentFrame = 0;
                            }
                        }
                    }
                }

                for (auto& enemy : enemies) {
                    if (enemy.isAttacking && !enemy.isDying && !enemy.isHurt && enemy.attackCooldown <= 0) {
                        SDL_Rect enemyAttackRect = getEnemyAttackRect(enemy);
                        SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), player.rect.w, player.rect.h };
                        if (SDL_HasIntersection(&enemyAttackRect, &playerRect)) {
                            player.health -= 15;
                            enemy.attackCooldown = enemy.attackCooldownMax;
                            std::cout << "Enemy hit player, health: " << player.health << std::endl;
                            if (player.health <= 0 && !player.isDying) {
                                player.isDying = true;
                                player.currentFrame = 0;
                                player.deadFrameTimer = 0;
                            }
                        }
                    }
                }

                for (auto& newEnemy : newEnemies) {
                    if (newEnemy.isAttacking && !newEnemy.isDying && !newEnemy.isHurt && newEnemy.attackCooldown <= 0) {
                        SDL_Rect newEnemyAttackRect = getNewEnemyAttackRect(newEnemy);
                        SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), player.rect.w, player.rect.h };
                        if (SDL_HasIntersection(&newEnemyAttackRect, &playerRect)) {
                            player.health -= 20;
                            newEnemy.attackCooldown = newEnemy.attackCooldownMax;
                            std::cout << "NewEnemy hit player, health: " << player.health << std::endl;
                            if (player.health <= 0 && !player.isDying) {
                                player.isDying = true;
                                player.currentFrame = 0;
                                player.deadFrameTimer = 0;
                            }
                        }
                    }
                }

                for (auto& boss : bosses) { // Kiểm tra va chạm tấn công của Boss với người chơi
                    if (boss.isAttacking && !boss.isDying && !boss.isHurt && boss.attackCooldown <= 0) {
                        SDL_Rect bossAttackRect = getBossAttackRect(boss);
                        SDL_Rect playerRect = { static_cast<int>(player.x), static_cast<int>(player.y), player.rect.w, player.rect.h };
                        if (SDL_HasIntersection(&bossAttackRect, &playerRect)) {
                            player.health -= 20;
                            boss.attackCooldown = boss.attackCooldownMax;
                            std::cout << "Boss hit player, health: " << player.health << std::endl;
                            if (player.health <= 0 && !player.isDying) {
                                player.isDying = true;
                                player.currentFrame = 0;
                                player.deadFrameTimer = 0;
                            }
                        }
                    }
                }

                enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                    [](const Enemy& e) { return e.toRemove; }), enemies.end());
                newEnemies.erase(std::remove_if(newEnemies.begin(), newEnemies.end(),
                    [](const NewEnemy& e) { return e.toRemove; }), newEnemies.end());
                newEnemies5.erase(std::remove_if(newEnemies5.begin(), newEnemies5.end(),
                    [](const NewEnemy5& e) { return e.toRemove; }), newEnemies5.end());
                bosses.erase(std::remove_if(bosses.begin(), bosses.end(),
                    [](const Boss& b) { return b.toRemove; }), bosses.end()); // Xóa Boss đã chết
            }

            if (transition.update()) {
                if (transition.targetLevel == 2) {
                    initializeLevel(renderer, level2Map, platforms, enemies, newEnemies, newEnemies5, bosses, doors, items, player, textureManager);
                }
            }
        } else {
            player.frameTimer++;
            if (player.frameTimer >= player.frameDelay) {
                player.frameTimer = 0;
                player.currentFrame = (player.currentFrame + 1) % 4;
            }
        }

        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderClear(renderer);

        bool isAnyEndScreen = false;
        for (const auto& newEnemy5 : newEnemies5) {
            if (newEnemy5.isEndScreen) {
                isAnyEndScreen = true;
                break;
            }
        }

        if (isAnyEndScreen) {
            for (auto& newEnemy5 : newEnemies5) {
                newEnemy5.render(renderer, camera.x);
            }
        } else if (isGameStarted) {
            renderBackground(renderer, mapTexture, camera.x, camera.mapWidthPixels);
            for (auto& door : doors) door.render(renderer, camera.x);
            for (auto& platform : platforms) platform.render(renderer, camera.x);
            for (auto& item : items) item.render(renderer, camera.x);
            player.render(renderer, camera.x);
            for (auto& enemy : enemies) enemy.render(renderer, camera.x);
            for (auto& newEnemy : newEnemies) newEnemy.render(renderer, camera.x);
            for (auto& newEnemy5 : newEnemies5) newEnemy5.render(renderer, camera.x);
            for (auto& boss : bosses) boss.render(renderer, camera.x); // Vẽ Boss
            for (auto& enemy : enemies) {
                for (auto& bullet : enemy.bullets) {
                    bullet.render(renderer, camera.x);
                }
            }

            player.renderHealthBar(renderer);
            int heartWidth = 30;
            int heartHeight = 30;
            for (int i = 0; i < player.lives; i++) {
                SDL_Rect heartRect = { 10 + i * (heartWidth + 10), 10, heartWidth, heartHeight };
                SDL_RenderCopy(renderer, heartTexture, NULL, &heartRect);
            }

            transition.render(renderer);
        } else {
            SDL_RenderCopy(renderer, startScreenTexture, NULL, NULL);
            player.rect.x = (SCREEN_WIDTH - player.rect.w) / 2;
            player.rect.y = (SCREEN_HEIGHT - player.rect.h) / 2;
            SDL_Rect srcRect = { (player.currentFrame % 4) * 64, 0, 64, 80 };
            SDL_RenderCopyEx(renderer, player.idleTexture, &srcRect, &player.rect, 0, NULL, SDL_FLIP_NONE);

            titleRect.x = (SCREEN_WIDTH - titleRect.w) / 2;
            titleRect.y = player.rect.y - titleRect.h - 10;
            SDL_RenderCopy(renderer, titleTexture, NULL, &titleRect);

            startRect.x = (SCREEN_WIDTH - startRect.w) / 2;
            startRect.y = player.rect.y + player.rect.h + 10;
            SDL_RenderCopy(renderer, startTexture, NULL, &startRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    player.cleanup();
    for (auto& platform : platforms) platform.cleanup();
    for (auto& enemy : enemies) enemy.cleanup();
    for (auto& newEnemy : newEnemies) newEnemy.cleanup();
    for (auto& newEnemy5 : newEnemies5) newEnemy5.cleanup();
    for (auto& boss : bosses) boss.cleanup(); // Dọn dẹp Boss
    for (auto& door : doors) door.cleanup();
    for (auto& item : items) item.cleanup();
    textureManager.cleanup();
    if (mapTexture) SDL_DestroyTexture(mapTexture);
    if (backgroundMusic) Mix_FreeMusic(backgroundMusic);
    if (startScreenTexture) SDL_DestroyTexture(startScreenTexture);
    if (titleTexture) SDL_DestroyTexture(titleTexture);
    if (startTexture) SDL_DestroyTexture(startTexture);
    if (heartTexture) SDL_DestroyTexture(heartTexture);
    if (titleFont) TTF_CloseFont(titleFont);
    if (startFont) TTF_CloseFont(startFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
