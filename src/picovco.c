#include <hardware/interp.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include <bsp/board.h>
#include <tusb.h>

#include "gpio.pio.h"

#define OSC_GPIO 15

static volatile uint32_t pitch = 0;
const uint32_t pitch_table[128] = {
    2000,    2118,    2244,    2378,    2519,    2669,    2828,    2996,
    3174,    3363,    3563,    3775,    4000,    4237,    4489,    4756,
    5039,    5339,    5656,    5993,    6349,    6727,    7127,    7550,
    8000,    8475,    8979,    9513,    10079,   10678,   11313,   11986,
    12699,   13454,   14254,   15101,   16000,   16951,   17959,   19027,
    20158,   21357,   22627,   23972,   25398,   26908,   28508,   30203,
    32000,   33902,   35918,   38054,   40317,   42714,   45254,   47945,
    50796,   53817,   57017,   60407,   64000,   67805,   71837,   76109,
    80634,   85429,   90509,   95891,   101593,  107634,  114035,  120815,
    128000,  135611,  143675,  152218,  161269,  170859,  181019,  191783,
    203187,  215269,  228070,  241631,  256000,  271222,  287350,  304437,
    322539,  341719,  362038,  383566,  406374,  430538,  456140,  483263,
    512000,  542445,  574700,  608874,  645079,  683438,  724077,  767133,
    812749,  861077,  912280,  966527,  1024000, 1084890, 1149401, 1217748,
    1290159, 1366876, 1448154, 1534266, 1625498, 1722155, 1824560, 1933054,
    2048000, 2169780, 2298802, 2435496, 2580318, 2733752, 2896309};

void core1_entry() {
  board_init();
  tud_init(BOARD_TUD_RHPORT);

  while (true) {
    tud_task();
    while (tud_midi_available()) {
      uint8_t packet[4];
      tud_midi_packet_read(packet);

      if (packet[1] == 0x90) {
        pitch = pitch_table[packet[2]];
      }
    }
  }
}

int main() {
  // Overclock
  set_sys_clock_khz(250000, true);

  // Start USB stack
  multicore_launch_core1(core1_entry);

  // Configure interpolator lanes
  interp_config ic = interp_default_config();

  interp_config_set_cross_result(&ic, true);
  interp_config_set_add_raw(&ic, true);
  interp_set_config(interp0, 0, &ic);

  interp_config_set_mask(&ic, 0, 30);
  interp_config_set_add_raw(&ic, false);
  interp_set_config(interp0, 1, &ic);

  interp_set_base_both(interp0, 0);

  // Initialize PIO
  uint pio_ofs = pio_add_program(pio0, &pio_gpio_program);
  uint sm = pio_claim_unused_sm(pio0, true);

  pio_sm_config pc = pio_gpio_program_get_default_config(pio_ofs);
  sm_config_set_out_pins(&pc, OSC_GPIO, 1);
  sm_config_set_out_shift(&pc, false, true, 1);
  sm_config_set_clkdiv_int_frac(&pc, 1, 0);

  pio_gpio_init(pio0, OSC_GPIO);
  pio_sm_set_consecutive_pindirs(pio0, sm, OSC_GPIO, 1, true);

  pio_sm_init(pio0, sm, pio_ofs, &pc);
  pio_sm_set_enabled(pio0, sm, true);

  // Main loop
  uint32_t w = 0;
  while (true) {
    w += pitch;
    interp0->base[0] = w;
    pio0->txf[sm] = interp0->pop[0];
  }

  return 0;
}