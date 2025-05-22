#include <stdatomic.h>

// ————————————————————————
// Preprocessor Definitions from QMK
// ————————————————————————

#define RGB_MATRIX_HUE_STEP 8
#define RGB_MATRIX_SAT_STEP 16
#define RGB_MATRIX_VAL_STEP 16
#define RGB_MATRIX_SPD_STEP 16
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define HAS_FLAGS(bits, flags) ((bits & flags) == flags)
#define HAS_ANY_FLAGS(bits, flags) ((bits & flags) != 0x00)
#define LED_FLAG_ALL 0xFF
#define LED_FLAG_NONE 0x00
#define LED_FLAG_MODIFIER 0x01
#define LED_FLAG_UNDERGLOW 0x02
#define LED_FLAG_KEYLIGHT 0x04
#define LED_FLAG_INDICATOR 0x08

#define RGB_MATRIX_KEYREACTIVE_ENABLED
#define RGB_MATRIX_FRAMEBUFFER_EFFECTS
#define RGB_MATRIX_LED_COUNT LED_COUNT
#define RGB_MATRIX_LED_PROCESS_LIMIT (RGB_MATRIX_LED_COUNT + 4) / 5
#define LED_HITS_TO_REMEMBER 8

#define RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#define RGB_MATRIX_EFFECT(name)              \
  static bool name(effect_params_t *params); \
  void ANIMATION(effect_params_t *params) { name(params); }

#define RGB_MATRIX_USE_LIMITS(led_min, led_max) \
  uint8_t led_min = 0;                          \
  uint8_t led_max = LED_COUNT;
#define RGB_MATRIX_TEST_LED_FLAGS()

// ————————————————————————
// Type Definitions from QMK
// ————————————————————————

typedef struct {
  uint8_t h, s, v;
} HSV;

typedef struct {
  uint8_t r, g, b;
} RGB;

typedef struct {
  uint16_t x;
  uint16_t y;
} Point;

typedef struct {
  HSV hsv;
  uint8_t speed;
} rgb_matrix_config_t;

extern rgb_matrix_config_t rgb_matrix_config;

extern uint64_t g_rgb_timer;

typedef struct {
  RGB *led_colors;
  bool init;
  int8_t flags;
  uint8_t iter;
} effect_params_t;

typedef uint8_t fract8;

// ————————————————————————
// Helper Functions
// ————————————————————————

bool rgb_matrix_check_finished_leds(uint8_t _unused) { return true; }

uint16_t rand16seed = 1337;

uint8_t random8(void) {
  rand16seed = (rand16seed * ((uint16_t)(2053))) + ((uint16_t)(13849));
  return (uint8_t)(((uint8_t)(rand16seed & 0xFF)) +
                   ((uint8_t)(rand16seed >> 8)));
}

uint8_t random8_max(uint8_t lim) {
  uint8_t r = random8();
  r = (r * lim) >> 8;
  return r;
}

uint8_t random8_min_max(uint8_t min, uint8_t lim) {
  uint8_t delta = lim - min;
  uint8_t r = random8_max(delta) + min;
  return r;
}

uint8_t sin8(uint8_t theta) {
  return (uint8_t)(127.5f * sinf(2 * M_PI * theta / 256) + 127.5f);
}

uint8_t cos8(uint8_t theta) {
  return (uint8_t)(127.5f * cosf(2 * M_PI * theta / 256) + 127.5f);
}

uint8_t scale8(uint8_t val, uint8_t factor) {
  return (uint8_t)(((uint16_t)val * factor) >> 8);
}

uint8_t qadd8(uint8_t i, uint8_t j) {
  uint16_t t = i + j;
  if (t > 255) t = 255;
  return t;
}

uint8_t qsub8(uint8_t i, uint8_t j) {
  int16_t t = i - j;
  if (t < 0) t = 0;
  return t;
}

uint8_t atan2_8(int16_t dy, int16_t dx) {
  if (dy == 0) {
    if (dx >= 0)
      return 0;
    else
      return 128;
  }

  int16_t abs_y = dy > 0 ? dy : -dy;
  int8_t a;

  if (dx >= 0)
    a = 32 - (32 * (dx - abs_y) / (dx + abs_y));
  else
    a = 96 - (32 * (dx + abs_y) / (abs_y - dx));

  if (dy < 0) return -a;
  return a;
}

uint16_t scale16by8(uint16_t i, fract8 scale) { return (i * scale) / 256; }

uint8_t lerp8by8(uint8_t fromU8, uint8_t toU8, uint8_t fract8) {
  int16_t delta = (int16_t)toU8 - (int16_t)fromU8;
  int16_t scaled = (delta * fract8) >> 8;  // divide by 256
  return fromU8 + scaled;
}

int8_t abs8(int8_t i) {
  if (i == (int8_t)-128) return (int8_t)127;
  if (i < 0) i = -i;
  return i;
}

uint8_t sqrt16(uint16_t x) {
  if (x <= 1) {
    return x;
  }

  uint8_t low = 1;  // lower bound
  uint8_t hi, mid;

  if (x > 7904) {
    hi = 255;
  } else {
    hi = (x >> 5) + 8;  // initial estimate for upper bound
  }

  do {
    mid = (low + hi) >> 1;
    if ((uint16_t)(mid * mid) > x) {
      hi = mid - 1;
    } else {
      if (mid == 255) {
        return 255;
      }
      low = mid + 1;
    }
  } while (hi >= low);

  return low - 1;
}

RGB hsv_to_rgb(HSV hsv) {
  uint8_t h = hsv.h;
  uint8_t s = hsv.s;
  uint8_t v = hsv.v;

  if (s == 0) return (RGB){v, v, v};

  uint8_t region = h / 43;
  uint8_t remainder = (h - (region * 43)) * 6;

  uint8_t p = (uint16_t)v * (255 - s) >> 8;
  uint8_t q = (uint16_t)v * (255 - ((uint16_t)s * remainder >> 8)) >> 8;
  uint8_t t_val =
      (uint16_t)v * (255 - ((uint16_t)s * (255 - remainder) >> 8)) >> 8;

  switch (region) {
    case 0:
      return (RGB){v, t_val, p};
    case 1:
      return (RGB){q, v, p};
    case 2:
      return (RGB){p, v, t_val};
    case 3:
      return (RGB){p, q, v};
    case 4:
      return (RGB){t_val, p, v};
    default:
      return (RGB){v, p, q};
  }
}

uint8_t rgb_matrix_map_row_column_to_led_kb(uint8_t row, uint8_t column,
                                            uint8_t *led_i) {
  return 0;
}

uint8_t rgb_matrix_map_row_column_to_led(uint8_t row, uint8_t column,
                                         uint8_t *led_i) {
  uint8_t led_count = rgb_matrix_map_row_column_to_led_kb(row, column, led_i);
  uint8_t led_index = g_led_config.matrix_co[row][column];
  if (led_index != NO_LED) {
    led_i[led_count] = led_index;
    led_count++;
  }
  return led_count;
}

RGB rgb_matrix_hsv_to_rgb(HSV hsv) { return hsv_to_rgb(hsv); }

#define TIMER_DIFF(a, b, max)                  \
  ((max == UINT8_MAX)                          \
       ? ((uint8_t)((a) - (b)))                \
       : ((max == UINT16_MAX)                  \
              ? ((uint16_t)((a) - (b)))        \
              : ((max == UINT32_MAX)           \
                     ? ((uint32_t)((a) - (b))) \
                     : ((a) >= (b) ? (a) - (b) : (max) + 1 - (b) + (a)))))
#define TIMER_DIFF_8(a, b) TIMER_DIFF(a, b, UINT8_MAX)
#define TIMER_DIFF_16(a, b) TIMER_DIFF(a, b, UINT16_MAX)
#define TIMER_DIFF_32(a, b) TIMER_DIFF(a, b, UINT32_MAX)

static atomic_uint_least32_t current_time = 0;
static atomic_uint_least32_t async_tick_amount = 0;
static atomic_uint_least32_t access_counter = 0;

uint32_t timer_read32(void) {
  if (access_counter++ > 0) {
    current_time += async_tick_amount;
  }
  return current_time;
}

uint16_t timer_read(void) { return (uint16_t)timer_read32(); }

uint16_t timer_elapsed(uint16_t last) {
  return TIMER_DIFF_16(timer_read(), last);
}

// ————————————————————————
// External Declarations
// ————————————————————————

rgb_matrix_config_t rgb_matrix_config = {.hsv = {0, 255, 255}, .speed = 127};

typedef struct {
  uint8_t count;
  uint8_t x[LED_HITS_TO_REMEMBER];
  uint8_t y[LED_HITS_TO_REMEMBER];
  uint8_t index[LED_HITS_TO_REMEMBER];
  uint16_t tick[LED_HITS_TO_REMEMBER];
} last_hit_t;
last_hit_t g_last_hit_tracker;

uint64_t g_rgb_timer = 0;

// ————————————————————————
// Effect Runner Implementation
// ————————————————————————
RGB led_colors[LED_COUNT];
void rgb_matrix_set_color(uint8_t i, uint8_t r, uint8_t g, uint8_t b) {
  led_colors[i].r = r;
  led_colors[i].g = g;
  led_colors[i].b = b;
}

void rgb_matrix_set_color_all(uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    led_colors[i].r = r;
    led_colors[i].g = g;
    led_colors[i].b = b;
  }
}

typedef HSV (*i_f)(HSV hsv, uint8_t i, uint8_t time);

bool effect_runner_i(effect_params_t *params, i_f effect_func) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);

  uint8_t time = scale16by8(g_rgb_timer, rgb_matrix_config.speed / 4);
  for (uint8_t i = led_min; i < led_max; i++) {
    RGB_MATRIX_TEST_LED_FLAGS();
    RGB rgb =
        rgb_matrix_hsv_to_rgb(effect_func(rgb_matrix_config.hsv, i, time));
    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
  }
  return led_max < LED_COUNT;
}

typedef HSV (*dx_dy_dist_f)(HSV hsv, int16_t dx, int16_t dy, uint8_t dist,
                            uint8_t time);

bool effect_runner_dx_dy_dist(effect_params_t *params,
                              dx_dy_dist_f effect_func) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);

  uint8_t time = scale16by8(g_rgb_timer, rgb_matrix_config.speed / 2);
  for (uint8_t i = led_min; i < led_max; i++) {
    RGB_MATRIX_TEST_LED_FLAGS();
    int16_t dx = g_led_config.point[i].x - k_rgb_matrix_center.x;
    int16_t dy = g_led_config.point[i].y - k_rgb_matrix_center.y;
    uint8_t dist = sqrt16(dx * dx + dy * dy);
    RGB rgb = rgb_matrix_hsv_to_rgb(
        effect_func(rgb_matrix_config.hsv, dx, dy, dist, time));
    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
  }
  return led_max < LED_COUNT;
}

typedef HSV (*dx_dy_f)(HSV hsv, int16_t dx, int16_t dy, uint8_t time);

bool effect_runner_dx_dy(effect_params_t *params, dx_dy_f effect_func) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);

  uint8_t time = scale16by8(g_rgb_timer, rgb_matrix_config.speed / 2);
  for (uint8_t i = led_min; i < led_max; i++) {
    RGB_MATRIX_TEST_LED_FLAGS();
    int16_t dx = g_led_config.point[i].x - k_rgb_matrix_center.x;
    int16_t dy = g_led_config.point[i].y - k_rgb_matrix_center.y;
    RGB rgb =
        rgb_matrix_hsv_to_rgb(effect_func(rgb_matrix_config.hsv, dx, dy, time));
    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
  }
  return led_max < LED_COUNT;
}

typedef HSV (*sin_cos_i_f)(HSV hsv, int8_t sin, int8_t cos, uint8_t i,
                           uint8_t time);

bool effect_runner_sin_cos_i(effect_params_t *params, sin_cos_i_f effect_func) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);

  uint16_t time = scale16by8(g_rgb_timer, rgb_matrix_config.speed / 4);
  int8_t cos_value = cos8(time) - 128;
  int8_t sin_value = sin8(time) - 128;
  for (uint8_t i = led_min; i < led_max; i++) {
    RGB_MATRIX_TEST_LED_FLAGS();
    RGB rgb = rgb_matrix_hsv_to_rgb(
        effect_func(rgb_matrix_config.hsv, cos_value, sin_value, i, time));
    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
  }
  return led_max < LED_COUNT;
}

typedef HSV (*reactive_f)(HSV hsv, uint16_t offset);

bool effect_runner_reactive(effect_params_t *params, reactive_f effect_func) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);

  uint16_t max_tick = 65535 / rgb_matrix_config.speed;
  for (uint8_t i = led_min; i < led_max; i++) {
    RGB_MATRIX_TEST_LED_FLAGS();
    uint16_t tick = max_tick;
    // Reverse search to find most recent key hit
    for (int8_t j = g_last_hit_tracker.count - 1; j >= 0; j--) {
      if (g_last_hit_tracker.index[j] == i &&
          g_last_hit_tracker.tick[j] < tick) {
        tick = g_last_hit_tracker.tick[j];
        break;
      }
    }

    uint16_t offset = scale16by8(tick, rgb_matrix_config.speed);
    RGB rgb = rgb_matrix_hsv_to_rgb(effect_func(rgb_matrix_config.hsv, offset));
    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
  }
  return led_max < LED_COUNT;
}

typedef HSV (*reactive_splash_f)(HSV hsv, int16_t dx, int16_t dy, uint8_t dist,
                                 uint16_t tick);

bool effect_runner_reactive_splash(uint8_t start, effect_params_t *params,
                                   reactive_splash_f effect_func) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);

  uint8_t count = g_last_hit_tracker.count;
  for (uint8_t i = led_min; i < led_max; i++) {
    RGB_MATRIX_TEST_LED_FLAGS();
    HSV hsv = rgb_matrix_config.hsv;
    hsv.v = 0;
    for (uint8_t j = start; j < count; j++) {
      int16_t dx = g_led_config.point[i].x - g_last_hit_tracker.x[j];
      int16_t dy = g_led_config.point[i].y - g_last_hit_tracker.y[j];
      uint8_t dist = sqrt16(dx * dx + dy * dy);
      uint16_t tick =
          scale16by8(g_last_hit_tracker.tick[j], rgb_matrix_config.speed);
      hsv = effect_func(hsv, dx, dy, dist, tick);
    }
    hsv.v = scale8(hsv.v, rgb_matrix_config.hsv.v);
    RGB rgb = rgb_matrix_hsv_to_rgb(hsv);
    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
  }
  return led_max < LED_COUNT;
}
