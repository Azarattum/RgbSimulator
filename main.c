#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "helpers/layout.h"
#include "helpers/qmk.h"
#include "helpers/text.h"

// Load custom animation
#ifdef ANIMATION_HEADER
#include ANIMATION_HEADER
#else
#error ANIMATION_HEADER must be defined
#endif

#define WINDOW_WIDTH 715
#define WINDOW_HEIGHT 260

int main() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window =
      SDL_CreateWindow("QMK RGB Simulator", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  SDL_Renderer* renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture* font_tex = load_font(renderer);

  Uint32 last_time = SDL_GetTicks();

  bool running = true;
  SDL_Event event;
  effect_params_t params = {led_colors, true, 1, 0};

  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_d:
            rgb_matrix_config.hsv.h =
                (rgb_matrix_config.hsv.h - RGB_MATRIX_HUE_STEP + 256) % 256;
            break;
          case SDLK_e:
            rgb_matrix_config.hsv.h =
                (rgb_matrix_config.hsv.h + RGB_MATRIX_HUE_STEP) % 256;
            break;
          case SDLK_f:
            rgb_matrix_config.hsv.s =
                MAX(0, rgb_matrix_config.hsv.s - RGB_MATRIX_SAT_STEP);
            break;
          case SDLK_r:
            rgb_matrix_config.hsv.s =
                MIN(255, rgb_matrix_config.hsv.s + RGB_MATRIX_SAT_STEP);
            break;
          case SDLK_s:
            rgb_matrix_config.hsv.v =
                MAX(0, rgb_matrix_config.hsv.v - RGB_MATRIX_VAL_STEP);
            break;
          case SDLK_w:
            rgb_matrix_config.hsv.v =
                MIN(255, rgb_matrix_config.hsv.v + RGB_MATRIX_VAL_STEP);
            break;
          case SDLK_g:
            rgb_matrix_config.speed =
                MAX(0, rgb_matrix_config.speed - RGB_MATRIX_SPD_STEP);
            break;
          case SDLK_t:
            rgb_matrix_config.speed =
                MIN(255, rgb_matrix_config.speed + RGB_MATRIX_SPD_STEP);
            break;
        }
      }
    }

    g_rgb_timer = SDL_GetTicks();

    ANIMATION(&params);
    params.init = false;
    params.iter += 1;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    char buf[128];
    snprintf(buf, sizeof(buf),
             "Val: %3d        Hue: %3d        Sat: %3d        Spd: %3d",
             rgb_matrix_config.hsv.v, rgb_matrix_config.hsv.h,
             rgb_matrix_config.hsv.s, rgb_matrix_config.speed);
    draw_text(renderer, font_tex, buf, 4, 4, 2);

    for (int i = 0; i < LED_COUNT; i++) {
      int x = g_led_config.point[i].x;
      int y = g_led_config.point[i].y;

      SDL_Rect rect = {x * 3 + 4, y * 3 + 32, 32, 32};
      SDL_SetRenderDrawColor(renderer, led_colors[i].r, led_colors[i].g,
                             led_colors[i].b, 255);
      SDL_RenderFillRect(renderer, &rect);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(16);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}