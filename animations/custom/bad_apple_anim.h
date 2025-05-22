RGB_MATRIX_EFFECT(BAD_APPLE)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#    include "bad_apple_frames.h"

static bool BAD_APPLE(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    uint32_t frame  = g_rgb_timer / 125;
    uint32_t offset = (frame / 2) % 876 * 81;

    for (uint8_t i = led_min; i < led_max; i++) {
        uint8_t data      = bad_apple_frames[offset + i];
        uint8_t intensity = scale8((frame % 2 == 0 ? data & 0b1111 : data >> 4) * 16, rgb_matrix_config.hsv.v);
        rgb_matrix_set_color(i, intensity, intensity, intensity);
    }

    return rgb_matrix_check_finished_leds(led_max);
}
#endif
