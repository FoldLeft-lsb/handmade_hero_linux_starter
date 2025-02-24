#include "lib/game.cpp"
#include "lib/game.h"

#include <SDL3/SDL.h>

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// NOTE: Handmade Hero does sound from a buffer
// I couldn't figure it out with SDL3 so I haven't.
// Either I will figure it out, or just use SDL later
// without managing the buffer myself.

// File IO
// These functions are defined in game.h
// and implemented here, part of the platform
// layer that is accessible within the game.
// Is that OK? I don't know lol

inline Uint32 SafeTruncateUInt64(Uint64 value) {
  // assert
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

// End File IO

// Should eliminate some or all of these globals

global_variable int target_fps = 60;
global_variable Uint64 target_frame_time = 1000 / target_fps;
global_variable int target_physics_updates_ps = 30;
global_variable Uint64 target_physics_time = 1000 / target_physics_updates_ps;

global_variable bool quit = false;

const int scale = 2;

offscreen_buffer pixel_buffer =
    (offscreen_buffer){.width = WIDTH,
                       .height = HEIGHT,
                       .length = WIDTH * HEIGHT * BYTES_PER_PX,
                       .bytes_per_px = BYTES_PER_PX,
                       .buffer = {}};

global_variable int nGamepads;
global_variable SDL_JoystickID *joystickId;
global_variable SDL_Gamepad *gamepad;
global_variable int left_stick_deadzone = 9000;
global_variable int right_stick_deadzone = 8000;

// /globals

internal_fn void PlatformHandleInputButton(game_button_state_t *new_state,
                                           bool value) {
  new_state->ended_down = value;
  new_state->half_transition_count++;
}

internal_fn void PlatformHandleGamepadButton(game_button_state_t *old_state,
                                             game_button_state_t *new_state,
                                             bool value) {
  new_state->ended_down = value;
  new_state->half_transition_count +=
      ((new_state->ended_down == old_state->ended_down) ? 0 : 1);
}

internal_fn float PlatformGetGamepadAxisValue(int16_t value, int16_t deadzone) {
  float result = 0;
  if (value < -deadzone) {
    result = (float)((value + deadzone) / (32768.0f - deadzone));
  } else if (value > deadzone) {
    result = (float)((value - deadzone) / (32767.0f - deadzone));
  }
  return result;
}

internal_fn void PlatformHandleInputEvent(SDL_Event *event,
                                          game_input_t *new_input,
                                          game_input_t *old_input) {
  // Keyboard inputs
  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {

    if (event->key.key == SDLK_ESCAPE) {
      SDL_Log("Keyboard: Esc");
      quit = true;
    }

    if (event->key.repeat == 0) {
      if (event->key.key == SDLK_W) {
        PlatformHandleInputButton(&new_input->move_north, event->key.down);
      }
      if (event->key.key == SDLK_A) {
        PlatformHandleInputButton(&new_input->move_west, event->key.down);
      }
      if (event->key.key == SDLK_S) {
        PlatformHandleInputButton(&new_input->move_south, event->key.down);
      }
      if (event->key.key == SDLK_D) {
        PlatformHandleInputButton(&new_input->move_east, event->key.down);
      }
      if (event->key.key == SDLK_UP) {
        PlatformHandleInputButton(&new_input->move_north, event->key.down);
      }
      if (event->key.key == SDLK_LEFT) {
        PlatformHandleInputButton(&new_input->move_west, event->key.down);
      }
      if (event->key.key == SDLK_DOWN) {
        PlatformHandleInputButton(&new_input->move_south, event->key.down);
      }
      if (event->key.key == SDLK_RIGHT) {
        PlatformHandleInputButton(&new_input->move_east, event->key.down);
      }

      if (event->key.key == SDLK_E) {
        PlatformHandleInputButton(&new_input->action_south, event->key.down);
      }
      if (event->key.key == SDLK_F) {
        PlatformHandleInputButton(&new_input->action_east, event->key.down);
      }
      if (event->key.key == SDLK_Q) {
        PlatformHandleInputButton(&new_input->action_west, event->key.down);
      }
      if (event->key.key == SDLK_R) {
        PlatformHandleInputButton(&new_input->action_north, event->key.down);
      }

      if (event->key.key == SDLK_LSHIFT) {
        PlatformHandleInputButton(&new_input->left_shoulder, event->key.down);
      }
      if (event->key.key == SDLK_LCTRL) {
        PlatformHandleInputButton(&new_input->right_shoulder, event->key.down);
      }

      if (event->key.key == SDLK_P) {
        PlatformHandleInputButton(&new_input->start, event->key.down);
      }
      if (event->key.key == SDLK_I) {
        PlatformHandleInputButton(&new_input->select, event->key.down);
      }
    }
  }

  // Gamepad inputs
  if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
    if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX ||
        event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {

      new_input->is_analog = true;

      new_input->left_stick_average_x =
          PlatformGetGamepadAxisValue(event->gaxis.value, left_stick_deadzone);
      new_input->left_stick_average_y =
          PlatformGetGamepadAxisValue(event->gaxis.value, left_stick_deadzone);

      float threshold = 0.5f;
      PlatformHandleGamepadButton(&old_input->move_west, &new_input->move_west,
                                  new_input->left_stick_average_x < -threshold);
      PlatformHandleGamepadButton(&old_input->move_east, &new_input->move_east,
                                  new_input->left_stick_average_x > threshold);
      PlatformHandleGamepadButton(&old_input->move_north,
                                  &new_input->move_north,
                                  new_input->left_stick_average_y < -threshold);
      PlatformHandleGamepadButton(&old_input->move_south,
                                  &new_input->move_south,
                                  new_input->left_stick_average_y > threshold);
    }
  }

  if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
      event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) {

    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_START) {
      PlatformHandleInputButton(&new_input->start, event->gbutton.down);
      quit = true;
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
      PlatformHandleInputButton(&new_input->select, event->gbutton.down);
    }

    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
      PlatformHandleInputButton(&new_input->left_shoulder, event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
      PlatformHandleInputButton(&new_input->right_shoulder,
                                event->gbutton.down);
    }

    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_WEST) {
      PlatformHandleInputButton(&new_input->action_west, event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_NORTH) {
      PlatformHandleInputButton(&new_input->action_north, event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
      PlatformHandleInputButton(&new_input->action_south, event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST) {
      PlatformHandleInputButton(&new_input->action_east, event->gbutton.down);
    }

    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP) {
      new_input->is_analog = false;
      PlatformHandleInputButton(&new_input->move_north, event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
      new_input->is_analog = false;
      PlatformHandleInputButton(&new_input->move_south, event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) {
      new_input->is_analog = false;
      PlatformHandleInputButton(&new_input->move_west, event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
      new_input->is_analog = false;
      PlatformHandleInputButton(&new_input->move_east, event->gbutton.down);
    }
  }
}

void PlatformUpdateAndDrawFrame(SDL_Renderer *renderer, SDL_FRect *destR,
                                SDL_Texture *tex, offscreen_buffer *buffer) {
  destR->w = WIDTH * scale;
  destR->h = HEIGHT * scale;

  SDL_UpdateTexture(tex, NULL, &(buffer->buffer),
                    buffer->width * buffer->bytes_per_px);

  SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
  SDL_RenderClear(renderer);

  SDL_RenderTexture(renderer, tex, NULL, destR);

  SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {

  local_persist SDL_Window *window = NULL;
  local_persist SDL_Renderer *renderer = NULL;
  local_persist SDL_Texture *tex;
  local_persist SDL_FRect destR = (SDL_FRect){
      .x = 0,
      .y = 0,
  };

  game_memory_t game_memory = {};
  game_memory.permanent_storage_size = Megabytes(64);
  game_memory.transient_storage_size = Megabytes(512);

  game_memory.permanent_storage = mmap(
      0, // Do I need to start at 0? can be NULL to let OS decide
      game_memory.permanent_storage_size + game_memory.transient_storage_size,
      PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

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

  tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

  game_init(&game_memory, &pixel_buffer);

  // float highestRate = 0.0f;
  // int nDisplays;
  // SDL_DisplayID *displays = SDL_GetDisplays(&nDisplays);
  // for (int di = 0; di < nDisplays; di++) {
  //   int nModes;
  //   SDL_DisplayMode **displayModes =
  //       SDL_GetFullscreenDisplayModes(displays[di], &nModes);
  //   for (int mi = 0; mi < nModes; mi++) {
  //     float rate = (*displayModes)[mi].refresh_rate;
  //     if (rate > highestRate)
  //       highestRate = rate;
  //   }
  //   SDL_free(displayModes);
  // }
  // SDL_Log("Highest available rate: %f", highestRate);
  // SDL_free(displays);

  local_persist Uint64 last_tick;
  local_persist Uint64 current_tick;
  local_persist float delta_time;

  local_persist game_input_t input[2] = {};
  local_persist game_input_t *new_input = &input[0];
  local_persist game_input_t *old_input = &input[1];

  while (!quit) {
    last_tick = current_tick;
    current_tick = SDL_GetTicks();
    delta_time = (current_tick - last_tick) / 1000.0f;

    *new_input = {};
    for (int button_i = 0; button_i < array_length(new_input->buttons);
         button_i++) {
      new_input->buttons[button_i].ended_down =
          old_input->buttons[button_i].ended_down;
    }

    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {

      if (event.type == SDL_EVENT_QUIT) {
        // Quit if told to
        quit = true;
      }
      if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        SDL_Log("Window Resized");
      }

      if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
        SDL_Log("Gamepad Removed");
      }

      if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        SDL_Log("Gamepad Added");
        joystickId = SDL_GetGamepads(&nGamepads);
        gamepad = SDL_OpenGamepad(*joystickId);
        SDL_Log("Gamepad Added %i", nGamepads);
      }

      PlatformHandleInputEvent(&event, new_input, old_input);
    }

    // // == DRAW ==
    PlatformUpdateAndDrawFrame(renderer, &destR, tex, &pixel_buffer);
    // // == END DRAW ==

    // // == UPDATE ==
    // // I don't think is the the correct way to do physics ticks
    // if ((SDL_GetTicks() - current_tick) < target_physics_time) {
    game_update_and_render(&game_memory, &pixel_buffer, new_input, delta_time);

    game_input_t *temp_input_ptr = new_input;
    new_input = old_input;
    old_input = temp_input_ptr;

    // }
    // // == END UPDATE ==

    // Limit fps
    Uint64 frame_time = SDL_GetTicks() - current_tick;
    if (frame_time < target_frame_time) {
      SDL_Delay(target_frame_time - frame_time);
    } else {
      // Missed framerate
      SDL_Log("Failed to hit framerate");
    }

  } // End game loop

  SDL_Quit();
  return 0;
}
