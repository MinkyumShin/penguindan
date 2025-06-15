
#include <stdio.h>
#include <float.h>
#include "omok_ai.h"

int evaluation_count = 0;

int has_neighbor(int x, int y, int board[BOARD_SIZE][BOARD_SIZE]) {
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -2; dy <= 2; dy++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx < 0 || ny < 0 || nx >= BOARD_SIZE || ny >= BOARD_SIZE) continue;
            if (board[nx][ny] != EMPTY) return 1;
        }
    }
    return 0;
}

int evaluate_horizontal(int board[BOARD_SIZE][BOARD_SIZE], int player) {
    int score = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        int x = 0;
        while (x < BOARD_SIZE) {
            if (board[x][y] == player) {
                int count = 0;
                int start = x;
                while (x < BOARD_SIZE && board[x][y] == player) { count++; x++; }

                int left_open = (start - 1 >= 0 && board[start - 1][y] == EMPTY);
                int right_open = (x < BOARD_SIZE && board[x][y] == EMPTY);
                int open_ends = left_open + right_open;

                if (count >= 5) score += 1000000;
                else if (count == 4) score += open_ends == 2 ? 10000 : 1000;
                else if (count == 3) score += open_ends == 2 ? 500 : 100;
                else if (count == 2) score += open_ends == 2 ? 50 : 10;
                else if (count == 1 && open_ends == 2) score += 5;
            } else x++;
        }
    }
    return score;
}

int evaluate_vertical(int board[BOARD_SIZE][BOARD_SIZE], int player) {
    int score = 0;
    for (int x = 0; x < BOARD_SIZE; x++) {
        int y = 0;
        while (y < BOARD_SIZE) {
            if (board[x][y] == player) {
                int count = 0;
                int start = y;
                while (y < BOARD_SIZE && board[x][y] == player) { count++; y++; }

                int up_open = (start - 1 >= 0 && board[x][start - 1] == EMPTY);
                int down_open = (y < BOARD_SIZE && board[x][y] == EMPTY);
                int open_ends = up_open + down_open;

                if (count >= 5) score += 1000000;
                else if (count == 4) score += open_ends == 2 ? 10000 : 1000;
                else if (count == 3) score += open_ends == 2 ? 500 : 100;
                else if (count == 2) score += open_ends == 2 ? 50 : 10;
                else if (count == 1 && open_ends == 2) score += 5;
            } else y++;
        }
    }
    return score;
}

int evaluate_diagonal(int board[BOARD_SIZE][BOARD_SIZE], int player) {
    int score = 0;
    // ↘ 방향
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            int x = i, y = j;
            while (x < BOARD_SIZE && y < BOARD_SIZE) {
                if (board[x][y] == player) {
                    int count = 0, sx = x, sy = y;
                    while (x < BOARD_SIZE && y < BOARD_SIZE && board[x][y] == player) { count++; x++; y++; }
                    int left_open = (sx - 1 >= 0 && sy - 1 >= 0 && board[sx - 1][sy - 1] == EMPTY);
                    int right_open = (x < BOARD_SIZE && y < BOARD_SIZE && board[x][y] == EMPTY);
                    int open_ends = left_open + right_open;

                    if (count >= 5) score += 1000000;
                    else if (count == 4) score += open_ends == 2 ? 10000 : 1000;
                    else if (count == 3) score += open_ends == 2 ? 500 : 100;
                    else if (count == 2) score += open_ends == 2 ? 50 : 10;
                    else if (count == 1 && open_ends == 2) score += 5;
                } else { x++; y++; }
            }
        }
    }
    // ↗ 방향
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            int x = i, y = j;
            while (x < BOARD_SIZE && y >= 0) {
                if (board[x][y] == player) {
                    int count = 0, sx = x, sy = y;
                    while (x < BOARD_SIZE && y >= 0 && board[x][y] == player) { count++; x++; y--; }
                    int left_open = (sx - 1 >= 0 && sy + 1 < BOARD_SIZE && board[sx - 1][sy + 1] == EMPTY);
                    int right_open = (x < BOARD_SIZE && y >= 0 && board[x][y] == EMPTY);
                    int open_ends = left_open + right_open;

                    if (count >= 5) score += 1000000;
                    else if (count == 4) score += open_ends == 2 ? 10000 : 1000;
                    else if (count == 3) score += open_ends == 2 ? 500 : 100;
                    else if (count == 2) score += open_ends == 2 ? 50 : 10;
                    else if (count == 1 && open_ends == 2) score += 5;
                } else { x++; y--; }
            }
        }
    }
    return score;
}

int get_score(int board[BOARD_SIZE][BOARD_SIZE], int player) {
    return evaluate_horizontal(board, player) +
           evaluate_vertical(board, player) +
           evaluate_diagonal(board, player);
}

double evaluate_board_for_white(int board[BOARD_SIZE][BOARD_SIZE]) {
    evaluation_count++;
    int black_score = get_score(board, BLACK);
    int white_score = get_score(board, WHITE);
    if (black_score == 0) black_score = 1;
    return (double)white_score / black_score;
}

MoveResult minimax_search_ab(int board[BOARD_SIZE][BOARD_SIZE], int depth, int is_max, double alpha, double beta) {
    MoveResult best_move;
    best_move.score = is_max ? -DBL_MAX : DBL_MAX;
    best_move.x = -1;
    best_move.y = -1;

    if (depth == 0) {
        best_move.score = evaluate_board_for_white(board);
        return best_move;
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board[x][y] != EMPTY || !has_neighbor(x, y, board)) continue;

            board[x][y] = is_max ? WHITE : BLACK;
            MoveResult result = minimax_search_ab(board, depth - 1, !is_max, alpha, beta);
            board[x][y] = EMPTY;

            if (is_max) {
                if (result.score > best_move.score) {
                    best_move = result;
                    best_move.x = x;
                    best_move.y = y;
                }
                if (result.score > alpha) alpha = result.score;
            } else {
                if (result.score < best_move.score) {
                    best_move = result;
                    best_move.x = x;
                    best_move.y = y;
                }
                if (result.score < beta) beta = result.score;
            }

            if (beta <= alpha)
                return best_move;
        }
    }

    return best_move;
}
