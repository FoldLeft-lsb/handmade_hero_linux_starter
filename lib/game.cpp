#include "game.h"
// #include <stdio.h>

internal_fn void game_init_pixels(offscreen_buffer *buff) {
  for (int i = 0; i < buff->length; i++) {
    // for testing
    if (i % buff->bytes_per_px == 0) {
      // init Red
      buff->buffer[i] = 0xFF;
    } else {
      // init Blue Green and Alpha as 0
      buff->buffer[i] = 0x00;
    }
  }
}

internal_fn void game_update_pixels_alpha(offscreen_buffer *buff, Uint8 alpha) {
  for (int i = 0; i < buff->length; i += buff->bytes_per_px) {
    // pixel index
    // int idx = i / BYTES_PER_PX;
    // can work out x, y with w and h from that

    // red
    // pixelData[i]

    // green
    // pixelData[i + 1]

    // blue
    // pixelData[i + 2]

    // alpha
    buff->buffer[i + 3] = alpha;
  }
}

void game_init(game_memory_t *memory, offscreen_buffer *buff) {
  game_init_pixels(buff);
};

void game_update_and_render(game_memory_t *memory, offscreen_buffer *buff,
                            game_input_t *input, float delta_time) {

  game_state_t *state = (game_state_t *)memory->permanent_storage;
  if (!memory->is_initialized) {
    memory->is_initialized = true;

    state->alpha = 0x00;

    // // Testing file IO

    // // Assignable string warning for C++11 but I'm using 20
    // char *filename = "linux_platform.cpp";

    // debug_read_file_result file_memory =
    // DEBUGPlatformReadEntireFile(filename); if (file_memory.contents) {
    //   printf("Contents size %d\n", file_memory.contents_size);

    //   // Relative path works for writing
    //   DEBUGPlatformWriteEntireFile("./data/test.out",
    //   file_memory.contents_size,
    //                                file_memory.contents);

    //   DEBUGPlatformFreeFileMemory(file_memory.contents);
    // } else {
    //   printf("Failed to read file\n");
    // }
    // printf("Contents size after free %d\n", file_memory.contents_size);

    // // File IO worked correctly in manual testing
  }

  // // Testing input abstraction

  // if (input->move_north.half_transition_count > 0) {
  //   printf("Key Is Analog: %s\n", input->is_analog ? "Yes" : "No");
  //   printf("Key Move North: %s\n",
  //          input->move_north.ended_down ? "down" : "up");
  //   printf("Key Move North: %d\n", input->move_north.half_transition_count);
  // }

  if (input->move_north.ended_down) {
    state->alpha++;
  } else if (input->move_south.ended_down) {
    state->alpha--;
  }

  // state->alpha++;

  game_update_pixels_alpha(buff, state->alpha);
};
