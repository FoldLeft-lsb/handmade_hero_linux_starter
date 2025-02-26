#include "lib/game.h"

#include <SDL3/SDL.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// NOTE: Handmade Hero does sound from a buffer
// I couldn't figure it out with SDL3 so I haven't.
// Either I will figure it out, or just use SDL later
// without managing the buffer myself.

#if STATIC_WHOLE_COMPILE

#include "lib/game.cpp"

#else

// Should probably sort out debugger info

game_update_and_render_t(*game_update_and_render_ptr) = NULL;

void *game_lib_handle;
const char *game_lib_name = "./lib/libgame.so";

time_t prev_st_mtime;

internal_fn void PlatformLoadGameCodeLib() {

  char *error;

  game_lib_handle = dlopen(game_lib_name, RTLD_NOW);
  if (!game_lib_handle) {
    fputs(dlerror(), stderr);
    SDL_Log(" line %d", __LINE__);
    exit(1);
  }

  *(void **)(&game_update_and_render_ptr) =
      dlsym(game_lib_handle, "game_update_and_render");
  if ((error = dlerror()) != NULL) {
    fputs(error, stderr);
    SDL_Log(" line %d", __LINE__);
    exit(1);
  }

  struct stat attr;
  stat(game_lib_name, &attr);
  prev_st_mtime = attr.st_mtime;

  SDL_Log("Loaded game code from shared object");
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

// File IO

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

typedef struct platform_state {

  Uint64 game_memory_total_size;
  void *game_memory_block;

  int input_recording_file_descriptor;
  int input_recording_idx;

  int input_playback_file_descriptor;
  int input_playback_idx;

  bool recording;
  bool playing;

} platform_state_t;

const char *record_filename_format = "./tmp/playback_%d.dat";

internal_fn void PlatformBeginRecordingInput(platform_state_t *platform_state,
                                             int recording_idx) {
  if (platform_state->game_memory_total_size > 0xFFFFFFFF) {
    SDL_Log("Attempted to read game memory that exceeds max 32bit int");
    exit(1);
  };
  int namesize = 32;
  char name[namesize];
  SDL_snprintf(name, namesize, record_filename_format, recording_idx);
  platform_state->input_recording_idx = recording_idx;
  platform_state->input_recording_file_descriptor =
      open(name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (write(platform_state->input_recording_file_descriptor,
            platform_state->game_memory_block,
            platform_state->game_memory_total_size)) {
    SDL_Log("Recorded game memory block");
    platform_state->recording = true;
  } else {
    SDL_Log("Failed to record game memory block");
  }
}
internal_fn void PlatformEndRecordingInput(platform_state_t *platform_state) {
  platform_state->recording = false;
  close(platform_state->input_recording_file_descriptor);
}

internal_fn void PlatformBeginPlaybackInput(platform_state_t *platform_state,
                                            int playback_idx) {
  if (platform_state->game_memory_total_size > 0xFFFFFFFF) {
    SDL_Log("Attempted to write game memory that exceeds max 32bit int");
    exit(1);
  };
  int namesize = 32;
  char name[namesize];
  SDL_snprintf(name, namesize, record_filename_format, playback_idx);

  platform_state->input_playback_idx = playback_idx;
  platform_state->input_playback_file_descriptor = open(name, O_RDONLY);
  if (read(platform_state->input_playback_file_descriptor,
           platform_state->game_memory_block,
           platform_state->game_memory_total_size)) {
    SDL_Log("Recovered game memory block");
    platform_state->playing = true;
  } else {
    SDL_Log("Failed to recover game memory block");
  }
}
internal_fn void PlatformEndPlaybackInput(platform_state_t *platform_state) {
  platform_state->playing = false;
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
    read(platform_state->input_recording_file_descriptor, input,
         sizeof(*input));
  }
}

// end Input recording and playback

// Should eliminate some or all of these globals

global_variable int target_fps;
global_variable Uint64 target_frame_time;
global_variable int target_physics_updates_ps;
global_variable Uint64 target_physics_time;

global_variable bool quit = false;

const int scale = 1;

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

// end globals

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
  // Mouse for debug purposes
  // Generates multiple events per frame which is not desireable
  // uint leftclick = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_LMASK);
  // can be used to detect a mouse click

  float mouseX_f;
  float mouseY_f;
  SDL_GetMouseState(&mouseX_f, &mouseY_f);
  new_input->mouseX = (Uint32)mouseX_f;
  new_input->mouseY = (Uint32)mouseY_f;
  // new_input->mouseY = 0; // Support mouse wheel scroll?

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
      event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
    if (event->button.button == SDL_BUTTON_LEFT) {
      PlatformHandleInputButton(&new_input->left_click, event->button.down);
    }
    if (event->button.button == SDL_BUTTON_RIGHT) {
      PlatformHandleInputButton(&new_input->right_click, event->button.down);
    }
  }

  // Keyboard inputs
  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {

    if (event->key.key == SDLK_ESCAPE) {
      SDL_Log("Keyboard: Esc");
      quit = true;
    }

    if (event->key.repeat == 0) {

#if IN_DEVELOPMENT

      if ((!platform_state->recording) && (!platform_state->playing)) {
        if (event->key.mod & SDL_KMOD_ALT && event->key.key == SDLK_7 &&
            event->key.down) {
          platform_state->input_recording_idx = 0;
          platform_state->input_playback_idx = 0;
          SDL_Log("Save state slot 0");
        }
        if (event->key.mod & SDL_KMOD_ALT && event->key.key == SDLK_8 &&
            event->key.down) {
          platform_state->input_recording_idx = 1;
          platform_state->input_playback_idx = 1;
          SDL_Log("Save state slot 1");
        }
        if (event->key.mod & SDL_KMOD_ALT && event->key.key == SDLK_9 &&
            event->key.down) {
          platform_state->input_recording_idx = 2;
          platform_state->input_playback_idx = 2;
          SDL_Log("Save state slot 2");
        }
        if (event->key.mod & SDL_KMOD_ALT && event->key.key == SDLK_0 &&
            event->key.down) {
          platform_state->input_recording_idx = 3;
          platform_state->input_playback_idx = 3;
          SDL_Log("Save state slot 3");
        }
      }

      if (event->key.key == SDLK_L && event->key.down) {
        if (!platform_state->playing) {
          if (!platform_state->recording) {
            PlatformBeginRecordingInput(platform_state,
                                        platform_state->input_recording_idx);
          } else {
            PlatformEndRecordingInput(platform_state);
            PlatformBeginPlaybackInput(platform_state,
                                       platform_state->input_playback_idx);
          }
        } else {
          PlatformEndPlaybackInput(platform_state);
          new_input->controller = {};
        }
      }

#else
// Disable input recording and playback for non-DEV builds
#endif

      if (event->key.key == SDLK_W) {
        new_input->controller.is_analog = false;
        PlatformHandleInputButton(&new_input->controller.move_north,
                                  event->key.down);
      }
      if (event->key.key == SDLK_A) {
        new_input->controller.is_analog = false;
        PlatformHandleInputButton(&new_input->controller.move_west,
                                  event->key.down);
      }
      if (event->key.key == SDLK_S) {
        new_input->controller.is_analog = false;
        PlatformHandleInputButton(&new_input->controller.move_south,
                                  event->key.down);
      }
      if (event->key.key == SDLK_D) {
        new_input->controller.is_analog = false;
        PlatformHandleInputButton(&new_input->controller.move_east,
                                  event->key.down);
      }
      if (event->key.key == SDLK_UP) {
        new_input->controller.is_analog = false;
        PlatformHandleInputButton(&new_input->controller.move_north,
                                  event->key.down);
      }
      if (event->key.key == SDLK_LEFT) {
        new_input->controller.is_analog = false;
        PlatformHandleInputButton(&new_input->controller.move_west,
                                  event->key.down);
      }
      if (event->key.key == SDLK_DOWN) {
        new_input->controller.is_analog = false;
        PlatformHandleInputButton(&new_input->controller.move_south,
                                  event->key.down);
      }
      if (event->key.key == SDLK_RIGHT) {
        new_input->controller.is_analog = false;
        PlatformHandleInputButton(&new_input->controller.move_east,
                                  event->key.down);
      }

      if (event->key.key == SDLK_E) {
        PlatformHandleInputButton(&new_input->controller.action_south,
                                  event->key.down);
      }
      if (event->key.key == SDLK_F) {
        PlatformHandleInputButton(&new_input->controller.action_east,
                                  event->key.down);
      }
      if (event->key.key == SDLK_Q) {
        PlatformHandleInputButton(&new_input->controller.action_west,
                                  event->key.down);
      }
      if (event->key.key == SDLK_R) {
        PlatformHandleInputButton(&new_input->controller.action_north,
                                  event->key.down);
      }

      if (event->key.key == SDLK_LSHIFT) {
        PlatformHandleInputButton(&new_input->controller.left_shoulder,
                                  event->key.down);
      }
      if (event->key.key == SDLK_LCTRL) {
        PlatformHandleInputButton(&new_input->controller.right_shoulder,
                                  event->key.down);
      }

      if (event->key.key == SDLK_P) {
        PlatformHandleInputButton(&new_input->controller.start,
                                  event->key.down);
      }
      if (event->key.key == SDLK_I) {
        PlatformHandleInputButton(&new_input->controller.select,
                                  event->key.down);
      }
    }
  }

  // Gamepad inputs
  if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
    if (event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX ||
        event->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) {

      new_input->controller.is_analog = true;

      new_input->controller.left_stick_average_x =
          PlatformGetGamepadAxisValue(event->gaxis.value, left_stick_deadzone);
      new_input->controller.left_stick_average_y =
          PlatformGetGamepadAxisValue(event->gaxis.value, left_stick_deadzone);

      float threshold = 0.5f;
      PlatformHandleGamepadButton(
          &old_input->controller.move_west, &new_input->controller.move_west,
          new_input->controller.left_stick_average_x < -threshold);
      PlatformHandleGamepadButton(
          &old_input->controller.move_east, &new_input->controller.move_east,
          new_input->controller.left_stick_average_x > threshold);
      PlatformHandleGamepadButton(
          &old_input->controller.move_north, &new_input->controller.move_north,
          new_input->controller.left_stick_average_y < -threshold);
      PlatformHandleGamepadButton(
          &old_input->controller.move_south, &new_input->controller.move_south,
          new_input->controller.left_stick_average_y > threshold);
    }
  }

  if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ||
      event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) {

    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_START) {
      PlatformHandleInputButton(&new_input->controller.start,
                                event->gbutton.down);
      quit = true;
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK) {
      PlatformHandleInputButton(&new_input->controller.select,
                                event->gbutton.down);
    }

    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) {
      PlatformHandleInputButton(&new_input->controller.left_shoulder,
                                event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
      PlatformHandleInputButton(&new_input->controller.right_shoulder,
                                event->gbutton.down);
    }

    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_WEST) {
      PlatformHandleInputButton(&new_input->controller.action_west,
                                event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_NORTH) {
      PlatformHandleInputButton(&new_input->controller.action_north,
                                event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
      PlatformHandleInputButton(&new_input->controller.action_south,
                                event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST) {
      PlatformHandleInputButton(&new_input->controller.action_east,
                                event->gbutton.down);
    }

    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP) {
      new_input->controller.is_analog = false;
      PlatformHandleInputButton(&new_input->controller.move_north,
                                event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) {
      new_input->controller.is_analog = false;
      PlatformHandleInputButton(&new_input->controller.move_south,
                                event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) {
      new_input->controller.is_analog = false;
      PlatformHandleInputButton(&new_input->controller.move_west,
                                event->gbutton.down);
    }
    if (event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
      new_input->controller.is_analog = false;
      PlatformHandleInputButton(&new_input->controller.move_east,
                                event->gbutton.down);
    }
  }
}

void PlatformUpdateAndDrawFrame(SDL_Window *window, SDL_Renderer *renderer,
                                SDL_FRect *destR, SDL_Texture *tex,
                                offscreen_buffer *buffer) {
  destR->w = pixel_buffer.width * scale;
  destR->h = pixel_buffer.height * scale;
  int window_width;
  int window_height;
  int gutter_x = 0;
  int gutter_y = 0;
  SDL_GetWindowSizeInPixels(window, &window_width, &window_height);
  if (window_height > buffer->height) {
    gutter_y = (window_height - buffer->height) / 2;
  }
  if (window_width > buffer->width) {
    gutter_x = (window_width - buffer->width) / 2;
  }

  destR->x = gutter_x;
  destR->y = gutter_y;

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

  void *game_mem_base_addr = (void *)Terabytes(2);

#else

  // (void *)0 produces a nullptr, but I want an actual Zero value...
  void *game_mem_base_addr = (void *)(1 - 1);

#endif

  platform_state_t platform_state = {
      .game_memory_total_size = 0,
      .game_memory_block = nullptr,

      .input_recording_file_descriptor = 0,
      .input_recording_idx = 0,
      .input_playback_file_descriptor = 0,
      .input_playback_idx = 0,

      .recording = false,
      .playing = false,
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

  thread_context_t thread_context = {};

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
      SDL_Log("game_update_and_render_ptr is NULL");
      exit(1);
    }

#endif

    last_tick = current_tick;
    current_tick = SDL_GetTicks();
    delta_time = (current_tick - last_tick) / 1000.0f;

    *new_input = {};
    for (int button_i = 0;
         button_i < array_length(new_input->controller.buttons); button_i++) {
      new_input->controller.buttons[button_i].ended_down =
          old_input->controller.buttons[button_i].ended_down;
    }
    new_input->controller.is_analog = old_input->controller.is_analog;
    new_input->mouseX = old_input->mouseX;
    new_input->mouseY = old_input->mouseY;
    new_input->mouseZ = old_input->mouseZ;

    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {

      if (event.type == SDL_EVENT_QUIT) {
        // Quit if told to
        quit = true;
      }
      // if (event.type == SDL_EVENT_WINDOW_RESIZED) {
      //   SDL_Log("Window Resized");
      // }

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

    if (platform_state.recording) {
      PlatformRecordInput(&platform_state, new_input);
    }

    if (platform_state.playing) {
      PlatformPlaybackInput(&platform_state, new_input);
    }

    // // end Input recording and playback

#else
// Disable input recording and playback for non-DEV builds
#endif

    // Draw

    PlatformUpdateAndDrawFrame(window, renderer, &destR, tex, &pixel_buffer);

    // end Draw

    // Update

#if STATIC_WHOLE_COMPILE

    game_update_and_render(&thread_context, &game_memory, &pixel_buffer,
                           new_input, delta_time);

#else

    (*game_update_and_render_ptr)(&thread_context, &game_memory, &pixel_buffer,
                                  new_input, delta_time);

#endif

    game_input_t *temp_input_ptr = new_input;
    new_input = old_input;
    old_input = temp_input_ptr;

    // end Update

    Uint64 frame_time = SDL_GetTicks() - current_tick;

    if (frame_time > 12)
      SDL_Log("WARN: target: %ld, time: %ld", target_frame_time, frame_time);

    if (frame_time < target_frame_time) {
      SDL_Delay(target_frame_time - frame_time);
    } else {
      SDL_Log("Failed to hit framerate");
    }
  }

  SDL_Quit();
  return 0;
}
