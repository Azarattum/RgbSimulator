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

void process_hits() {
  uint16_t max_tick =
      (rgb_matrix_config.speed == 0) ? 0 : 65535 / rgb_matrix_config.speed;

  for (int i = 0; i < g_last_hit_tracker.count;) {
    g_last_hit_tracker.tick[i] += rgb_matrix_config.speed;

    if (g_last_hit_tracker.tick[i] > max_tick) {
      for (int j = i; j < g_last_hit_tracker.count - 1; j++) {
        g_last_hit_tracker.x[j] = g_last_hit_tracker.x[j + 1];
        g_last_hit_tracker.y[j] = g_last_hit_tracker.y[j + 1];
        g_last_hit_tracker.index[j] = g_last_hit_tracker.index[j + 1];
        g_last_hit_tracker.tick[j] = g_last_hit_tracker.tick[j + 1];
      }
      g_last_hit_tracker.count--;
    } else {
      i++;
    }
  }
}

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
      if (event.type == SDL_MOUSEBUTTONDOWN) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        for (int i = 0; i < LED_COUNT; i++) {
          int led_x = g_led_config.point[i].x * 3 + 4;
          int led_y = g_led_config.point[i].y * 3 + 32;
          SDL_Rect rect = {led_x, led_y, 32, 32};

          if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &rect)) {
            if (g_last_hit_tracker.count < LED_HITS_TO_REMEMBER) {
              g_last_hit_tracker.x[g_last_hit_tracker.count] =
                  g_led_config.point[i].x;
              g_last_hit_tracker.y[g_last_hit_tracker.count] =
                  g_led_config.point[i].y;
              g_last_hit_tracker.index[g_last_hit_tracker.count] = i;
              g_last_hit_tracker.tick[g_last_hit_tracker.count] = 0;

              g_last_hit_tracker.count++;
            }

#ifdef ENABLE_RGB_MATRIX_TYPING_HEATMAP
            for (int row = 0; row < MATRIX_ROWS; row++) {
              for (int col = 0; col < MATRIX_COLS; col++) {
                if (g_led_config.matrix_co[row][col] == i) {
                  process_rgb_matrix_typing_heatmap(row, col);
                  break;
                }
              }
            }
#endif

            break;
          }
        }
      }
    }

    g_rgb_timer = SDL_GetTicks();

    ANIMATION(&params);
    params.init = false;
    process_hits();

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
    advance_time(16);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}