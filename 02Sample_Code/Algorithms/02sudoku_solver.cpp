#include <iostream>
#include <vector>
#include <string>

using Board = std::vector<std::vector<int>>;

// Print the board nicely with grid lines
void printBoard(const Board& board) {
    std::cout << "+-------+-------+-------+\n";
    for (int r = 0; r < 9; ++r) {
        std::cout << "| ";
        for (int c = 0; c < 9; ++c) {
            if (board[r][c] == 0)
                std::cout << ". ";
            else
                std::cout << board[r][c] << " ";
            if (c % 3 == 2) std::cout << "| ";
        }
        std::cout << "\n";
        if (r % 3 == 2)
            std::cout << "+-------+-------+-------+\n";
    }
}

// Check if placing 'num' at (row, col) is valid
bool isValid(const Board& board, int row, int col, int num) {
    // Check row
    for (int c = 0; c < 9; ++c)
        if (board[row][c] == num) return false;

    // Check column
    for (int r = 0; r < 9; ++r)
        if (board[r][col] == num) return false;

    // Check 3x3 box
    int boxRow = (row / 3) * 3;
    int boxCol = (col / 3) * 3;
    for (int r = boxRow; r < boxRow + 3; ++r)
        for (int c = boxCol; c < boxCol + 3; ++c)
            if (board[r][c] == num) return false;

    return true;
}

// Backtracking solver
bool solve(Board& board) {
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            if (board[row][col] != 0) continue;  // already filled

            for (int num = 1; num <= 9; ++num) {
                if (isValid(board, row, col, num)) {
                    board[row][col] = num;       // try

                    if (solve(board))            // recurse
                        return true;

                    board[row][col] = 0;         // backtrack
                }
            }
            return false;  // no number works here → dead end
        }
    }
    return true;  // all cells filled → solved
}

int main() {
    Board board(9, std::vector<int>(9, 0));

    std::cout << "Enter the Sudoku puzzle (9 lines, 9 digits each, 0 for empty):\n";
    std::cout << "(You can paste digits with or without spaces.)\n\n";

    for (int r = 0; r < 9; ++r) {
        std::string line;
        std::getline(std::cin, line);

        int col = 0;
        for (char ch : line) {
            if (ch >= '0' && ch <= '9' && col < 9) {
                board[r][col++] = ch - '0';
            }
        }
        if (col != 9) {
            std::cerr << "Error: row " << (r + 1) << " must contain 9 digits.\n";
            return 1;
        }
    }

    std::cout << "\nPuzzle:\n";
    printBoard(board);

    if (solve(board)) {
        std::cout << "\nSolved:\n";
        printBoard(board);
    } else {
        std::cout << "\nNo solution exists for this puzzle.\n";
    }

    return 0;
}