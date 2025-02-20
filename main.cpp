#include <SDL3/SDL.h>
// #include <cstdio>

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

global_variable bool quit = false;

global_variable Uint8 pixel_buff[buff_length] = {};

global_variable SDL_Surface *surface;
global_variable SDL_Texture *tex;
global_variable SDL_FRect destR;

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

  SDL_Init(SDL_INIT_VIDEO);

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

  destR.x = 0;
  destR.y = 0;
  destR.w = WIDTH * scale;
  destR.h = HEIGHT * scale;

  // for testing animation
  local_persist Uint8 alpha = 0x00;

  while (!quit) {

    SDL_Event event = {};
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        quit = true;
      if (event.type == SDL_EVENT_KEY_DOWN)
        if (event.key.key == SDLK_ESCAPE)
          quit = true;
      // if (event.type == SDL_EVENT_WINDOW_RESIZED)
      //   printf("Window Resized\n");
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

  SDL_Quit();
  return 0;
}