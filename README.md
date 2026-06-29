# Sudoku Solver

A fast, interactive Sudoku solver written in C that uses backtracking to find valid solutions. The solver includes a graphical interface built with raylib.

## Features

- Backtracking algorithm for efficient puzzle solving
- Interactive GUI powered by raylib
- Command-line interface — load puzzles from text files
- Fast computation even for difficult puzzles

## Usage

./sudoku_solver <puzzle_file>

Replace <puzzle_file> with the path to your Sudoku puzzle file. The file should contain 81 digits (0–9), where 0 represents an empty cell.

### Example puzzle file:

5 3 0 0 7 0 0 0 0
6 0 0 1 9 5 0 0 0
0 9 8 0 0 0 0 6 0
8 0 0 0 6 0 0 0 3
4 0 0 8 0 3 0 0 1
7 0 0 0 2 0 0 0 6
0 6 0 0 0 0 2 8 0
0 0 0 4 1 9 0 0 5
0 0 0 0 8 0 0 7 9

## Building

### Prerequisites

- GCC (or compatible C compiler)
- raylib development library

### On Linux (Ubuntu/Debian):

sudo apt-get install libraylib-dev
gcc sudoku_solver.c -o sudoku_solver -lraylib -lm

### Compile and run:

./sudoku_solver puzzle.txt

## How It Works

The solver uses backtracking to systematically fill empty cells:

1. Find the next empty cell
2. Try digits 1–9
3. Check if the digit is valid (no conflicts in row, column, or 3×3 box)
4. If valid, place it and recurse
5. If no valid digit works, backtrack and try a different choice
6. Continue until solved or proven unsolvable

## License

MIT