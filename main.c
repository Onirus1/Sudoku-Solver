#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>

#include <raylib.h>
#include <raymath.h> // required for Lerp.

#define CELL_SIZE 100
#define GRID_COLOR BLACK
#define NUMBERS_SIZE 90
#define NUMBERS_COLOR BLACK
#define TIME_COLOR GRAY
#define TIME_SIZE 70
#define EMPTY 0
#define DELAY 100

#define index(x, y) 9 * (x) + (y)

typedef struct
{
    int grid[9][9];
    int solving;
    int solved;
    double start_time;
    double end_time;
    pthread_mutex_t mutex;
} SolverThread;

void print_grid()
{
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            DrawRectangleLines(CELL_SIZE * j + 1, CELL_SIZE * i + 1, CELL_SIZE, CELL_SIZE, GRID_COLOR);
        }
    }
    for (int i = 0; i < 4; i++)
    {
        DrawLineEx((Vector2){CELL_SIZE * i * 3 + 1, 1}, (Vector2){CELL_SIZE * i * 3 + 1, CELL_SIZE * 9 + 1}, 3, GRID_COLOR);
        DrawLineEx((Vector2){1, CELL_SIZE * i * 3 + 1}, (Vector2){CELL_SIZE * 9 + 1, CELL_SIZE * i * 3 + 1}, 3, GRID_COLOR);
    }
}

void print_numbers(SolverThread *solver)
{
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            // Preparing everything needed to center align the text in each cell:
            Font def_font = GetFontDefault();
            char c[] = {solver->grid[i][j] == EMPTY ? ' ' : '0' + solver->grid[i][j], '\0'};
            Vector2 text_size = MeasureTextEx(def_font, c, NUMBERS_SIZE, NUMBERS_SIZE * .1f);
            Vector2 text_pos = (Vector2){CELL_SIZE * j + Lerp(0.0f, CELL_SIZE - text_size.x, 0.5f), CELL_SIZE * i + 5 + Lerp(0.0f, CELL_SIZE - text_size.y, 0.5f)};

            DrawTextEx(def_font, c, text_pos, (float)NUMBERS_SIZE, NUMBERS_SIZE * 0.2f, GRID_COLOR);
        }
    }
}

void print_timer(SolverThread *solver)
{
    char stime[30];
    if (solver->solving)
    {
        if (!solver->solved)
        {
            snprintf(stime, 30, "%f", GetTime() - solver->start_time);
            Font def_font = GetFontDefault();
            Vector2 text_size = MeasureTextEx(def_font, stime, TIME_SIZE, NUMBERS_SIZE * .1f);
            Rectangle bottom = (Rectangle){0.0f, (float)CELL_SIZE * 9, (float)CELL_SIZE * 9, (float)CELL_SIZE};
            Vector2 text_pos = (Vector2){bottom.x + Lerp(0.0f, bottom.width - text_size.x, 0.5f), bottom.y + Lerp(0.0f, bottom.height - text_size.y, 0.5f)};
            DrawTextEx(def_font, stime, text_pos, TIME_SIZE, TIME_SIZE * 0.1, GRID_COLOR);
        }
        else
        {
            snprintf(stime, 30, "%f", solver->end_time - solver->start_time);
            Font def_font = GetFontDefault();
            Vector2 text_size = MeasureTextEx(def_font, stime, TIME_SIZE, NUMBERS_SIZE * .1f);
            Rectangle bottom = (Rectangle){0.0f, (float)CELL_SIZE * 9, (float)CELL_SIZE * 9, (float)CELL_SIZE};
            Vector2 text_pos = (Vector2){bottom.x + Lerp(0.0f, bottom.width - text_size.x, 0.5f), bottom.y + Lerp(0.0f, bottom.height - text_size.y, 0.5f)};
            DrawTextEx(def_font, stime, text_pos, TIME_SIZE, TIME_SIZE * 0.1, GRID_COLOR);
        }
    }
}

void print_start_message()
{
    Font def_font = GetFontDefault();
    Vector2 text_size = MeasureTextEx(def_font, "Press R to start solver", TIME_SIZE * 0.8, NUMBERS_SIZE * .1f);
    Rectangle bottom = (Rectangle){0.0f, (float)CELL_SIZE * 9, (float)CELL_SIZE * 9, (float)CELL_SIZE};
    Vector2 text_pos = (Vector2){bottom.x + Lerp(0.0f, bottom.width - text_size.x, 0.5f), bottom.y + Lerp(0.0f, bottom.height - text_size.y, 0.5f)};
    DrawTextEx(def_font, "Press R to start solver", text_pos, TIME_SIZE * 0.8, TIME_SIZE * 0.8 * 0.1, GRID_COLOR);
}

int calc_block(int i, int j)
{
    return (i / 3) * 3 + (j / 3);
}

void get_puzzle_from_file(char *path, SolverThread *solver)
{ // we assume that there are 9 rows, 9 columns with 0 in place of empty space.
    FILE *f = fopen(path, "r");
    int count = 0;
    int num;

    while (fscanf(f, "%d", &num) == 1)
    {
        solver->grid[count / 9][count % 9] = num;
        count++;
    }
}

int check_cell_valid(int grid[9][9], int row, int col, int val) // checks if val can be placed in cell (row, col).
{
    // check row+col:
    for (int i = 0; i < 9; i++)
    {
        if (grid[row][i] == val || grid[i][col] == val)
            return 0;
    }
    // check block:
    int br = row - row % 3;
    int bc = col - col % 3;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (grid[br + i][bc + j] == val)
                return 0;
        }
    }
    return 1;
}

int solve_sudoku(int grid[9][9])
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1500000;

    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            if (grid[i][j] == EMPTY)
            { // for every empty cell
                for (int v = 1; v <= 9; v++)
                {
                    if (check_cell_valid(grid, i, j, v))
                    { // if the cell can be filled with v, temporarily fill it and check if the new grid is solvable. if it is not, empty the cell and continue.
                        grid[i][j] = v;
                        // nanosleep(&ts, NULL); // enable to delay.
                        if (solve_sudoku(grid))
                            return 1;
                        grid[i][j] = EMPTY;
                    }
                }
                return 0;
            }
        }
    }
    return 1;
}

void *solve_thread(void *arg)
{
    SolverThread *solver = (SolverThread *)arg;
    solve_sudoku(solver->grid);
    solver->end_time = GetTime();
    solver->solved = true;
    return NULL;
}

int main(int argc, char **argv)
{
    // Parsing input:
    char *filepath;
    struct stat buffer;
    if (argc == 1)
    {
        fprintf(stderr, "Usage: %s <file path>\n", argv[0]);
        return 1;
    }
    else if (argc > 2)
    {
        fprintf(stderr, "Error: Too many arguments were given!\n");
        fprintf(stderr, "Usage: %s <file path>\n", argv[0]);
        return 1;
    }
    else
    {
        filepath = argv[1];
        if (stat(filepath, &buffer) != 0)
        {
            fprintf(stderr, "Error: Cannot access file '%s'\n", filepath);
            return 1;
        }
    }

    // Opening window and drawing grid:
    InitWindow(CELL_SIZE * 9 + 1, CELL_SIZE * 9 + 1 + CELL_SIZE, "Sudoku"); // Space for the grid + timer.
    SetTargetFPS(60);

    RenderTexture2D screen = LoadRenderTexture(CELL_SIZE * 9 + 1, CELL_SIZE * 9 + 1 + CELL_SIZE);
    BeginTextureMode(screen);
    ClearBackground(WHITE);
    print_grid();
    print_start_message();
    EndTextureMode();

    // Initialization:
    SolverThread *solver = malloc(sizeof(SolverThread));
    get_puzzle_from_file(filepath, solver);
    pthread_t thread;
    pthread_mutex_init(&solver->mutex, NULL);

    while (!WindowShouldClose())
    {
        // Cheking for start condition:
        if (!solver->solving && IsKeyPressed(KEY_R))
        {
            solver->solving = 1;
            solver->start_time = GetTime(); // Recording starting time.
            pthread_create(&thread, NULL, solve_thread, solver);
        }

        BeginTextureMode(screen);
        ClearBackground(WHITE);
        print_grid();
        if (!solver->solving)
            print_start_message();
        print_numbers(solver);
        print_timer(solver);
        EndTextureMode();
        BeginDrawing();
        DrawTexturePro(screen.texture, (Rectangle){0, 0, (float)screen.texture.width, -(float)screen.texture.height}, (Rectangle){0, 0, (float)screen.texture.width, (float)screen.texture.height}, (Vector2){0, 0}, 0.0f, WHITE);
        EndDrawing();
    }

    pthread_join(thread, NULL);
    pthread_mutex_destroy(&solver->mutex);

    CloseWindow();
    return 0;
}