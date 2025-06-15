
#ifndef OMOK_AI_H
#define OMOK_AI_H

#define BOARD_SIZE 19
#define EMPTY 0
#define BLACK 1
#define WHITE 2

typedef struct {
    double score;
    int x;
    int y;
} MoveResult;

extern int evaluation_count;

double evaluate_board_for_white(int board[BOARD_SIZE][BOARD_SIZE]);
int get_score(int board[BOARD_SIZE][BOARD_SIZE], int player);
int evaluate_horizontal(int board[BOARD_SIZE][BOARD_SIZE], int player);
int evaluate_vertical(int board[BOARD_SIZE][BOARD_SIZE], int player);
int evaluate_diagonal(int board[BOARD_SIZE][BOARD_SIZE], int player);
MoveResult minimax_search_ab(int board[BOARD_SIZE][BOARD_SIZE], int depth, int is_max, double alpha, double beta);

#endif
