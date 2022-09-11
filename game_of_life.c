

#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mouse.h>
#define WINDOW_WIDTH (1000)
#define WINDOW_HEIGHT (650)

#define GRID_WIDTH (WINDOW_WIDTH / 10)
#define GRID_HEIGHT (WINDOW_HEIGHT / 10)

#define FORMAT SDL_PIXELFORMAT_ARGB8888

uint32_t *pixels;

SDL_Texture *tex;
SDL_Renderer *rend;

int loop()
{
    SDL_UpdateTexture(tex, NULL, pixels, GRID_WIDTH * sizeof(uint32_t));
    // clear the window
    SDL_RenderClear(rend);

    // draw the image to the window
    SDL_RenderCopy(rend, tex, NULL, NULL);
    SDL_RenderPresent(rend);

    // wait 1/60th of a second
    SDL_Delay(3333 / 60);

    // check if the window is closed
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {

        case SDL_WINDOWEVENT:

            switch (event.window.event)
            {

            case SDL_WINDOWEVENT_CLOSE: // exit game
                return 0;
                break;

            default:
                break;
            }
            break;
        }
    }

    return 1;
}

int init()
{
    // attempt to initialize graphics
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Hello, CS50!",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!win)
    {
        printf("error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // create a renderer, which sets up the graphics hardware
    Uint32 render_flags = SDL_RENDERER_ACCELERATED;
    rend = SDL_CreateRenderer(win, -1, render_flags);
    if (!rend)
    {
        printf("error creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    SDL_RenderClear(rend);

    // load the image data into the graphics hardware's memory
    tex = SDL_CreateTexture(
        rend,
        FORMAT,
        SDL_TEXTUREACCESS_STREAMING,
        GRID_WIDTH,
        GRID_HEIGHT);

    return 0;
}

int uninit()
{
    // clean up resources before exiting
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(rend);
    SDL_Quit();
    return 0;
}

typedef struct game
{
    uint32_t *pixels;
    int width;
    int height;
    uint32_t live_cell;
    uint32_t dead_cell;
    int wrap;
} game;

void game_fill_random(game *g, uint16_t start_state)
{
    uint16_t lfsr = start_state;
    uint16_t bit; /* Must be 16-bit to allow bit<<15 later in the code */

    for (int i = 0; i < g->width * g->height; i++)
    {

        bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1u;
        if (bit)
        {
            g->pixels[i] = g->live_cell;
        }
        else
        {
            g->pixels[i] = g->dead_cell;
        }
        lfsr = (lfsr >> 1) | (bit << 15);
    }
}

game *game_create(int width, int height, uint32_t live_cell, uint32_t dead_cell)
{
    game *g = malloc(sizeof(game));

    g->pixels = malloc(sizeof(uint32_t) * width * height);

    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            g->pixels[i + (j * width)] = dead_cell;
        }
    }

    g->width = width;
    g->height = height;
    g->live_cell = live_cell;
    g->dead_cell = dead_cell;
    g->wrap = 0;
    return g;
}
void game_free(game *g)
{
    free(g->pixels);
    free(g);
}
int count_neighbours(game *g, int x, int y, uint32_t val)
{

    int x1, x2, y1, y2;
    int nw, n, ne, w, e, sw, s, se;
    if (g->wrap)
    {

        x1 = x == 0 ? g->width - 1 : x - 1;
        x2 = x == g->width - 1 ? 0 : x + 1;
        y1 = y == 0 ? g->height - 1 : y - 1;
        y2 = y == g->height - 1 ? 0 : y + 1;

        nw = g->pixels[x1 + (y1 * g->width)] == val ? 1 : 0;
        n = g->pixels[x + (y1 * g->width)] == val ? 1 : 0;
        ne = g->pixels[x2 + (y1 * g->width)] == val ? 1 : 0;
        w = g->pixels[x1 + (y * g->width)] == val ? 1 : 0;
        e = g->pixels[x2 + (y * g->width)] == val ? 1 : 0;
        sw = g->pixels[x1 + (y2 * g->width)] == val ? 1 : 0;
        s = g->pixels[x + (y2 * g->width)] == val ? 1 : 0;
        se = g->pixels[x2 + (y2 * g->width)] == val ? 1 : 0;
    }
    else
    {
        nw = x > 0 && y > 0 ? g->pixels[x - 1 + ((y - 1) * g->width)] == val ? 1 : 0 : 0;
        n = y > 0 ? g->pixels[x + ((y - 1) * g->width)] == val ? 1 : 0 : 0;
        ne = x < g->width - 1 && y > 0 ? g->pixels[x + 1 + ((y - 1) * g->width)] == val ? 1 : 0 : 0;
        w = x > 0 ? g->pixels[x - 1 + (y * g->width)] == val ? 1 : 0 : 0;
        e = x < g->width - 1 ? g->pixels[x + 1 + (y * g->width)] == val ? 1 : 0 : 0;
        sw = x > 0 && y < g->height - 1 ? g->pixels[x - 1 + ((y + 1) * g->width)] == val ? 1 : 0 : 0;
        s = y < g->height - 1 ? g->pixels[x + ((y + 1) * g->width)] == val ? 1 : 0 : 0;
        se = x < g->width - 1 && y < g->height - 1 ? g->pixels[x + 1 + ((y + 1) * g->width)] == val ? 1 : 0 : 0;
    }

    int neighbours = 0;

    neighbours = nw + n + ne + w + e + sw + s + se;

    return neighbours;
}
// returns 1 if cell should be alive
int game_of_life(int neighbours, int alive)
{
    if (alive)
    {
        switch (neighbours)
        {
        case 2:
        case 3:
            return 1;
        default:
            return 0;
        }
    }
    else
    {
        switch (neighbours)
        {
        case 3:
            return 1;
        default:
            return 0;
        }
    }
}

int day_and_night(int neighbours, int alive)
{
    if (alive)
    {
        switch (neighbours)
        {
        case 3:
        case 4:
        case 6:
        case 7:
        case 8:

            return 1;
        default:
            return 0;
        }
    }
    else
    {
        switch (neighbours)
        {
        case 3:
        case 6:
        case 7:
        case 8:
            return 1;
        default:
            return 0;
        }
    }
}
void game_run_tick(game *curr, game *next)
{
    for (int y = 0; y < curr->height; y++)
    {
        for (int x = 0; x < curr->width; x++)
        {
            int alive = curr->pixels[x + (y * curr->width)] == curr->live_cell;
            int neighbours = count_neighbours(curr, x, y, curr->live_cell);
            int next_alive = game_of_life(neighbours, alive);
            next->pixels[x + (y * curr->width)] = next_alive ? curr->live_cell : curr->dead_cell;
        }
    }
}

int main(void)
{
    SDL_PixelFormat *fmt = SDL_AllocFormat(FORMAT);
    int white_cell = SDL_MapRGB(fmt, 255, 255, 255);
    int black_cell = SDL_MapRGB(fmt, 14, 15, 18);
    game *curr = game_create(GRID_WIDTH, GRID_HEIGHT, white_cell, black_cell);
    game *next = game_create(GRID_WIDTH, GRID_HEIGHT, white_cell, black_cell);
    game *tmp = NULL;
    init();
    // game_fill_random(curr, 568);
    //  draw Gospers_glider_gun machine on pixel
    curr->pixels[1 + (5 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[2 + (5 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[1 + (6 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[2 + (6 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[11 + (5 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[11 + (6 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[11 + (7 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[12 + (4 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[12 + (8 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[13 + (3 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[13 + (9 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[14 + (3 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[14 + (9 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[15 + (6 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[16 + (4 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[16 + (8 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[17 + (5 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[17 + (6 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[17 + (7 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[18 + (6 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[21 + (3 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[21 + (4 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[21 + (5 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[22 + (3 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[22 + (4 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[22 + (5 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[23 + (2 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[23 + (6 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[25 + (1 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[25 + (2 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[25 + (6 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[25 + (7 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[35 + (3 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[35 + (4 * GRID_WIDTH)] = curr->live_cell;

    curr->pixels[36 + (3 * GRID_WIDTH)] = curr->live_cell;
    curr->pixels[36 + (4 * GRID_WIDTH)] = curr->live_cell;

    while (1)
    {
        pixels = curr->pixels;

        if (!loop())
        {
            break;
        }
        game_run_tick(curr, next);
        tmp = curr;
        curr = next;
        next = tmp;
    }
    uninit();

    game_free(curr);
    game_free(next);
}
