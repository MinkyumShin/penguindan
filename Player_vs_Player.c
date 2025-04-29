#include <stdio.h>
#include <stdlib.h>

#define SIZE 15

char board[SIZE][SIZE];

void initBoard() {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = '.';
}

void printBoard() {
    printf("   ");
    for (int i = 0; i < SIZE; i++) printf("%2d", i);
    printf("\n");

    for (int i = 0; i < SIZE; i++) {
        printf("%2d ", i);
        for (int j = 0; j < SIZE; j++) {
            printf("%2c", board[i][j]);
        }
        printf("\n");
    }
}

int checkWin(int x, int y, char player) {
    int dx[] = { 1, 0, 1, 1 }; // 가로, 세로, 대각선 
    int dy[] = {0, 1, 1, -1}; // 대각선 /

    for (int dir = 0; dir < 4; dir++) {
        int count = 1;

        for (int step = 1; step < 5; step++) {
            int nx = x + dx[dir] * step;
            int ny = y + dy[dir] * step;
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE) break;
            if (board[nx][ny] != player) break;
            count++;
        }

        for (int step = 1; step < 5; step++) {
            int nx = x - dx[dir] * step;
            int ny = y - dy[dir] * step;
            if (nx < 0 || ny < 0 || nx >= SIZE || ny >= SIZE) break;
            if (board[nx][ny] != player) break;
            count++;
        }

        if (count >= 5) return 1;
    }

    return 0;
}

int main() {
    int x, y;
    char currentPlayer = 'X';
    initBoard();

    while (1) {
        printBoard();
        printf("Player %c, enter your move (row column): ", currentPlayer);
        scanf_s("%d %d", &x, &y);

        if (x < 0 || x >= SIZE || y < 0 || y >= SIZE || board[x][y] != '.') {
            printf("Invalid move. Try again.\n");
            continue;
        }

        board[x][y] = currentPlayer;

        if (checkWin(x, y, currentPlayer)) {
            printBoard();
            printf("Player %c wins!\n", currentPlayer);
            break;
        }

        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
    }

    return 0;
}
