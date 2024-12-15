#include "pch.h"
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <cmath>

struct Food {
    SDL_Rect body;
    bool alive;
    bool food; // normal food
    bool isPoisonous; // Flag for poisoned food
    bool increasesSpeed; // Flag for speed increasing food
    bool createsShield;  // Flag for food creating shield
    double dietime;
};

const int respawn_time = 1500; // Time for food respawn in milliseconds
const int speed_boost_duration = 5000; // Duration of the increased speed in milliseconds
const int shield_duration = 5000; // Shield duration in milliseconds
const int current_time = 5000;
bool isFullscreen = true;

// Texture loading function
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* filename) {
    SDL_Surface* tempSurface = IMG_Load(filename);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);
    return texture;
}

// Interference check
bool intersect(int x1, int y1, int x2, int y2, int r1, int r2) {
    double distance = std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
    return distance < (r1 + r2);
}

// Function for drawing a circle
void drawCircle(SDL_Renderer* renderer, int x, int y, int radius) {
    int cx = radius - 1;
    int cy = 0;
    int radiusError = 1 - cx;

    while (cx >= cy) {
        SDL_RenderDrawPoint(renderer, x + cx, y - cy);
        SDL_RenderDrawPoint(renderer, x + cx, y + cy);
        SDL_RenderDrawPoint(renderer, x - cx, y - cy);
        SDL_RenderDrawPoint(renderer, x - cx, y + cy);
        SDL_RenderDrawPoint(renderer, x + cy, y - cx);
        SDL_RenderDrawPoint(renderer, x + cy, y + cx);
        SDL_RenderDrawPoint(renderer, x - cy, y - cx);
        SDL_RenderDrawPoint(renderer, x - cy, y + cx);

        cy++;

        if (radiusError < 0) {
            radiusError += 2 * cy + 1;
        }
        else {
            cx--;
            radiusError += 2 * (cy - cx + 1);
        }
    }
}

Uint32 player1_shield_break_time = 0;
Uint32 player2_shield_break_time = 0;
// Variables for resetting state after death and rebirth
void respawnPlayer(bool& player_alive, int& x, int& y, Uint32& death_time, Uint32& shield_break_time, bool& shieldboost, int respawn_x, int respawn_y) {
    // Возрождение игрока
    if (!player_alive && SDL_GetTicks() - death_time > 3000) { // Respawn time = 3 seconds
        player_alive = true;
        x = respawn_x; // New coordinates
        y = respawn_y;
        shieldboost = false; // The shield is reset
        shield_break_time = 0; // Shield removal timer resets
    }
}


int main(int argc, char** argv) {
    // Initializing SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << std::endl;
        return -1;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0 || TTF_Init() != 0) {
        std::cerr << "SDL_image/SDL_ttf init error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Creating a Window and Renderer
    SDL_Window* window = SDL_CreateWindow("SDL2 Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1500, 1000, SDL_WINDOW_FULLSCREEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    // Loading textures
    SDL_Texture* player1Texture = loadTexture(renderer, "player1.png");
    SDL_Texture* player2Texture = loadTexture(renderer, "player2.png");
    SDL_Texture* backgroundTexture = loadTexture(renderer, "background.png");
    SDL_Texture* foodpoisonTexture = loadTexture(renderer, "poison_food.png");
    SDL_Texture* foodnormalTexture = loadTexture(renderer, "normal_food.png");
    SDL_Texture* speedFoodTexture = loadTexture(renderer, "speed_food.png");
    SDL_Texture* shieldFoodTexture = loadTexture(renderer, "shield_food.png");

    // Player Variables
    int x1 = 100, y1 = 150, w1 = 180, h1 = 180; // player 1
    int x2 = 300, y2 = 250, w2 = 170, h2 = 170; // player 2
    bool player1_alive = true, player2_alive = true;
    Uint32 player1_death_time = 0, player2_death_time = 0; // Time of death of players
    bool player1_left = false, player1_right = false, player1_up = false, player1_down = false;
    bool player2_left = false, player2_right = false, player2_up = false, player2_down = false;

    // Variables for acceleration and shield
    bool player1_speedBoost = false, player2_speedBoost = false;
    Uint32 player1_speedBoostTime = 0, player2_speedBoostTime = 0;
    bool player1_shieldboost = false, player2_shieldboost = false;
    Uint32 player1_shieldboostTime = 0, player2_shieldboostTime = 0;

    // Array of food
    const int FoodCount = 20;
    Food food[FoodCount];
    for (int i = 0; i < FoodCount; i++) {
        food[i].body = { rand() % 1400, rand() % 900, 40, 40 };
        food[i].alive = true;
        food[i].food = rand() % 2 == 0;
        food[i].isPoisonous = rand() % 4 == 0;  // 1 in 5 chance the food is poisoned
        food[i].increasesSpeed = rand() % 6 == 0; // 1 in 6 chance that food increases speed
        food[i].createsShield = rand() % 8 == 0;  // 1 in 8 chance that food creates a shield
    }

    //Main game loop
    bool exit = false;
    SDL_Event event;
    while (!exit) {
        // Event Handling
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                exit = true;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_a: player1_left = true; break;
                case SDLK_d: player1_right = true; break;
                case SDLK_w: player1_up = true; break;
                case SDLK_s: player1_down = true; break;
                case SDLK_LEFT: player2_left = true; break;
                case SDLK_RIGHT: player2_right = true; break;
                case SDLK_UP: player2_up = true; break;
                case SDLK_DOWN: player2_down = true; break;

                case SDLK_f:
                    isFullscreen = !isFullscreen;
                    SDL_SetWindowFullscreen(window, isFullscreen ? SDL_WINDOW_FULLSCREEN : 0);
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                case SDLK_a: player1_left = false; break;
                case SDLK_d: player1_right = false; break;
                case SDLK_w: player1_up = false; break;
                case SDLK_s: player1_down = false; break;
                case SDLK_LEFT: player2_left = false; break;
                case SDLK_RIGHT: player2_right = false; break;
                case SDLK_UP: player2_up = false; break;
                case SDLK_DOWN: player2_down = false; break;
                }
                break;
            }
        }

        // Handling player movement
        if (player1_left) x1 -= 10;
        if (player1_right) x1 += 10;
        if (player1_up) y1 -= 10;
        if (player1_down) y1 += 10;

        if (player2_left) x2 -= 10;
        if (player2_right) x2 += 10;
        if (player2_up) y2 -= 10;
        if (player2_down) y2 += 10;

        // Restricting player movement to the screen
        x1 = std::max(0, std::min(1500 - w1, x1));
        y1 = std::max(0, std::min(1000 - h1, y1));
        x2 = std::max(0, std::min(1500 - w2, x2));
        y2 = std::max(0, std::min(1000 - h2, y2));

        for (int i = 0; i < FoodCount; i++) {
            if (food[i].alive) {
                SDL_Texture* currentFoodTexture = food[i].increasesSpeed ? speedFoodTexture : foodpoisonTexture;
                SDL_RenderCopy(renderer, currentFoodTexture, nullptr, &food[i].body);
            }
            else if (SDL_GetTicks() - food[i].dietime >= 10000) {
                food[i].body.x = rand() % 1400;
                food[i].body.y = rand() % 900;
                food[i].alive = true;
                food[i].isPoisonous = rand() % 4 == 0;
                food[i].increasesSpeed = rand() % 6 == 0;
                food[i].createsShield = rand() % 8 == 0;
                food[i].food = rand() % 2 == 0;
            }
        }





        // Check for interference with food
        for (int i = 0; i < FoodCount; i++) {
            if (food[i].alive && intersect(x1 + w1 / 2, y1 + h1 / 2, food[i].body.x + food[i].body.w / 2, food[i].body.y + food[i].body.h / 2, w1 / 2, food[i].body.w / 2)) {
                if (food[i].isPoisonous) {
                    w1 -= 10; h1 -= 10; // Reducing size for poisoned food
                }
                else if (food[i].increasesSpeed) {
                    player1_speedBoost = true;
                    player1_speedBoostTime = SDL_GetTicks();
                }
                else if (food[i].createsShield) {
                    player1_shieldboost = true;
                    player1_shieldboostTime = SDL_GetTicks();
                }
                else if (food[i].food)
                {
                    h1 += 10;
                    w1 += 10;
                }
                food[i].alive = false;
            }

            if (food[i].alive && intersect(x2 + w2 / 2, y2 + h2 / 2, food[i].body.x + food[i].body.w / 2, food[i].body.y + food[i].body.h / 2, w2 / 2, food[i].body.w / 2)) {
                if (food[i].isPoisonous) {
                    w2 -= 10; h2 -= 10; // Reducing size for poisoned food
                }
                else if (food[i].increasesSpeed) {
                    player2_speedBoost = true;
                    player2_speedBoostTime = SDL_GetTicks();
                }
                else if (food[i].createsShield) {
                    player2_shieldboost = true;
                    player2_shieldboostTime = SDL_GetTicks();
                }
                else if (food[i].food)
                {
                    h2 += 10;
                    w2 += 10;
                }
                food[i].alive = false;
            }
        }

        respawnPlayer(player1_alive, x1, y1, player1_death_time, player1_shield_break_time, player1_shieldboost, 100, 100);
        respawnPlayer(player2_alive, x2, y2, player2_death_time, player2_shield_break_time, player2_shieldboost, 400, 400);

        if (intersect(x1 + w1 / 2, y1 + h1 / 2, x2 + w2 / 2, y2 + h2 / 2, w1 / 2, w2 / 2)) {
            if (w1 > w2 && h1 > h2 && (!player2_shieldboost || SDL_GetTicks() - player2_shieldboostTime >= shield_duration)) {
                player2_alive = false;
                player2_death_time = SDL_GetTicks();
                x2 = -1000; // Removing player 2 from the screen
                y2 = -1000;
            }
            else if (w2 > w1 && h2 > h1 && (!player1_shieldboost || SDL_GetTicks() - player1_shieldboostTime >= shield_duration)) {
                player1_alive = false;
                player1_death_time = SDL_GetTicks();
                x1 = -1000; // Removing player 1 from the screen
                y1 = -1000;
            }
        }

        // Players Revival
        if (!player1_alive && SDL_GetTicks() - player1_death_time >= respawn_time) {
            player1_alive = true;
            h1 = 170;
            w1 = 170;
            h2 = 170;
            w2 = 170;
            x1 = rand() % (1500 - w1);
            y1 = rand() % (1000 - h1);

        }

        if (!player2_alive && SDL_GetTicks() - player2_death_time >= respawn_time) {
            player2_alive = true;
            h1 = 170;
            w1 = 170;
            h2 = 170;
            w2 = 170;
            x2 = rand() % (1500 - w2);
            y2 = rand() % (1000 - h2);

        }



        // Background rendering
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);

        //Food display
        for (int i = 0; i < FoodCount; i++) {
            if (food[i].alive) {
                if (food[i].isPoisonous)
                    SDL_RenderCopy(renderer, foodpoisonTexture, nullptr, &food[i].body);
                else if (food[i].increasesSpeed)
                    SDL_RenderCopy(renderer, speedFoodTexture, nullptr, &food[i].body);
                else if (food[i].createsShield)
                    SDL_RenderCopy(renderer, shieldFoodTexture, nullptr, &food[i].body);
                else if (food[i].food)
                    SDL_RenderCopy(renderer, foodnormalTexture, nullptr, &food[i].body);
            }
        }


        // Shield display for player 1
        if (player1_shieldboost && SDL_GetTicks() - player1_shieldboostTime < shield_duration) {
            SDL_SetRenderDrawColor(renderer, 255, 165, 0, 0); // orange color for shield
            drawCircle(renderer, x1 + w1 / 2, y1 + h1 / 2, w1 / 2 + 10);
        }

        // Shield display for player 2
        if (player2_shieldboost && SDL_GetTicks() - player2_shieldboostTime < shield_duration) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for shield
            drawCircle(renderer, x2 + w2 / 2, y2 + h2 / 2, w2 / 2 + 10);
        }


        // Player display
        if (player1_alive) {
            SDL_Rect target1 = { x1, y1, w1, h1 };
            SDL_RenderCopy(renderer, player1Texture, nullptr, &target1);
        }

        if (player2_alive) {
            SDL_Rect target2 = { x2, y2, w2, h2 };
            SDL_RenderCopy(renderer, player2Texture, nullptr, &target2);
        }

        SDL_RenderPresent(renderer);

        SDL_Delay(16);  // Latency for approximately 60 FPS
    }

    // Cleaning

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}