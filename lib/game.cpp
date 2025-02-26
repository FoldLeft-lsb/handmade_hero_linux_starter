#include "game.h"

// RGBA
internal_fn void game_init_pixels(offscreen_buffer *buff) {
  for (int i = 0; i < buff->length; i += buff->bytes_per_px) {
    buff->buffer[i] = 0xFF;
    buff->buffer[i + 1] = 0x00;
    buff->buffer[i + 2] = 0x00;
    buff->buffer[i + 3] = 0x00;
  }
}

internal_fn void game_update_pixels_alpha(offscreen_buffer *buff,
                                          uint8_t alpha) {
  for (int i = 0; i < buff->length; i += buff->bytes_per_px) {
    // int pixel_idx = i / buff->bytes_per_px;

    buff->buffer[i + 3] = alpha;
  }
}

extern "C" void game_init(game_memory_t *memory, offscreen_buffer *buff) {
  game_init_pixels(buff);
};

extern "C" void game_update_and_render(thread_context_t *thread_context,
                                       game_memory_t *memory,
                                       offscreen_buffer *buff,
                                       game_input_t *input, float delta_time) {

  game_state_t *state = (game_state_t *)memory->permanent_storage;
  if (!memory->is_initialized) {
    memory->is_initialized = true;

    state->alpha = 0x00;
  }

  if (input->controller.move_north.ended_down) {
    state->alpha++;
  } else if (input->controller.move_south.ended_down) {
    state->alpha--;
  }

  // state->alpha++;

  game_update_pixels_alpha(buff, state->alpha);
};
