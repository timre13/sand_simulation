#include <array>
#include <cassert>
#include <iostream>
#include <SDL2/SDL.h>

#define WORLD_WIDTH 700
#define WORLD_HEIGHT 500
#define CELL_SCALE 2
#define FIRE_DEF_LIFE 30

#define BRUSH_RAD 5

enum CellType : uint8_t
{
    CELL_TYPE_NONE, // Has to be the first
    CELL_TYPE_SAND,
    CELL_TYPE_WATER,
    CELL_TYPE_DIRT,
    CELL_TYPE_WOOD,
    CELL_TYPE_FIRE,
    // Add new here
    CELL_TYPE__COUNT,
};

static constexpr SDL_Color cellTypeColors[CELL_TYPE__COUNT] = {
    {100, 100, 100, 255}, // None - background
    {153, 149, 125, 255}, // Sand
    { 50,  50, 255, 255}, // Water
    { 65,  44,  23, 255}, // Dirt
    { 90,  74,  43, 255}, // Wood
    {198,  80,  31, 255}, // Fire
};

#define UNPACK_COLOR_RGB(x) x.r, x.g, x.b

struct Cell
{
    CellType type : 4;
    int lifeRemaining = FIRE_DEF_LIFE; // Only if fire
    bool isModified = true;
};

using World_t = std::array<Cell, WORLD_WIDTH*WORLD_HEIGHT>;

Cell& getCell(World_t& world, int x, int y)
{
    assert(x >= 0 && x < WORLD_WIDTH);
    assert(y >= 0 && y < WORLD_HEIGHT);
    return world[y*WORLD_WIDTH+x];
}

const Cell& getCell(const World_t& world, int x, int y)
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

    auto handleCell = [world, x, y, newType](int relX, int relY){
        Cell& checkedCell = getCell(*world, x+relX, y+relY);
        if (checkedCell.type == CELL_TYPE_NONE || (newType == CELL_TYPE_SAND && checkedCell.type == CELL_TYPE_WATER))
        {
            auto& cell = getCell(*world, x, y);
            cell.type = ((newType == CELL_TYPE_SAND && checkedCell.type == CELL_TYPE_WATER) ? CELL_TYPE_WATER : CELL_TYPE_NONE);
            cell.isModified = true;
            checkedCell.type = newType;
            checkedCell.isModified = true;
            return true;
        }
        return false;
    };

    // Below
    couldMove = handleCell(0, 1);

    // Left below
    auto doLeft{[&](){
        if (!couldMove && x > 0)
            couldMove = handleCell(-1, 1);
    }};

    // Right below
    auto doRight{[&](){
        if (!couldMove && x < WORLD_WIDTH-1)
            couldMove = handleCell(1, 1);
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

bool simulateWater(World_t* world, int x, int y, CellType newType, bool isEven)
{
    bool couldMove = false;

    couldMove = simulateSand(world, x, y, newType, isEven);

    auto handleCell = [world, x, y, newType](int relX, int relY){
        Cell& checkedCell = getCell(*world, x+relX, y+relY);
        if (checkedCell.type == CELL_TYPE_NONE)
        {
            checkedCell.type = newType;
            checkedCell.isModified = true;
            auto& cell = getCell(*world, x, y);
            cell.type = CELL_TYPE_NONE;
            cell.isModified = true;
            return true;
        }
        return false;
    };

    auto doLeft{[&](){
        if (!couldMove && x > 0)
            couldMove = handleCell(-1, 0);
    }};

    auto doRight{[&](){
        if (!couldMove && x < WORLD_WIDTH-1)
            couldMove = handleCell(1, 0);
    }};

    if (isEven)
    {
        doRight();
        doLeft();
    }
    else
    {
        doLeft();
        doRight();
    }

    return couldMove;
}

void simulateFire(World_t* world, int x, int y)
{
    if (std::rand() % 10 == 0)
    {
        auto& cell = getCell(*world, x, y);
        --cell.lifeRemaining;
        if (cell.lifeRemaining <= 0)
        {
            cell.type = CELL_TYPE_NONE;
            cell.isModified = true;
            return;
        }
    }

    if (std::rand() % 20 != 0)
    {
        return;
    }

    auto handleCell = [world, x, y](int relX, int relY){
        Cell& checkedCell = getCell(*world, x+relX, y+relY);
        if (checkedCell.type == CELL_TYPE_WOOD)
        {
            checkedCell.type = CELL_TYPE_FIRE;
            checkedCell.lifeRemaining = FIRE_DEF_LIFE;
            checkedCell.isModified = true;
        }
    };

    // Above
    if (y > 0)
        handleCell(0, -1);

    // Below
    if (y < WORLD_HEIGHT-1)
        handleCell(0, 1);

    // Left
    if (x > 0)
        handleCell(-1, 0);

    // Right
    if (x < WORLD_WIDTH-1)
        handleCell(1, 0);

    // Left below
    if (x > 0 && y < WORLD_HEIGHT-1)
        handleCell(-1, 1);

    // Right below
    if (x < WORLD_WIDTH-1 && y < WORLD_HEIGHT-1)
        handleCell(1, 1);
}

void stepSimulation(World_t* world, ulong frame)
{
    const bool isEven = (frame % 2 == 0);
    for (int y{WORLD_HEIGHT-1}; y >= 0; --y)
    {
        for (int x{isEven ? 0 : WORLD_WIDTH-1}; (isEven ? x < WORLD_WIDTH : x >= 0); (isEven ? ++x : --x))
        {
            const Cell& cell = getCell(*world, x, y);

            switch (cell.type)
            {
            case CELL_TYPE_NONE:
            case CELL_TYPE__COUNT:
            case CELL_TYPE_DIRT: // Static cell
            case CELL_TYPE_WOOD: // Static cell
                break;

            case CELL_TYPE_SAND:
                simulateSand(world, x, y, CELL_TYPE_SAND, isEven);
                break;

            case CELL_TYPE_WATER:
                simulateWater(world, x, y, CELL_TYPE_WATER, isEven);
                break;

            case CELL_TYPE_FIRE:
                simulateFire(world, x, y);
                break;
            }
        }
    }
}

#define REND_TEXT_PIXEL_FORMAT_ENUM SDL_PIXELFORMAT_RGBA32

void drawWorld(World_t& world, SDL_Texture* tex, SDL_PixelFormat* pixFormat)
{
    uint32_t* pixels;
    int pitch;
    int ret = SDL_LockTexture(tex, nullptr, (void**)&pixels, &pitch);
    assert(ret == 0);

    for (int y{}; y < WORLD_HEIGHT; ++y)
    {
        for (int x{}; x < WORLD_WIDTH; ++x)
        {
            Cell& cell = getCell(world, x, y);
            if (cell.isModified)
            {
                pixels[y*WORLD_WIDTH+x] = SDL_MapRGBA(pixFormat, UNPACK_COLOR_RGB(cellTypeColors[cell.type]), 255);
                cell.isModified = false;
            }
        }
    }

    SDL_UnlockTexture(tex);
}

void paintCells(World_t* world, uint centerX, uint centerY, int radius, CellType type, bool randomize=true)
{
    const int rad2 = radius*radius;

    for (int yoffs{-radius}; yoffs <= radius; ++yoffs)
    {
        if (centerY+yoffs < 0 || centerY+yoffs >= WORLD_HEIGHT)
            continue;

        for (int xoffs{-radius}; xoffs <= radius; ++xoffs)
        {
            if (centerX+xoffs < 0 || centerX+xoffs >= WORLD_WIDTH)
                continue;

            if (xoffs*xoffs + yoffs*yoffs <= rad2 && (!randomize || rand() % 2))
            {
                auto& cell = getCell(*world, centerX+xoffs, centerY+yoffs);
                cell.type = type;
                cell.isModified = true;
            }
        }
    }
}

void drawToolbar(SDL_Renderer* rend, CellType brushMaterial)
{
    assert(brushMaterial > CELL_TYPE_NONE && brushMaterial < CELL_TYPE__COUNT);

    static constexpr int rectSize = 40;

    for (int i{1}; i < CELL_TYPE__COUNT; ++i)
    {
        SDL_SetRenderDrawColor(rend, UNPACK_COLOR_RGB(cellTypeColors[i]), 255);
        SDL_Rect rect{WORLD_WIDTH*CELL_SCALE-(rectSize+5), 5+(i-1)*rectSize, rectSize, rectSize};
        SDL_RenderFillRect(rend, &rect);

        if (i == brushMaterial)
        {
            SDL_SetRenderDrawColor(rend, 255, 255, 255, 255);
            SDL_RenderDrawRect(rend, &rect);
        }
    }
}

int main()
{
    std::srand(time(nullptr));

    SDL_Window* win = SDL_CreateWindow("SandSim",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            WORLD_WIDTH*CELL_SCALE, WORLD_HEIGHT*CELL_SCALE,
            0);
    assert(win);

    SDL_Renderer* rend = SDL_CreateRenderer(win, 0, SDL_RENDERER_PRESENTVSYNC);
    assert(rend);

    SDL_Texture* rendTex = SDL_CreateTexture(rend, REND_TEXT_PIXEL_FORMAT_ENUM, SDL_TEXTUREACCESS_STREAMING, WORLD_WIDTH, WORLD_HEIGHT);
    assert(rendTex);

    SDL_PixelFormat* rendTexPixFormat = SDL_AllocFormat(REND_TEXT_PIXEL_FORMAT_ENUM);
    assert(rendTexPixFormat);

    World_t world{};

    ulong frame{};
    CellType brushMaterial = CELL_TYPE_SAND;

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

                case SDL_MOUSEWHEEL:
                    if (event.wheel.y > 0)
                        (int&)brushMaterial -= 1;
                    else
                        (int&)brushMaterial += 1;

                    if (brushMaterial <= CELL_TYPE_NONE)
                        brushMaterial = CellType(CELL_TYPE_NONE+1);
                    else if (brushMaterial >= CELL_TYPE__COUNT)
                        brushMaterial = CellType(CELL_TYPE__COUNT-1);
            }
        }
        if (!running)
            break;

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        const bool isMouseInWindow
            = (mouseX >= 0 && mouseX < WORLD_WIDTH*CELL_SCALE)
            && (mouseY >= 0 && mouseY < WORLD_HEIGHT*CELL_SCALE);

        if (isMouseInWindow && isLMouseBtnDown)
        {
            const int cellX = mouseX/CELL_SCALE;
            const int cellY = mouseY/CELL_SCALE;
            paintCells(&world, cellX, cellY, BRUSH_RAD, brushMaterial);
        }
        if (isMouseInWindow && isRMouseBtnDown)
        {
            const int cellX = mouseX/CELL_SCALE;
            const int cellY = mouseY/CELL_SCALE;
            paintCells(&world, cellX, cellY, BRUSH_RAD, CELL_TYPE_NONE, false);
        }

        stepSimulation(&world, frame);

        const uint64_t renderStart = SDL_GetTicks64();

        drawWorld(world, rendTex, rendTexPixFormat);
        SDL_RenderCopy(rend, rendTex, nullptr, nullptr);
        drawToolbar(rend, brushMaterial);

        const uint64_t renderTime = SDL_GetTicks64()-renderStart;
        SDL_SetWindowTitle(win, ("SandSim - render time: "+std::to_string(renderTime)+"ms").c_str());

        SDL_RenderPresent(rend);
        //SDL_Delay(16);
        ++frame;
    }

    SDL_FreeFormat(rendTexPixFormat);
    SDL_DestroyTexture(rendTex);
    SDL_DestroyWindow(win);
    SDL_DestroyRenderer(rend);
    SDL_Quit();
    return 0;
}
