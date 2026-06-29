#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "raylib.h"

#define CELL_SIZE 100
#define GRID_COLOR BLACK
#define NUMBERS_SIZE 90
#define NUMBERS_COLOR BLACK
#define TIME_COLOR GRAY
#define TIME_SIZE 40
#define EMPTY 0
#define DELAY 100

#define index(x, y) 9 * (x) + (y)

// Set data structure implementation, an array of 9 digits, 0 if not in set, 1 if in set.
typedef struct
{
    int digits[9];
} Set;

typedef struct
{
    int grid[9][9];
    int solving;
    int solved;
} SolverThread;

Set *init_set()
{
    Set *set = (Set *)malloc(sizeof(Set));
    if (set == NULL)
    {
        printf("Failed to allocate memory for the set!\n");
        return NULL;
    }
    for (int i = 0; i < 9; i++)
    {
        set->digits[i] = 1;
    }
    return set;
}

Set *empty_set()
{
    Set *set = (Set *)malloc(sizeof(Set));
    if (set == NULL)
    {
        printf("Failed to allocate memory for the set!\n");
        return NULL;
    }
    return set;
}

int insert_set(Set *set, int value)
{
    if (value < 0 || value > 8)
    {
        printf("Value for insertion not between 1 and 9!\n");
        return -1;
    }
    set->digits[value] = 1;
    return 1;
}

int search_set(Set *set, int value)
{
    if (value < 0 || value > 8)
    {
        printf("Value for search not between 1 and 9!\n");
        return -1;
    }
    return set->digits[value];
}

int delete_set(Set *set, int value)
{
    if (value < 0 || value > 8)
    {
        printf("Value for deletion not between 1 and 9!\n");
        return -1;
    }
    set->digits[value] = 0;
    return 1;
}

char *print_set(Set *set)
{
    char *result = malloc(10);

    for (int i = 0; i < 9; i++)
    {
        if (search_set(set, i))
            result[i] = '0' + i + 1;
        else
            result[i] = ' ';
    }
    result[9] = '\0';

    return result;
}

void init_set_array_full(Set **sets)
{
    Set *set;
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            set = init_set();
            sets[i + 9 * j] = set;
        }
    }
}

void init_set_array_empty(Set **sets)
{
    Set *set;
    for (int i = 0; i < 9; i++)
    {
        set = init_set();
        for (int j = 0; j < 9; j++)
        {
            set->digits[j] = 0;
        }
        sets[i] = set;
    }
}

int size_set(Set *set)
{
    int count = 0;
    for (int i = 0; i < 9; i++)
    {
        count += set->digits[i];
    }
    return count;
}

Set *set_union(Set *set1, Set *set2)
{
    Set *result = empty_set();
    for (int i = 0; i < 9; i++)
    {
        result->digits[i] = set1->digits[i] || set2->digits[i];
    }
    return result;
}

Set *set_diff(Set *set1, Set *set2)
{
    Set *result = empty_set();
    for (int i = 0; i < 9; i++)
    {
        result->digits[i] = set1->digits[i] && !set2->digits[i];
    }
    return result;
}

Set *set_compl(Set *set)
{
    Set *result = empty_set();
    for (int i = 0; i < 9; i++)
    {
        result->digits[i] = !set->digits[i];
    }
    return result;
}
//------------------------------------------------------------

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
            char c[] = {solver->grid[i][j] == EMPTY ? ' ' : '0' + solver->grid[i][j], '\0'};
            DrawText(c, CELL_SIZE * j + 40, CELL_SIZE * i + 10, NUMBERS_SIZE, GRID_COLOR);
        }
    }
}

void print_time()
{
    double dtime = GetTime();
    char stime[30];
    snprintf(stime, 30, "%f", dtime);
    DrawText(stime, CELL_SIZE * 5, 10, TIME_SIZE, TIME_COLOR);
}

int calc_block(int i, int j)
{
    return (i / 3) * 3 + (j / 3);
}

void get_puzzle_from_file(char *path, SolverThread *solver)
{ // we assume that there are 9 rows with 0 in place of empty space.
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
    solver->solved = true;
    solver->solving = false;
    return NULL;
}

int main(int argc, char **argv)
{
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
    InitWindow(CELL_SIZE * 9 + 1, CELL_SIZE * 9 + 1, "Sudoku");
    SetTargetFPS(60);

    RenderTexture2D screen = LoadRenderTexture(CELL_SIZE * 9 + 1, CELL_SIZE * 9 + 1);
    BeginTextureMode(screen);
    ClearBackground(WHITE);
    print_grid();
    EndTextureMode();

    // Initialization:
    SolverThread *solver = malloc(sizeof(SolverThread));
    get_puzzle_from_file(filepath, solver);
    pthread_t thread;
    pthread_create(&thread, NULL, solve_thread, solver);

    while (!WindowShouldClose())
    {
        BeginTextureMode(screen);
        ClearBackground(WHITE);
        print_grid();
        print_numbers(solver);
        EndTextureMode();
        BeginDrawing();
        DrawTexturePro(screen.texture, (Rectangle){0, 0, (float)screen.texture.width, -(float)screen.texture.height}, (Rectangle){0, 0, (float)screen.texture.width, (float)screen.texture.height}, (Vector2){0, 0}, 0.0f, WHITE);
        EndDrawing();
    }

    pthread_join(thread, NULL);

    CloseWindow();
    return 0;
}