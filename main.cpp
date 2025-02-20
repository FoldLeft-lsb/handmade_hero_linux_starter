#include <SDL3/SDL.h>
#include <iostream>

#define internal_fn static
#define local_persist static
#define global_variable static

#define WIDTH 768
#define HEIGHT 432
#define BYTES_PER_PX 4

const int buff_length = WIDTH * HEIGHT * BYTES_PER_PX;

const int scale = 2;

global_variable SDL_Window *window = NULL;
global_variable SDL_Renderer *renderer = NULL;

global_variable bool hasGamepad;
global_variable int nGamepads;
global_variable SDL_JoystickID *joystickId;
global_variable SDL_Gamepad *gamepad;

global_variable bool quit = false;

global_variable Uint8 pixel_buff[buff_length] = {};

global_variable SDL_Surface *surface;
global_variable SDL_Texture *tex;
global_variable SDL_FRect destR = (SDL_FRect){
    .x = 0,
    .y = 0,
};

internal_fn void update_pixels_alpha(Uint8 *pixelData, Uint8 alpha) {
  for (int i = 0; i < buff_length; i += BYTES_PER_PX) {
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
    pixelData[i + 3] = alpha;
  }
}

/*
  SDL_PIXELFORMAT_RGBA8888 means each pixel has 4 entries in the buffer
  { ..., [r]: 0x00, [g]: 0x00, [b]: 0x00, [a]: 0x00, ... }
  At 800*600 that means the buffer has 1,920,000 entries
  The frame update steps 4 per iteration, so that's 480,000 per frame

  Or 768x432 (divisible by 8) is 1,327,104 and 331,776

  It might be better to pack the pixel data into a single 32 bit int
  like they used to, but I don't know if it's worth the trouble
  */

int main() {

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);

  SDL_CreateWindowAndRenderer("Hello SDL3", WIDTH * scale, HEIGHT * scale, 0,
                              &window, &renderer);
  SDL_SetWindowResizable(window, true);

  for (int i = 0; i < buff_length; i++) {
    if (i % BYTES_PER_PX == 0 || i % BYTES_PER_PX == 3) {
      pixel_buff[i] = 0xFF;
    } else {
      pixel_buff[i] = 0x00;
    }
  }

  surface = SDL_CreateSurface(WIDTH, HEIGHT, SDL_PIXELFORMAT_RGBA8888);

  destR.w = WIDTH * scale;
  destR.h = HEIGHT * scale;

  // for testing animation
  local_persist Uint8 alpha = 0x00;

  while (!quit) {

    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {
      // Quit if told to
      if (event.type == SDL_EVENT_QUIT)
        quit = true;

      if (event.type == SDL_EVENT_GAMEPAD_REMOVED)
        std::cout << "Gamepad Removed" << std::endl;
      // Open gamepad if connected
      // This event fires if started with a gamepad available
      if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        std::cout << "Gamepad Added" << std::endl;
        joystickId = SDL_GetGamepads(&nGamepads);
        gamepad = SDL_OpenGamepad(*joystickId);
        std::cout << "Gamepads found: " << nGamepads << std::endl;
      }

      // Keyboard inputs
      if (event.type == SDL_EVENT_KEY_DOWN) {

        if (event.key.key == SDLK_ESCAPE) {
          std::cout << "Keyboard: Esc" << std::endl;
          quit = true;
        }

        if (event.key.key == SDLK_W)
          std::cout << "Keyboard: W" << std::endl;
        if (event.key.key == SDLK_A)
          std::cout << "Keyboard: A" << std::endl;
        if (event.key.key == SDLK_S)
          std::cout << "Keyboard: S" << std::endl;
        if (event.key.key == SDLK_D)
          std::cout << "Keyboard: D" << std::endl;

        if (event.key.key == SDLK_E)
          std::cout << "Keyboard: E" << std::endl;
        if (event.key.key == SDLK_P)
          std::cout << "Keyboard: P" << std::endl;
      }

      // Gamepad inputs
      if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {

        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_START) {
          std::cout << "Gamepad: Start" << std::endl;
          quit = true;
        }

        // I happen to have an xbox one controller
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST)
          std::cout << "Gamepad: X" << std::endl;
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_NORTH)
          std::cout << "Gamepad: Y" << std::endl;
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH)
          std::cout << "Gamepad: A" << std::endl;
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST)
          std::cout << "Gamepad: B" << std::endl;

        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
          std::cout << "Gamepad: Up" << std::endl;
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
          std::cout << "Gamepad: Down" << std::endl;
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
          std::cout << "Gamepad: Left" << std::endl;
        if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
          std::cout << "Gamepad: Right" << std::endl;
      }

      // if (event.type == SDL_EVENT_WINDOW_RESIZED)
      //   std::cout << "Window Resized" << std::endl;
      // Maybe change the scale or stretch the texture to the new screen size?
    }

    // for testing animation
    // this overflows Uint8 and wraps back to 0x00
    alpha++;

    update_pixels_alpha(pixel_buff, alpha);

    // Updated buffer overwrites to the surface
    surface->pixels = &pixel_buff;

    // Allocate a new texture each frame which must be cleared
    tex = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
    SDL_RenderClear(renderer);

    SDL_RenderTexture(renderer, tex, NULL, &destR);

    SDL_RenderPresent(renderer);

    // Clear the texture after drawing
    SDL_DestroyTexture(tex);
  }

  SDL_CloseGamepad(gamepad);
  SDL_free(joystickId);
  SDL_Quit();
  return 0;
}