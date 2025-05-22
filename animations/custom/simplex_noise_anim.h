RGB_MATRIX_EFFECT(SIMPLEX_NOISE)
#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#ifndef FNL_IMPL
#define FNL_IMPL
#include "fast_noise_lite.h"
#endif

#define COORD_SCALE_X 1.0f
#define COORD_SCALE_Y 1.0f

#define SIMPLEX_ANIMATION_SPEED_MAGNITUDE 50.0f
#define SIMPLEX_SCROLL_SPEED_X_MAGNITUDE 20.0f
#define SIMPLEX_SCROLL_SPEED_Y_MAGNITUDE 50.0f

#define FBM_OCTAVES_FNL 5
#define FBM_LACUNARITY_FNL 2.0f
#define FBM_GAIN_FNL 0.5f
#define FBM_WEIGHTED_STRENGTH_FNL 0.0f

#define MIN_EFFECT_TIME_STEP 0.0015f
#define FASTNOISE_FREQUENCY 0.01f

typedef struct {
  float r, g, b;
} float_rgb_t;

typedef struct {
  float position;
  float_rgb_t color;
} color_point_f_t;

static const color_point_f_t noise_gradient_map[] = {
    {0.0f, {0.043f, 0.0f, 1.0f}},  // #0b00ff
    {0.155f, {0.043f, 0.0f, 1.0f}},
    {0.607f, {1.0f, 0.0f, 0.95f}},   // #ff00f1
    {0.782f, {1.0f, 0.270f, 0.0f}},  // #ff4500
    {1.0f, {1.0f, 0.270f, 0.0f}}};

static const uint8_t noise_gradient_map_size =
    sizeof(noise_gradient_map) / sizeof(noise_gradient_map[0]);

static float current_z_pos_input = 0.0f;
static float current_x_scroll_input = 0.0f;
static float current_y_scroll_input = 0.0f;
static fnl_state noise_state;

static inline float_rgb_t interpolate_gradient_f(float val) {
  if (val <= noise_gradient_map[0].position) return noise_gradient_map[0].color;
  if (val >= noise_gradient_map[noise_gradient_map_size - 1].position)
    return noise_gradient_map[noise_gradient_map_size - 1].color;
  for (uint8_t i = 0; i < noise_gradient_map_size - 1; i++) {
    if (val <= noise_gradient_map[i + 1].position) {
      const float_rgb_t c0 = noise_gradient_map[i].color;
      const float_rgb_t c1 = noise_gradient_map[i + 1].color;
      const float p0 = noise_gradient_map[i].position;
      const float p1 = noise_gradient_map[i + 1].position;
      float t = (p1 - p0 == 0.0f) ? 1.0f : (val - p0) / (p1 - p0);
      float_rgb_t cout;
      cout.r = c0.r + t * (c1.r - c0.r);
      cout.g = c0.g + t * (c1.g - c0.g);
      cout.b = c0.b + t * (c1.b - c0.b);
      return cout;
    }
  }
  return noise_gradient_map[noise_gradient_map_size - 1].color;
}

static inline RGB color_float_to_rgb8(const float_rgb_t* frgb, uint8_t sat8,
                                      uint8_t val8) {
  uint8_t r8 = (uint8_t)(fmaxf(0.0f, fminf(1.0f, frgb->r)) * 255.99f);
  uint8_t g8 = (uint8_t)(fmaxf(0.0f, fminf(1.0f, frgb->g)) * 255.99f);
  uint8_t b8 = (uint8_t)(fmaxf(0.0f, fminf(1.0f, frgb->b)) * 255.99f);

  if (sat8 < 255) {
    uint8_t gray = (r8 / 4) + (g8 / 2) + (b8 / 4);
    r8 = lerp8by8(gray, r8, sat8);
    g8 = lerp8by8(gray, g8, sat8);
    b8 = lerp8by8(gray, b8, sat8);
  }
  RGB out;
  out.r = scale8(r8, val8);
  out.g = scale8(g8, val8);
  out.b = scale8(b8, val8);
  return out;
}

static bool SIMPLEX_NOISE(effect_params_t* params) {
  RGB_MATRIX_USE_LIMITS(led_min, led_max);

  if (params->init) {
    noise_state = fnlCreateState();
    noise_state.noise_type = FNL_NOISE_OPENSIMPLEX2S;
    noise_state.frequency = FASTNOISE_FREQUENCY;

    noise_state.fractal_type = FNL_FRACTAL_FBM;
    noise_state.octaves = FBM_OCTAVES_FNL;
    noise_state.lacunarity = FBM_LACUNARITY_FNL;
    noise_state.gain = FBM_GAIN_FNL;
    noise_state.weighted_strength = FBM_WEIGHTED_STRENGTH_FNL;

    current_z_pos_input = 0.0f;
    current_x_scroll_input = 0.0f;
    current_y_scroll_input = 0.0f;
    params->init = false;
  }

  const uint8_t qmk_speed = rgb_matrix_config.speed;
  float rate_multiplier = (qmk_speed == 0) ? 0.0f : ((float)qmk_speed / 128.0f);

  float frame_time_step = MIN_EFFECT_TIME_STEP * rate_multiplier;

  current_z_pos_input += frame_time_step * SIMPLEX_ANIMATION_SPEED_MAGNITUDE;
  current_x_scroll_input += frame_time_step * SIMPLEX_SCROLL_SPEED_X_MAGNITUDE;
  current_y_scroll_input += frame_time_step * SIMPLEX_SCROLL_SPEED_Y_MAGNITUDE;

  const uint8_t global_sat8 = rgb_matrix_config.hsv.s;
  const uint8_t global_val8 = rgb_matrix_config.hsv.v;

  for (uint16_t i = led_min; i < led_max; i++) {
    RGB_MATRIX_TEST_LED_FLAGS();

    FNLfloat x_fnl =
        (float)g_led_config.point[i].x * COORD_SCALE_X + current_x_scroll_input;
    FNLfloat y_fnl =
        (float)g_led_config.point[i].y * COORD_SCALE_Y + current_y_scroll_input;
    FNLfloat z_fnl = current_z_pos_input;

    float noise_val_f = fnlGetNoise3D(&noise_state, x_fnl, y_fnl, z_fnl);
    float norm_noise_f = fmaxf(0.0f, fminf(1.0f, (noise_val_f + 1.0f) * 0.5f));

    float_rgb_t grad_col_f = interpolate_gradient_f(norm_noise_f);
    RGB final_rgb = color_float_to_rgb8(&grad_col_f, global_sat8, global_val8);

    rgb_matrix_set_color(i, final_rgb.r, final_rgb.g, final_rgb.b);
  }

  return rgb_matrix_check_finished_leds(led_max);
}

#endif
