/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_rgb_underglow

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <dt-bindings/zmk/rgb.h>
#include <zmk/rgb_underglow.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int behavior_rgb_underglow_init(const struct device *dev) { return 0; }

static int
on_keymap_binding_convert_central_state_dependent_params(struct zmk_behavior_binding *binding,
                                                         struct zmk_behavior_binding_event event) {
    switch (binding->param1) {
    case RGB_TOG_CMD: {
        bool state;
        int err = zmk_rgb_underglow_get_state(&state);
        if (err) {
            LOG_ERR("Failed to get RGB underglow state (err %d)", err);
            return err;
        }

        binding->param1 = state ? RGB_OFF_CMD : RGB_ON_CMD;
        break;
    }
    case RGB_BRI_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_brt(1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_BRD_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_brt(-1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_HUI_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_HUD_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(-1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_SAI_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_sat(1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_SAD_CMD: {
        struct zmk_led_hsb color = zmk_rgb_underglow_calc_sat(-1);

        binding->param1 = RGB_COLOR_HSB_CMD;
        binding->param2 = RGB_COLOR_HSB_VAL(color.h, color.s, color.b);
        break;
    }
    case RGB_EFR_CMD: {
        binding->param1 = RGB_EFS_CMD;
        binding->param2 = zmk_rgb_underglow_calc_effect(-1);
        break;
    }
    case RGB_EFF_CMD: {
        binding->param1 = RGB_EFS_CMD;
        binding->param2 = zmk_rgb_underglow_calc_effect(1);
        // Set the start date for the effect to begin to be 50ms in the future.
        // This gives us time to get things ready on the both halves.
        // TODO(PT): This is just to convey the idea, I'm unsure if the timescale here is actually milliseconds.
        binding->param3 = k_uptime_get() + 50;
        break;
    }
    default:
        return 0;
    }

    LOG_DBG("RGB relative convert to absolute (%d/%d, %d)", binding->param1, binding->param2, binding->param3);

    return 0;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    if (binding->param3 > 0) {
        // TODO(PT): Here, both halves would wait until the start date specified by param3 has been
        // reached.
        // In my testing, however, it appears that this is invoked on the main half before the BLE
        // command is sent to the peripheral half, so I think more rearranging would be needed
        // (or perhaps this isn't the right place for this at all).
        // Also, importantly, the clock here is currently just the uptime of this half, which isn't
        // synchronized between the halves. If this were a real feature, each half would need to be
        // synchronized with an external clock (or just each other's clocks). This perhaps could be
        // pulled from the host?
        LOG_WRN("Sleeping until timestamp is reached...");
        while (k_uptime_get() < binding->param3) {
            // TODO(PT): Perhaps Zephyr has some kind of utility for a gentle spinloop?
        }
        LOG_WRN("Timestamp has elapsed! Timestamp: %d, Now: %d", binding->param3, k_uptime_get());
    }
    switch (binding->param1) {
    case RGB_TOG_CMD:
        return zmk_rgb_underglow_toggle();
    case RGB_ON_CMD:
        return zmk_rgb_underglow_on();
    case RGB_OFF_CMD:
        return zmk_rgb_underglow_off();
    case RGB_HUI_CMD:
        return zmk_rgb_underglow_change_hue(1);
    case RGB_HUD_CMD:
        return zmk_rgb_underglow_change_hue(-1);
    case RGB_SAI_CMD:
        return zmk_rgb_underglow_change_sat(1);
    case RGB_SAD_CMD:
        return zmk_rgb_underglow_change_sat(-1);
    case RGB_BRI_CMD:
        return zmk_rgb_underglow_change_brt(1);
    case RGB_BRD_CMD:
        return zmk_rgb_underglow_change_brt(-1);
    case RGB_SPI_CMD:
        return zmk_rgb_underglow_change_spd(1);
    case RGB_SPD_CMD:
        return zmk_rgb_underglow_change_spd(-1);
    case RGB_EFS_CMD:
        return zmk_rgb_underglow_select_effect(binding->param2);
    case RGB_EFF_CMD:
        return zmk_rgb_underglow_cycle_effect(1);
    case RGB_EFR_CMD:
        return zmk_rgb_underglow_cycle_effect(-1);
    case RGB_COLOR_HSB_CMD:
        return zmk_rgb_underglow_set_hsb((struct zmk_led_hsb){.h = (binding->param2 >> 16) & 0xFFFF,
                                                              .s = (binding->param2 >> 8) & 0xFF,
                                                              .b = binding->param2 & 0xFF});
    }

    return -ENOTSUP;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_rgb_underglow_driver_api = {
    .binding_convert_central_state_dependent_params =
        on_keymap_binding_convert_central_state_dependent_params,
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_rgb_underglow_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_rgb_underglow_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
