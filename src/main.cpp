// src/main.cpp - SDL3 FlipMan with BMP assets (player, wall, background + rotation)
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>

// Helper: load a BMP from disk and turn it into a texture
SDL_Texture* LoadBMPTexture(SDL_Renderer* renderer, const char* path)
{
    SDL_Surface* surf = SDL_LoadBMP(path);
    if (!surf) {
        std::cerr << "SDL_LoadBMP failed for '" << path << "': "
                  << SDL_GetError() << "\n";
        return nullptr;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tex) {
        std::cerr << "SDL_CreateTextureFromSurface failed for '" << path
                  << "': " << SDL_GetError() << "\n";
    }

    SDL_DestroySurface(surf); // SDL3: destroy surface
    return tex;
}

int main(int argc, char** argv)
{
    std::cout << "SDL3 FlipMan + BMP assets + rotation: start\n";

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init error: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Flip Man - SDL3 (BMP Assets + Rotation)",
                                          800, 600, 0);
    if (!window) {
        std::cerr << "SDL_CreateWindow error: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(window, nullptr);
    if (!ren) {
        std::cerr << "SDL_CreateRenderer error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // ------------------------------------------------------------------
    // Load textures (BMP) from ../assets/
    // ------------------------------------------------------------------
    SDL_Texture* texPlayer = LoadBMPTexture(ren, "../assets/player.bmp");
    SDL_Texture* texWall   = LoadBMPTexture(ren, "../assets/wall.bmp");
    SDL_Texture* texBg     = LoadBMPTexture(ren, "../assets/background.bmp"); // optional

    if (!texPlayer) std::cout << "player.bmp missing, using green rect.\n";
    if (!texWall)   std::cout << "wall.bmp missing, using gray rects.\n";
    if (!texBg)     std::cout << "background.bmp missing, using solid color.\n";

    // ------------------------------------------------------------------
    // Player / physics
    // ------------------------------------------------------------------
    SDL_FRect player{ 380.f, 520.f, 40.f, 60.f }; // x, y, w, h

    float vx = 0.f;
    float vy = 0.f;

    const float gravity    = 900.f; // constant magnitude
    float       gravityDir = 1.f;   // +1 = gravity down, -1 = gravity up

    const float moveSpeed  = 240.f;

    // ------------------------------------------------------------------
    // Player rotation (for flip animation)
    // ------------------------------------------------------------------
    float playerAngle      = 0.f;   // current angle in degrees
    float targetAngle      = 0.f;   // target angle (0 or 180)
    const float angleSpeed = 720.f; // degrees per second (how fast we rotate)

    // ------------------------------------------------------------------
    // Walls: floor, ceiling, and two platforms
    // ------------------------------------------------------------------
    std::vector<SDL_FRect> walls;
    const float tileW = 64.f;
    const float tileH = 40.f;

    // Floor (bottom of screen)
    for (float x = 0.f; x < 800.f; x += tileW) {
        walls.push_back(SDL_FRect{ x, 600.f - tileH, tileW, tileH });
    }

    // Ceiling (top of screen)
    for (float x = 0.f; x < 800.f; x += tileW) {
        walls.push_back(SDL_FRect{ x, 0.f, tileW, tileH });
    }

    // Platforms (middle of level)
    walls.push_back(SDL_FRect{ 200.f, 600.f - 160.f, 128.f, 32.f });
    walls.push_back(SDL_FRect{ 500.f, 600.f - 260.f, 128.f, 32.f });

    Uint64 lastTicks = SDL_GetTicks();
    bool running = true;

    std::cout << "Window created, entering main loop.\n";

    while (running) {
        // ---------------- Input ----------------
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE && e.key.down) {
                    running = false;
                }
                if (e.key.key == SDLK_SPACE && e.key.down) {
                    // Flip gravity direction
                    gravityDir *= -1.f;

                    // Reset vertical velocity to avoid weird residual speeds.
                    vy = 0.f;

                    // Set target angle based on new gravity direction:
                    // gravity down  -> upright (0°)
                    // gravity up    -> upside down (180°)
                    targetAngle = (gravityDir > 0.f) ? 0.f : 180.f;

                    std::cout << "Gravity flipped. Now "
                              << (gravityDir > 0 ? "DOWN, " : "UP, ")
                              << "targetAngle = " << targetAngle << " deg\n";
                }
            }
        }

        int numKeys = 0;
        const bool* kb = SDL_GetKeyboardState(&numKeys);
        vx = 0.f;
        if (kb[SDL_SCANCODE_A] || kb[SDL_SCANCODE_LEFT])  vx = -moveSpeed;
        if (kb[SDL_SCANCODE_D] || kb[SDL_SCANCODE_RIGHT]) vx =  moveSpeed;

        // ---------------- Update ----------------
        Uint64 nowTicks = SDL_GetTicks();
        float dt = (nowTicks - lastTicks) / 1000.0f;
        lastTicks = nowTicks;

        // Safety clamp if the frame spikes
        if (dt > 0.05f) dt = 0.05f;

        // Animate rotation: move playerAngle toward targetAngle
        if (playerAngle < targetAngle) {
            playerAngle += angleSpeed * dt;
            if (playerAngle > targetAngle) playerAngle = targetAngle;
        } else if (playerAngle > targetAngle) {
            playerAngle -= angleSpeed * dt;
            if (playerAngle < targetAngle) playerAngle = targetAngle;
        }

        // Apply gravity
        vy += gravity * gravityDir * dt;

        // Save previous position before moving (for directional collision)
        float oldX = player.x;
        float oldY = player.y;

        // Move
        player.x += vx * dt;
        player.y += vy * dt;

        // ---------------- Collision handling ----------------
        for (const auto& w : walls) {
            if (SDL_HasRectIntersectionFloat(&player, &w)) {
                float wallTop    = w.y;
                float wallBottom = w.y + w.h;
                float wallLeft   = w.x;
                float wallRight  = w.x + w.w;

                float overlapLeft   = (player.x + player.w) - wallLeft;
                float overlapRight  = wallRight - player.x;
                float overlapTop    = (player.y + player.h) - wallTop;
                float overlapBottom = wallBottom - player.y;

                float minHoriz = std::min(overlapLeft, overlapRight);
                float minVert  = std::min(overlapTop, overlapBottom);

                if (minVert < minHoriz) {
                    // Resolve vertically based on movement direction
                    if (player.y > oldY) {
                        // We moved DOWN into the wall -> snap to top
                        player.y = wallTop - player.h;
                        vy = 0.f;
                    } else if (player.y < oldY) {
                        // We moved UP into the wall -> snap to bottom
                        player.y = wallBottom;
                        vy = 0.f;
                    }
                } else {
                    // Resolve horizontally
                    if (player.x > oldX) {
                        // moved right
                        player.x = wallLeft - player.w;
                    } else if (player.x < oldX) {
                        // moved left
                        player.x = wallRight;
                    }
                    vx = 0.f;
                }
            }
        }

        // Clamp horizontally within the screen
        if (player.x < 0.f) player.x = 0.f;
        if (player.x + player.w > 800.f) player.x = 800.f - player.w;

        // ---------------- Render ----------------
        if (!texBg) {
            SDL_SetRenderDrawColor(ren, 18, 18, 28, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(ren);
        } else {
            SDL_FRect bgRect{ 0.f, 0.f, 800.f, 600.f };
            SDL_RenderTexture(ren, texBg, nullptr, &bgRect);
        }

        // Walls
        if (texWall) {
            for (const auto& w : walls) {
                SDL_RenderTexture(ren, texWall, nullptr, &w);
            }
        } else {
            SDL_SetRenderDrawColor(ren, 120, 120, 120, SDL_ALPHA_OPAQUE);
            for (const auto& w : walls) {
                SDL_RenderFillRect(ren, &w);
            }
        }

        // Player (rotated)
        if (texPlayer) {
            SDL_FPoint center{ player.w / 2.0f, player.h / 2.0f }; // rotate around center
            SDL_RenderTextureRotated(
                ren,
                texPlayer,
                nullptr,      // full source texture
                &player,      // destination rect
                playerAngle,  // angle in degrees
                &center,
                SDL_FLIP_NONE // no extra flip
            );
        } else {
            // Fallback: no rotation for solid rect, just draw
            SDL_SetRenderDrawColor(ren, 0, 200, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(ren, &player);
        }

        SDL_RenderPresent(ren);
    }

    // Cleanup
    if (texPlayer) SDL_DestroyTexture(texPlayer);
    if (texWall)   SDL_DestroyTexture(texWall);
    if (texBg)     SDL_DestroyTexture(texBg);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "SDL3 FlipMan + BMP assets + rotation: exit\n";
    return 0;
}
