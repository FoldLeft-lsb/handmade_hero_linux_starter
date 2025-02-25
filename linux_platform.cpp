#include "lib/game.h"

#include <SDL3/SDL.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if STATIC_WHOLE_COMPILE

#include "lib/game.cpp"

#else

// This method of loading game functions from so
// does not account for debug information presently

game_init_t(*game_init_ptr) = NULL;
game_update_and_render_t(*game_update_and_render_ptr) = NULL;

void *game_lib_handle;
const char *game_lib_name = "./lib/libgame.so";

time_t prev_st_mtime;

internal_fn void PlatformLoadGameCodeLib() {

  char *error;

  game_lib_handle = dlopen(game_lib_name, RTLD_NOW);
  if (!game_lib_handle) {
    fputs(dlerror(), stderr);
    printf(" line %d\n", __LINE__);
    exit(1);
  }

  *(void **)(&game_init_ptr) = dlsym(game_lib_handle, "game_init");
  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    printf(" line %d\n", __LINE__);
  }
  *(void **)(&game_update_and_render_ptr) =
      dlsym(game_lib_handle, "game_update_and_render");
  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    printf(" line %d\n", __LINE__);
    exit(1);
  }

  struct stat attr;
  stat(game_lib_name, &attr);
  prev_st_mtime = attr.st_mtime;

  printf("Loaded game code from shared object\n");
}

internal_fn void PlatformReloadGameCodeLib() {
  struct stat attr;
  stat(game_lib_name, &attr);
  int new_st_mtime = attr.st_mtime;

  if (new_st_mtime > prev_st_mtime) {
    dlclose(game_lib_handle);
    // If I don't wait then dlopen fails because the so isn't written yet
    SDL_Delay(400);
    PlatformLoadGameCodeLib();
  }
}

#endif

// NOTE: Handmade Hero does sound from a buffer
// I couldn't figure it out with SDL3 so I haven't.
// Either I will figure it out, or just use SDL later
// without managing the buffer myself.

// File IO
// These functions are defined in game.h
// and implemented here, part of the platform
// layer that is accessible within the game.
// Is that OK? I don't know lol

Uint32 SafeTruncateUInt64(Uint64 value) {
  // assert
  Uint32 result = (Uint32)value;
  return (result);
}

debug_read_file_result_t DEBUGPlatformReadEntireFile(char *filename) {
  debug_read_file_result_t result = {};

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

// Input recording and playback

// // Use a file instead of memory
// typedef struct recorded_input {
//   int input_count;
//   game_input_t *input_stream;
// } recorded_input_t;

typedef struct platform_state {

  Uint64 game_memory_total_size;
  void *game_memory_block;

  int input_recording_file_descriptor;
  int input_recording_idx;

  int input_playback_file_descriptor;
  int input_playback_idx;

} platform_state_t;

const char *recording_filename = "foo.hmi";

internal_fn void PlatformBeginRecordingInput(platform_state_t *platform_state,
                                             int recording_idx) {
  if (platform_state->game_memory_total_size > 0xFFFFFFFF) {
    SDL_Log("Attempted to read game memory that exceeds max 32bit int");
    exit(1);
  };
  platform_state->input_recording_idx = recording_idx;
  platform_state->input_recording_file_descriptor =
      open(recording_filename, O_WRONLY | O_CREAT,
           S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (write(platform_state->input_recording_file_descriptor,
            platform_state->game_memory_block,
            platform_state->game_memory_total_size)) {
    SDL_Log("Recorded game memory block");
  } else {
    SDL_Log("Failed to record game memory block");
  }
}
internal_fn void PlatformEndRecordingInput(platform_state_t *platform_state) {
  platform_state->input_recording_idx = 0;
  close(platform_state->input_recording_file_descriptor);
}

internal_fn void PlatformBeginPlaybackInput(platform_state_t *platform_state,
                                            int playback_idx) {
  if (platform_state->game_memory_total_size > 0xFFFFFFFF) {
    SDL_Log("Attempted to write game memory that exceeds max 32bit int");
    exit(1);
  };
  platform_state->input_playback_idx = playback_idx;
  platform_state->input_playback_file_descriptor =
      open(recording_filename, O_RDONLY);
  if (read(platform_state->input_playback_file_descriptor,
           platform_state->game_memory_block,
           platform_state->game_memory_total_size)) {
    SDL_Log("Recovered game memory block");
  } else {
    SDL_Log("Failed to recover game memory block");
  }
}
internal_fn void PlatformEndPlaybackInput(platform_state_t *platform_state) {
  platform_state->input_playback_idx = 0;
  close(platform_state->input_playback_file_descriptor);
}

internal_fn void PlatformRecordInput(platform_state_t *platform_state,
                                     game_input_t *input) {

  if (write(platform_state->input_recording_file_descriptor, input,
            sizeof(*input))) {
    // SDL_Log("Recorded an input");
  } else {
    SDL_Log("Failed to record an input");
  }
}

internal_fn void PlatformPlaybackInput(platform_state_t *platform_state,
                                       game_input_t *input) {
  if (read(platform_state->input_recording_file_descriptor, input,
           sizeof(*input))) {
    // SDL_Log("Played back an input");
  } else {
    SDL_Log("Looping playback");
    int playing_idx = platform_state->input_playback_idx;
    PlatformEndPlaybackInput(platform_state);
    PlatformBeginPlaybackInput(platform_state, playing_idx);
  }
}

// end Input recording and playback

// Should eliminate some or all of these globals

global_variable int target_fps;
global_variable Uint64 target_frame_time;
global_variable int target_physics_updates_ps;
global_variable Uint64 target_physics_time;

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
// global_variable int right_stick_deadzone = 8000;

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
                                          game_input_t *old_input,
                                          platform_state_t *platform_state) {
  // Keyboard inputs
  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {

    if (event->key.key == SDLK_ESCAPE) {
      SDL_Log("Keyboard: Esc");
      quit = true;
    }

    if (event->key.repeat == 0) {

#if IN_DEVELOPMENT

      if (event->key.key == SDLK_L && event->key.down) {
        if (platform_state->input_recording_idx == 0) {
          PlatformEndPlaybackInput(platform_state);
          PlatformBeginRecordingInput(platform_state, 1);
        } else {
          PlatformEndRecordingInput(platform_state);
          PlatformBeginPlaybackInput(platform_state, 1);
        }
      }
#else
// Disable input recording and playback for non-DEV builds
#endif

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
  destR->w = pixel_buffer.width * scale;
  destR->h = pixel_buffer.height * scale;

  SDL_UpdateTexture(tex, NULL, &pixel_buffer.buffer,
                    pixel_buffer.width * pixel_buffer.bytes_per_px);

  SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
  SDL_RenderClear(renderer);

  SDL_RenderTexture(renderer, tex, NULL, destR);

  SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {

#if STATIC_WHOLE_COMPILE
#else

  PlatformLoadGameCodeLib();

#endif

  target_fps = 60;
  target_frame_time = 1000 / target_fps;
  target_physics_updates_ps = 30;
  target_physics_time = 1000 / target_physics_updates_ps;

  game_memory_t game_memory = {};
  game_memory.permanent_storage_size = Megabytes(64);
  game_memory.transient_storage_size = Megabytes(512);

#if IN_DEVELOPMENT

  // Clean address space with nothing in it means pointers
  // saved to file as binary should remain valid when read from file

  void *game_mem_base_addr = (void *)Terabytes(2);

#else

  // (void *)0 produces a nullptr, but I need an actual Zero value
  void *game_mem_base_addr = (void *)(1 - 1);

#endif

  platform_state_t platform_state = {
      .game_memory_total_size = 0,
      .game_memory_block = nullptr,

      .input_recording_file_descriptor = 0,
      .input_recording_idx = 0,
      .input_playback_file_descriptor = 0,
      .input_playback_idx = 0,
  };

  platform_state.game_memory_total_size =
      game_memory.permanent_storage_size + game_memory.transient_storage_size;

  platform_state.game_memory_block =
      mmap(game_mem_base_addr, platform_state.game_memory_total_size,
           PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

  game_memory.permanent_storage = platform_state.game_memory_block;
  game_memory.transient_storage = (Uint8 *)(platform_state.game_memory_block) +
                                  game_memory.permanent_storage_size;
  if (game_memory.permanent_storage == NULL ||
      game_memory.transient_storage == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Unable to allocate memory");
    return 1;
  }

  local_persist SDL_Window *window = NULL;
  local_persist SDL_Renderer *renderer = NULL;
  local_persist SDL_Texture *tex = NULL;
  local_persist SDL_FRect destR = (SDL_FRect){
      .x = 0,
      .y = 0,
  };

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
  SDL_CreateWindowAndRenderer("Hello SDL3", WIDTH * scale, HEIGHT * scale, 0,
                              &window, &renderer);
  SDL_SetWindowResizable(window, true);
  tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

#if STATIC_WHOLE_COMPILE

  game_init(&game_memory, &pixel_buffer);

#else
  if (game_init_ptr == NULL) {
    printf("game_init_ptr is NULL\n");
    exit(1);
  }
  if (game_update_and_render_ptr == NULL) {
    printf("game_update_and_render_ptr is NULL\n");
    exit(1);
  }

  (*game_init_ptr)(&game_memory, &pixel_buffer);

#endif

  // Query OS for info about monitors such as refresh rate

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

#if STATIC_WHOLE_COMPILE
#else

    PlatformReloadGameCodeLib();

    if (game_update_and_render_ptr == NULL) {
      printf("game_update_and_render_ptr is NULL\n");
      exit(1);
    }
#endif

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

      PlatformHandleInputEvent(&event, new_input, old_input, &platform_state);
    }

#if IN_DEVELOPMENT

    // // Input recording and playback

    if (platform_state.input_recording_idx) {
      PlatformRecordInput(&platform_state, new_input);
    }

    if (platform_state.input_playback_idx) {
      PlatformPlaybackInput(&platform_state, new_input);
    }

    // // end Input recording and playback

#else
// Disable input recording and playback for non-DEV builds
#endif

    // // == DRAW ==

    PlatformUpdateAndDrawFrame(renderer, &destR, tex, &pixel_buffer);

    // // == END DRAW ==

    // // == UPDATE ==

#if STATIC_WHOLE_COMPILE

    game_update_and_render(&game_memory, &pixel_buffer, new_input, delta_time);

#else

    (*game_update_and_render_ptr)(&game_memory, &pixel_buffer, new_input,
                                  delta_time);

#endif

    game_input_t *temp_input_ptr = new_input;
    new_input = old_input;
    old_input = temp_input_ptr;

    // // == END UPDATE ==

    // Limit fps
    Uint64 frame_time = SDL_GetTicks() - current_tick;

    if (frame_time > 12)
      SDL_Log("WARN: target: %ld, time: %ld", target_frame_time, frame_time);

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
