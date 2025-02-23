#include "lib/game.h"

#include <SDL3/SDL.h>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// IO
// These functions are defined in game.h
// and implemented here, part of the platform
// layer that is accessible within the game.
// Is that OK? I don't know lol

inline Uint32 SafeTruncateUInt64(Uint64 value) {
  Uint32 result = (Uint32)value;
  return (result);
}

debug_read_file_result DEBUGPlatformReadEntireFile(char *filename) {
  debug_read_file_result result = {};

  int file_handle = open(filename, O_RDONLY);
  if (file_handle == -1) {
    return result;
  }

  struct stat file_status;
  if (fstat(file_handle, &file_status) == -1) {
    close(file_handle);
    return result;
  }
  result.contents_size = SafeTruncateUInt64(file_status.st_size);

  result.contents = malloc(result.contents_size);
  if (result.contents == NULL) {
    close(file_handle);
    result.contents_size = 0;
    return result;
  }

  Uint32 bytes_to_read = result.contents_size;
  Uint8 *next_byte_location = (Uint8 *)result.contents;
  while (bytes_to_read) {
    Uint32 bytes_read = read(file_handle, next_byte_location, bytes_to_read);
    if (bytes_read == -1) {
      free(result.contents);
      result.contents = 0;
      result.contents_size = 0;
      close(file_handle);
      return result;
    }
    bytes_to_read -= bytes_read;
    next_byte_location += bytes_read;
  }

  close(file_handle);
  return result;
};

void DEBUGPlatformFreeFileMemory(void *memory) { free(memory); };

bool DEBUGPlatformWriteEntireFile(char *filename, Uint32 memory_size,
                                  void *memory) {
  int file_handle =
      open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (!file_handle)
    return false;

  Uint32 bytes_to_write = memory_size;
  Uint8 *next_byte_location = (Uint8 *)memory;
  while (bytes_to_write) {
    Uint32 bytes_written =
        write(file_handle, next_byte_location, bytes_to_write);
    if (bytes_written == -1) {
      close(file_handle);
      return false;
    }
    bytes_to_write -= bytes_written;
    next_byte_location += bytes_written;
  }

  close(file_handle);
  return true;
};

// /IO

// Should eliminate some of these globals

global_variable int target_fps = 60;
global_variable Uint64 target_frame_time = 1000 / target_fps;

const int scale = 1;

global_variable SDL_Window *window = NULL;
global_variable SDL_Renderer *renderer = NULL;
offscreen_buffer pixel_buffer =
    (offscreen_buffer){.width = WIDTH,
                       .height = HEIGHT,
                       .length = WIDTH * HEIGHT * BYTES_PER_PX,
                       .bytes_per_px = BYTES_PER_PX,
                       .buffer = {}};
global_variable int nGamepads;
global_variable SDL_JoystickID *joystickId;
global_variable SDL_Gamepad *gamepad;
global_variable bool quit = false;

global_variable SDL_Surface *surface;
global_variable SDL_Texture *tex;
global_variable SDL_FRect destR = (SDL_FRect){
    .x = 0,
    .y = 0,
};

// /globals

int main() {

  game_memory_t game_memory = {};
  game_memory.permanent_storage_size = Megabytes(64);
  game_memory.transient_storage_size = Megabytes(512);
  Uint64 total_storage_size =
      game_memory.permanent_storage_size + game_memory.transient_storage_size;

  game_memory.permanent_storage =
      mmap(0, total_storage_size, PROT_READ | PROT_WRITE,
           MAP_ANON | MAP_PRIVATE, -1, 0);

  game_memory.transient_storage = (Uint8 *)(game_memory.permanent_storage) +
                                  game_memory.permanent_storage_size;

  if (game_memory.permanent_storage == NULL ||
      game_memory.transient_storage == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to allocate memory");
    return 1;
  }

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

  SDL_CreateWindowAndRenderer("Hello SDL3", WIDTH * scale, HEIGHT * scale, 0,
                              &window, &renderer);
  SDL_SetWindowResizable(window, true);

  game_init(&game_memory, &pixel_buffer);

  surface = SDL_CreateSurface(WIDTH, HEIGHT, SDL_PIXELFORMAT_RGBA8888);

  local_persist Uint64 last_tick;
  local_persist Uint64 current_tick;
  local_persist float delta_time;

  // Game loop
  while (!quit) {

    last_tick = current_tick;
    current_tick = SDL_GetTicks();
    delta_time = (current_tick - last_tick) / 1000.0f;

    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {

      // Quit if told to
      if (event.type == SDL_EVENT_QUIT)
        quit = true;
      if (event.type == SDL_EVENT_WINDOW_RESIZED)
        SDL_Log("Window Resized");

      if (event.type == SDL_EVENT_GAMEPAD_REMOVED)
        SDL_Log("Gamepad Removed");
      // Open gamepad if connected
      // This event fires if started with a gamepad available
      if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        SDL_Log("Gamepad Added");
        joystickId = SDL_GetGamepads(&nGamepads);
        gamepad = SDL_OpenGamepad(*joystickId);
        SDL_Log("Gamepad Added %i", nGamepads);
      }

      // Keyboard inputs
      if (event.type == SDL_EVENT_KEY_DOWN) {

        if (event.key.key == SDLK_ESCAPE) {
          SDL_Log("Keyboard: Esc");
          quit = true;
        }

        if (event.key.key == SDLK_W)
          SDL_Log("Keyboard: W");
        if (event.key.key == SDLK_A)
          SDL_Log("Keyboard: A");
        if (event.key.key == SDLK_S)
          SDL_Log("Keyboard: S");
        if (event.key.key == SDLK_D)
          SDL_Log("Keyboard: D");

        if (event.key.key == SDLK_E)
          SDL_Log("Keyboard: E");
        if (event.key.key == SDLK_P)
          SDL_Log("Keyboard: P");
      }

      // Gamepad inputs
      if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {

        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_START) {
          SDL_Log("Gamepad: Start");
          quit = true;
        }

        // I happen to have an xbox one controller
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST)
          SDL_Log("Gamepad: X");
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_NORTH)
          SDL_Log("Gamepad: Y");
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH)
          SDL_Log("Gamepad: A");
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST)
          SDL_Log("Gamepad: B");

        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
          SDL_Log("Gamepad: Up");
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
          SDL_Log("Gamepad: Down");
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
          SDL_Log("Gamepad: Left");
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
          SDL_Log("Gamepad: Right");
      }
    }

    // Updated buffer overwrites to the surface
    surface->pixels = pixel_buffer.buffer;

    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    destR.w = WIDTH * scale;
    destR.h = HEIGHT * scale;

    // Allocate a new texture each frame which must be cleared
    // Maybe should use SDL_UpdateTexture instead but requires
    // computing pitch etc.
    tex = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
    SDL_RenderClear(renderer);

    SDL_RenderTexture(renderer, tex, NULL, &destR);

    SDL_RenderPresent(renderer);

    // Clear the texture after drawing
    SDL_DestroyTexture(tex);

    // == UPDATE ==
    game_update_and_render(&game_memory, &pixel_buffer, delta_time);
    // == END UPDATE ==

    // Limit fps
    Uint64 frame_time = SDL_GetTicks() - current_tick;
    if (frame_time < target_frame_time)
      SDL_Delay(target_frame_time - frame_time);

  } // End loop

  SDL_Quit();
  return 0;
}
