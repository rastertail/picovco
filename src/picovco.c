#include <pico/multicore.h>
#include <pico/stdlib.h>

#include <bsp/board.h>
#include <tusb.h>

#include "gpio.pio.h"

#define OSC_GPIO 15

static volatile uint32_t pitch = 0;
static volatile uint32_t pitch2 = 0;
const uint32_t pitch_table[128] = {
    256,    271,    287,    304,    323,    342,    362,    384,    406,
    431,    456,    483,    512,    542,    575,    609,    645,    683,
    724,    767,    813,    861,    912,    967,    1024,   1085,   1149,
    1218,   1290,   1367,   1448,   1534,   1625,   1722,   1825,   1933,
    2048,   2170,   2299,   2435,   2580,   2734,   2896,   3069,   3251,
    3444,   3649,   3866,   4096,   4340,   4598,   4871,   5161,   5468,
    5793,   6137,   6502,   6889,   7298,   7732,   8192,   8679,   9195,
    9742,   10321,  10935,  11585,  12274,  13004,  13777,  14596,  15464,
    16384,  17358,  18390,  19484,  20643,  21870,  23170,  24548,  26008,
    27554,  29193,  30929,  32768,  34716,  36781,  38968,  41285,  43740,
    46341,  49097,  52016,  55109,  58386,  61858,  65536,  69433,  73562,
    77936,  82570,  87480,  92682,  98193,  104032, 110218, 116772, 123715,
    131072, 138866, 147123, 155872, 165140, 174960, 185364, 196386, 208064,
    220436, 233544, 247431, 262144, 277732, 294247, 311744, 330281, 349920,
    370728, 392772};

void core1_entry() {
  board_init();
  tud_init(BOARD_TUD_RHPORT);

  uint32_t note;
  uint32_t mw;
  while (true) {
    tud_task();
    while (tud_midi_available()) {
      uint8_t packet[4];
      tud_midi_packet_read(packet);

      if (packet[1] == 0xB0 && packet[2] == 0x01) {
        mw = packet[3];
      }
      if (packet[1] == 0x90) {
        note = packet[2];
      }
    }
    pitch = (pitch_table[note] * (2000 + (mw << 8))) >> 8;
    pitch2 = (pitch_table[note] * 2000) >> 8;
  }
}

int main() {
  // Overclock
  set_sys_clock_khz(250000, true);

  // Start USB stack
  multicore_launch_core1(core1_entry);

  // Initialize PIO
  uint pio_ofs = pio_add_program(pio0, &pio_gpio_program);
  uint sm = pio_claim_unused_sm(pio0, true);

  pio_sm_config pc = pio_gpio_program_get_default_config(pio_ofs);
  sm_config_set_out_pins(&pc, OSC_GPIO, 1);
  sm_config_set_out_shift(&pc, true, true, 1);
  sm_config_set_clkdiv_int_frac(&pc, 1, 0);

  pio_gpio_init(pio0, OSC_GPIO);
  pio_sm_set_consecutive_pindirs(pio0, sm, OSC_GPIO, 1, true);

  pio_sm_init(pio0, sm, pio_ofs, &pc);
  pio_sm_set_enabled(pio0, sm, true);

  // Main loop
  uint32_t w = 0;
  uint32_t w2 = 0;
  uint32_t a = 0;
  while (true) {
    uint32_t v = 0;
    __asm(".syntax unified\n\t"
          "adds %[w2], %[pitch2]\n\t"
          "bcc .nosync\n\t"
          "movs %[w], #0\n"
          ".nosync:\n\t"
          "add %[w], %[pitch]\n\t"
          "adds %[a], %[w]\n\t"
          "adcs %[v], %[v]"
          : [w] "+r"(w), [w2] "+r"(w2), [a] "+r"(a), [v] "+r"(v)
          : [pitch] "r"(pitch), [pitch2] "r"(pitch2)
          : "cc");
    pio0->txf[sm] = v;
  }

  return 0;
}