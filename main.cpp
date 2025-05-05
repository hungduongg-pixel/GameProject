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
#include "const.h"
#include "open.h"
#include "camera.h"
#include "platform.h"
#include "door.h"
#include "bullet.h"
#include "item.h"
#include "newenemy5.h"
#include "enemy.h"
#include "newenemy4.h"
#include "player.h"
#include "boss.h"
#include "map.h"

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

SDL_Rect getAttackRect(SDL_Rect playerRect, float cameraX, bool facingLeft) {
    SDL_Rect attackRect = playerRect;
    attackRect.x -= static_cast<int>(cameraX);
    attackRect.w = 10;
    attackRect.h = playerRect.h / 2;
    if (facingLeft) attackRect.x -= 30;
    else attackRect.x += playerRect.w + 5;
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
    SDL_Rect attackRect = { static_cast<int>(newEnemy.x), static_cast<int>(newEnemy.y), 40, 40 };
    if (newEnemy.movingRight) {
        attackRect.x += 15;
    } else {
        attackRect.x -= 60;
    }
    return attackRect;
}

SDL_Rect getBossAttackRect(const Boss& boss) {
    int newWidth = static_cast<int>(60 * boss.scale);
    int newHeight = static_cast<int>(60 * boss.scale);
    SDL_Rect attackRect = { static_cast<int>(boss.x), static_cast<int>(boss.y), newWidth, newHeight };
    if (boss.movingRight) {
        attackRect.x += static_cast<int>(15 * boss.scale);
    } else {
        attackRect.x -= static_cast<int>(60 * boss.scale);
    }
    return attackRect;
}

void initializeLevel(SDL_Renderer* renderer, const int (&levelMap)[MAP_HEIGHT][MAP_WIDTH],
                     std::vector<Platform>& platforms, std::vector<Enemy>& enemies,
                     std::vector<NewEnemy>& newEnemies, std::vector<NewEnemy5>& newEnemies5,
                     std::vector<Boss>& bosses, std::vector<Door>& doors,
                     std::vector<Item>& items, Player& player, TextureManager& textureManager) {
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
                if (enemyType == 4 || enemyType == 8) {
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
    std::vector<Boss> bosses;
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
                for (auto& boss : bosses) boss.update(player.x, player.y, camera);

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
                    for (auto& boss : bosses) {
                        if (boss.isDying || boss.isHurt) continue;
                        SDL_Rect bossRect = { static_cast<int>(boss.x - camera.x), static_cast<int>(boss.y), static_cast<int>(288 * boss.scale), static_cast<int>(118 * boss.scale) };
                        if (SDL_HasIntersection(&attackRect, &bossRect)) {
                            boss.health -= 5; // Giảm 5 máu khi bị tấn công
                            boss.isHurt = true;
                            boss.currentFrame = 0;
                            std::cout << "Player hit boss, boss health: " << boss.health << std::endl;
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

                for (auto& boss : bosses) {
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
                    [](const Boss& b) { return b.toRemove; }), bosses.end());
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
            for (auto& boss : bosses) boss.render(renderer, camera.x);
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

            for (auto& boss : bosses) {
                boss.renderHealthBar(renderer, camera.x);
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
    for (auto& boss : bosses) boss.cleanup();
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
