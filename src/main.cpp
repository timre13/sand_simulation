#include <array>
#include <cassert>
#include <iostream>
#include <SDL2/SDL.h>

#define WORLD_WIDTH 200
#define WORLD_HEIGHT 200
#define CELL_SCALE 5

enum CellType : uint8_t
{
    CELL_TYPE_NONE,
    CELL_TYPE_SAND,
    //CELL_TYPE_WATER,
};

struct Cell
{
    CellType type : 4;
};

using World_t = std::array<Cell, WORLD_WIDTH*WORLD_HEIGHT>;

Cell& getParticle(World_t& world, int x, int y)
{
    assert(x >= 0 && x < WORLD_WIDTH);
    assert(y >= 0 && y < WORLD_HEIGHT);
    return world[y*WORLD_WIDTH+x];
}

const Cell& getParticle(const World_t& world, int x, int y)
{
    assert(x >= 0 && x < WORLD_WIDTH);
    assert(y >= 0 && y < WORLD_HEIGHT);
    return world[y*WORLD_WIDTH+x];
}

bool simulateSand(World_t* world, int x, int y, CellType newType, bool isEven)
{
    if (y == WORLD_HEIGHT-1)
        return false; // The bottom particles can't fall

    bool couldMove = false;

    // Below
    {
        Cell& cellBelow = getParticle(*world, x, y+1);
        if (cellBelow.type == CELL_TYPE_NONE)
        {
            cellBelow.type = newType;
            couldMove = true;
        }
    }

    // Left below
    auto doLeft{[&](){
        if (!couldMove && x > 0)
        {
            Cell& cellLeftBelow = getParticle(*world, x-1, y+1);
            if (cellLeftBelow.type == CELL_TYPE_NONE)
            {
                cellLeftBelow.type = newType;
                couldMove = true;
            }
        }
    }};

    // Right below
    auto doRight{[&](){
        if (!couldMove && x < WORLD_WIDTH-1)
        {
            Cell& cellRightBelow = getParticle(*world, x+1, y+1);
            if (cellRightBelow.type == CELL_TYPE_NONE)
            {
                cellRightBelow.type = newType;
                couldMove = true;
            }
        }
    }};

    if (isEven)
    {
        doLeft();
        doRight();
    }
    else
    {
        doRight();
        doLeft();
    }

    return couldMove;
}

void stepSimulation(World_t* world, ulong frame)
{
    const bool isEven = (frame % 2 == 0);
    for (int y{WORLD_HEIGHT-1}; y >= 0; --y)
    {
        for (int x{isEven ? 0 : WORLD_WIDTH-1}; (isEven ? x < WORLD_WIDTH : x >= 0); (isEven ? ++x : --x))
        {
            const Cell& cell = getParticle(*world, x, y);
            bool couldMove = false;

            switch (cell.type)
            {
            case CELL_TYPE_NONE:
                break;

            case CELL_TYPE_SAND:
                couldMove = simulateSand(world, x, y, CELL_TYPE_SAND, isEven);
                break;
                break;
            }

            if (couldMove)
                getParticle(*world, x, y).type = CELL_TYPE_NONE;
        }
    }
}

void drawWorld(const World_t& world, SDL_Renderer* rend)
{
    for (int y{}; y < WORLD_HEIGHT; ++y)
    {
        for (int x{}; x < WORLD_WIDTH; ++x)
        {
            const Cell& cell = getParticle(world, x, y);
            if (cell.type == CELL_TYPE_NONE)
                continue;

            switch (cell.type)
            {
            case CELL_TYPE_NONE: break;
            case CELL_TYPE_SAND: SDL_SetRenderDrawColor(rend, 153, 149, 125, 255);
            }

            SDL_Rect rect{x*CELL_SCALE, y*CELL_SCALE, CELL_SCALE, CELL_SCALE};
            SDL_RenderFillRect(rend, &rect);
        }
    }
}

int main()
{
    SDL_Window* win = SDL_CreateWindow("SandSim",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            WORLD_WIDTH*CELL_SCALE, WORLD_HEIGHT*CELL_SCALE,
            0);
    assert(win);

    SDL_Renderer* rend = SDL_CreateRenderer(win, 0, SDL_RENDERER_PRESENTVSYNC);
    assert(rend);

    World_t world{};

    ulong frame{};

    bool isLMouseBtnDown = false;
    bool isRMouseBtnDown = false;
    bool running = true;
    while (true)
    {
        SDL_Event event;
        while (running && SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    running = false;
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                        isLMouseBtnDown = true;
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        isRMouseBtnDown = true;
                    break;

                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT)
                        isLMouseBtnDown = false;
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        isRMouseBtnDown = false;
                    break;
            }
        }
        if (!running)
            break;

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        const bool isMouseInWindow
            = (mouseX >= 0 && mouseX < WORLD_WIDTH*CELL_SCALE)
            && (mouseY >= 0 && mouseY < WORLD_HEIGHT*CELL_SCALE);

        SDL_SetRenderDrawColor(rend, 50, 50, 50, 255);
        SDL_RenderClear(rend);

        stepSimulation(&world);
        stepSimulation(&world, frame);
        drawWorld(world, rend);

        SDL_RenderPresent(rend);
        //SDL_Delay(16);
        ++frame;
    }

    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(rend);
    SDL_Quit();
    return 0;
}
