#ifndef GAME_H_

#include <SDL3/SDL_stdinc.h>

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

#define WIDTH 768
#define HEIGHT 432
#define BYTES_PER_PX 4

#define internal_fn static
#define local_persist static
#define global_variable static

#define array_length(arr) (sizeof(arr)) / (sizeof(arr[0]))

typedef struct offscreen_buffer {
  int width;
  int height;
  int length;
  int bytes_per_px;
  Uint8 buffer[WIDTH * HEIGHT * BYTES_PER_PX];
} offscreen_buffer;

typedef struct game_button_state {
  int half_transition_count;
  bool ended_down;
} game_button_state_t;

typedef struct controller_input {
  float left_stick_average_x;
  float left_stick_average_y;
  bool is_analog;

  union {
    game_button_state_t buttons[12];
    struct {
      game_button_state_t move_north;
      game_button_state_t move_south;
      game_button_state_t move_west;
      game_button_state_t move_east;

      game_button_state_t action_north;
      game_button_state_t action_south;
      game_button_state_t action_west;
      game_button_state_t action_east;

      game_button_state_t left_shoulder;
      game_button_state_t right_shoulder;

      game_button_state_t start;
      game_button_state_t select;
    };
  };
} controller_input_t;

typedef struct game_input {
  controller_input_t controller;

  union {
    game_button_state_t mouse_buttons[2];
    struct {
      game_button_state_t left_click;
      game_button_state_t right_click;
    };
  };
  Uint32 mouseX, mouseY, mouseZ;
} game_input_t;

typedef struct thread_context {
} thread_context_t;

typedef struct game_state {
  Uint8 alpha;
} game_state_t;

typedef struct game_memory {
  bool is_initialized;

  Uint64 permanent_storage_size;
  void *permanent_storage;

  Uint64 transient_storage_size;
  void *transient_storage;

} game_memory_t;

typedef void game_init_t(game_memory_t *memory, offscreen_buffer *buff);
typedef void game_update_and_render_t(thread_context_t *thread,
                                      game_memory_t *memory,
                                      offscreen_buffer *buff,
                                      game_input_t *input, float delta_time);

// Platform layer File IO

typedef struct debug_read_file_result {
  Uint32 contents_size;
  void *contents;
} debug_read_file_result_t;

debug_read_file_result_t DEBUGPlatformReadEntireFile(char *filename);
void DEBUGPlatformFreeFileMemory(void *memory);
bool DEBUGPlatformWriteEntireFile(char *filename, Uint32 memory_size,
                                  void *memory);

// end Platform

#define GAME_H_
#endif