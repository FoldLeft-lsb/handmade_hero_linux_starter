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

typedef struct offscreen_buffer {
  int width;
  int height;
  int length;
  int bytes_per_px;
  Uint8 buffer[WIDTH * HEIGHT * BYTES_PER_PX];
} offscreen_buffer;

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

void game_init(game_memory_t *memory, offscreen_buffer *buff);
void game_update_and_render(game_memory_t *memory, offscreen_buffer *buff,
                            float delta_time);

// Platform layer implements these

struct debug_read_file_result {
  Uint32 contents_size;
  void *contents;
};
debug_read_file_result DEBUGPlatformReadEntireFile(char *filename);
void DEBUGPlatformFreeFileMemory(void *memory);
bool DEBUGPlatformWriteEntireFile(char *filename, Uint32 memory_size,
                                  void *memory);

// end Platform

#define GAME_H_
#endif