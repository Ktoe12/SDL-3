#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <cmath>
#include <SDL3/SDL.h>

// Window dimensions
constexpr int WINDOW_WIDTH{700};
constexpr int WINDOW_HEIGHT{800};

// SDL Window related variables
SDL_Window *window{nullptr};
SDL_Renderer *renderer{nullptr};
SDL_Event currentEvent;
SDL_Color backgroundColor{255, 255, 255, 255};
bool quit{false};
float displayScale{1.0f};

// Game States
enum class GameState {
    PLAYING,
    WON,
    LOST
};
GameState currentState = GameState::PLAYING;
float globalGameTime = 0.0f;

// Player variables
float playerX = WINDOW_WIDTH / 2.0f;
float playerY = WINDOW_HEIGHT - 25.0f;
constexpr float PLAYER_SIZE = 50.0f;
bool isMouseLeftPressed = false;
float fireTimer = 0.3f;

// Entities
struct PlayerProjectile {
    float x, y, radius, velocity;
};

struct EnemyProjectile {
    float x, y, radius, velocityX, velocityY;
};

struct Enemy {
    float x, y, fireTimer;
    bool active;
};

std::vector<PlayerProjectile> playerProjectiles;
std::vector<EnemyProjectile> enemyProjectiles;
std::vector<Enemy> enemies;

// Helper: Desenare cerc plin
void DrawFillCircle(SDL_Renderer* rend, float centerX, float centerY, float radius) {
    int offsetx = 0;
    int offsety = static_cast<int>(radius);
    int status = 0;

    while (offsety >= offsetx) {
        SDL_RenderLine(rend, centerX - offsety, centerY + offsetx, centerX + offsety, centerY + offsetx);
        SDL_RenderLine(rend, centerX - offsetx, centerY + offsety, centerX + offsetx, centerY + offsety);
        SDL_RenderLine(rend, centerX - offsetx, centerY - offsety, centerX + offsetx, centerY - offsety);
        SDL_RenderLine(rend, centerX - offsety, centerY - offsetx, centerX + offsety, centerY - offsetx);

        if (status >= 0) {
            offsety -= 1;
            status -= 2 * offsety;
        }
        offsetx += 1;
        status += 2 * offsetx + 1;
    }
}

// Coliziuni: Cerc vs Dreptunghi
bool checkCircleRectCollision(float cx, float cy, float cr, float rx, float ry, float rw, float rh) {
    float closestX = std::clamp(cx, rx, rx + rw);
    float closestY = std::clamp(cy, ry, ry + rh);
    float dx = cx - closestX;
    float dy = cy - closestY;
    return (dx * dx + dy * dy) <= (cr * cr);
}

// Coliziuni: Dreptunghi vs Dreptunghi
bool checkRectRectCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

// Spawn inamici
void initEnemies() {
    std::mt19937 rng(std::random_device{}());
    int numEnemies = 3;
    float sliceWidth = WINDOW_WIDTH / static_cast<float>(numEnemies);
    float halfSize = PLAYER_SIZE / 2.0f;
    float maxY = (WINDOW_HEIGHT / 2.0f) - halfSize;

    enemies.clear();
    for (int i = 0; i < numEnemies; ++i) {
        float minX_for_slice = (i * sliceWidth) + halfSize;
        float maxX_for_slice = ((i + 1) * sliceWidth) - halfSize;

        std::uniform_real_distribution<float> distX(minX_for_slice, maxX_for_slice);
        std::uniform_real_distribution<float> distY(halfSize, maxY);

        enemies.push_back({distX(rng), distY(rng), 0.0f, true});
    }
}

void resetGame() {
    currentState = GameState::PLAYING;
    globalGameTime = 0.0f;
    fireTimer = 0.3f;
    isMouseLeftPressed = false;
    playerProjectiles.clear();
    enemyProjectiles.clear();

    playerX = WINDOW_WIDTH / 2.0f;
    playerY = WINDOW_HEIGHT - 25.0f;

    initEnemies();
}

bool initWindow() {
    bool success = true;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        success = false;
    } else {
        displayScale = SDL_GetDisplayContentScale(1);
        if (!SDL_CreateWindowAndRenderer("SDL3 Space Shooter", static_cast<int>(displayScale * WINDOW_WIDTH), static_cast<int>(displayScale * WINDOW_HEIGHT), 0, &window, &renderer)) {
            success = false;
        } else {
            SDL_SetRenderScale(renderer, displayScale, displayScale);
            SDL_SetWindowRelativeMouseMode(window, true);
        }
    }
    return success;
}

void processEvents() {
    while (SDL_PollEvent(&currentEvent)) {
        if (currentEvent.type == SDL_EVENT_QUIT) quit = true;

        if (currentEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN && currentEvent.button.button == SDL_BUTTON_LEFT) {
            isMouseLeftPressed = true;
        }
        if (currentEvent.type == SDL_EVENT_MOUSE_BUTTON_UP && currentEvent.button.button == SDL_BUTTON_LEFT) {
            isMouseLeftPressed = false;
            fireTimer = 0.3f;
        }

        // Miscare nava doar in modul PLAYING
        if (currentEvent.type == SDL_EVENT_MOUSE_MOTION && currentState == GameState::PLAYING) {
            playerX += currentEvent.motion.xrel / displayScale;
            playerY += currentEvent.motion.yrel / displayScale;

            float halfSize = PLAYER_SIZE / 2.0f;
            if (playerX < halfSize) playerX = halfSize;
            if (playerX > WINDOW_WIDTH - halfSize) playerX = WINDOW_WIDTH - halfSize;
            if (playerY < halfSize) playerY = halfSize;
            if (playerY > WINDOW_HEIGHT - halfSize) playerY = WINDOW_HEIGHT - halfSize;
        }

        if (currentEvent.type == SDL_EVENT_KEY_DOWN) {
            if (currentEvent.key.key == SDLK_ESCAPE) quit = true;
            if (currentEvent.key.key == SDLK_R && currentState != GameState::PLAYING) {
                resetGame(); // Resetare permisă doar la finalul jocului
            }
        }
    }
}

void updateLogic(float dt) {
    if (currentState != GameState::PLAYING) return;

    globalGameTime += dt;

    // --- Tragere Player ---
    if (isMouseLeftPressed) {
        fireTimer += dt;
        if (fireTimer >= 0.3f) {
            playerProjectiles.push_back({playerX, playerY, 10.0f, 500.0f});
            fireTimer -= 0.3f;
        }
    }

    // Mișcare Proiectile Player
    for (auto& proj : playerProjectiles) proj.y -= proj.velocity * dt;

    // Stergere proiectile Player ieșite din ecran
    playerProjectiles.erase(
        std::remove_if(playerProjectiles.begin(), playerProjectiles.end(),
            [](const PlayerProjectile& p) { return p.y + p.radius < 0; }),
        playerProjectiles.end()
    );

    // --- Tragere Inamici ---
    if (globalGameTime >= 1.0f) {
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            enemy.fireTimer += dt;
            if (enemy.fireTimer >= 3.0f) {
                enemy.fireTimer -= 3.0f;
                float dx = playerX - enemy.x;
                float dy = playerY - enemy.y;
                float dist = std::sqrt(dx * dx + dy * dy);

                enemyProjectiles.push_back({enemy.x, enemy.y, 10.0f, (dx / dist) * 100.0f, (dy / dist) * 100.0f});
            }
        }
    }

    // Mișcare Proiectile Inamice
    for (auto& proj : enemyProjectiles) {
        proj.x += proj.velocityX * dt;
        proj.y += proj.velocityY * dt;
    }

    // Curățare Proiectile Inamice
    enemyProjectiles.erase(
        std::remove_if(enemyProjectiles.begin(), enemyProjectiles.end(),
            [](const EnemyProjectile& p) {
                return p.y - p.radius > WINDOW_HEIGHT || p.y + p.radius < 0 || p.x - p.radius > WINDOW_WIDTH || p.x + p.radius < 0;
            }),
        enemyProjectiles.end()
    );

    // --- Sistem de Coliziuni ---
    float pLeft = playerX - PLAYER_SIZE / 2.0f;
    float pTop = playerY - PLAYER_SIZE / 2.0f;

    // 1. Proiectil Player vs Inamic
    for (auto itP = playerProjectiles.begin(); itP != playerProjectiles.end(); ) {
        bool projectileDestroyed = false;
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            float eLeft = enemy.x - PLAYER_SIZE / 2.0f;
            float eTop = enemy.y - PLAYER_SIZE / 2.0f;

            if (checkCircleRectCollision(itP->x, itP->y, itP->radius, eLeft, eTop, PLAYER_SIZE, PLAYER_SIZE)) {
                enemy.active = false;
                projectileDestroyed = true;
                break;
            }
        }
        if (projectileDestroyed) itP = playerProjectiles.erase(itP);
        else ++itP;
    }

    // 2. Proiectil Inamic vs Player
    for (const auto& proj : enemyProjectiles) {
        if (checkCircleRectCollision(proj.x, proj.y, proj.radius, pLeft, pTop, PLAYER_SIZE, PLAYER_SIZE)) {
            currentState = GameState::LOST;
        }
    }

    // 3. Inamic vs Player
    bool allEnemiesDead = true;
    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;
        allEnemiesDead = false;

        float eLeft = enemy.x - PLAYER_SIZE / 2.0f;
        float eTop = enemy.y - PLAYER_SIZE / 2.0f;
        if (checkRectRectCollision(pLeft, pTop, PLAYER_SIZE, PLAYER_SIZE, eLeft, eTop, PLAYER_SIZE, PLAYER_SIZE)) {
            currentState = GameState::LOST;
        }
    }

    if (allEnemiesDead) currentState = GameState::WON;

    // --- Curățare Entități la Game Over ---
    if (currentState != GameState::PLAYING) {
        playerProjectiles.clear();
        enemyProjectiles.clear();
        enemies.clear();
    }
}

void drawFrame() {
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderClear(renderer);

    // Desenare Inamici (cu contur negru)
    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;
        SDL_FRect enemyRect = { enemy.x - PLAYER_SIZE / 2.0f, enemy.y - PLAYER_SIZE / 2.0f, PLAYER_SIZE, PLAYER_SIZE };

        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); // Portocaliu
        SDL_RenderFillRect(renderer, &enemyRect);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Contur Negru
        SDL_RenderRect(renderer, &enemyRect);
    }

    // Desenare Player (cu contur negru și stări de Win/Loss)
    SDL_FRect playerRect = { playerX - PLAYER_SIZE / 2.0f, playerY - PLAYER_SIZE / 2.0f, PLAYER_SIZE, PLAYER_SIZE };

    if (currentState == GameState::PLAYING) SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Verde
    else if (currentState == GameState::WON) SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Albastru
    else if (currentState == GameState::LOST) SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Rosu

    SDL_RenderFillRect(renderer, &playerRect);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Contur Negru
    SDL_RenderRect(renderer, &playerRect);

    // Desenare Proiectile Inamice (Magenta)
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    for (const auto& proj : enemyProjectiles) DrawFillCircle(renderer, proj.x, proj.y, proj.radius);

    // Desenare Proiectile Player (Roșu)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (const auto& proj : playerProjectiles) DrawFillCircle(renderer, proj.x, proj.y, proj.radius);

    SDL_RenderPresent(renderer);
}

void cleanup() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    if (!initWindow()) return -1;

    initEnemies();
    SDL_zero(currentEvent);
    Uint64 lastTime = SDL_GetTicks();

    while (!quit) {
        Uint64 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        processEvents();
        updateLogic(dt);
        drawFrame();
    }

    cleanup();
    return 0;
}