//gcc main.c -o main.out $(pkg-config --cflags --libs gtk+-3.0)
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BOARD_SIZE   19      // 오목판 크기
#define CELL_SIZE    30      // 셀 한 칸 크기 (픽셀)
#define SERVER_HOST  "127.0.0.1"
#define SERVER_PORT  8000
#define BUFFER_SIZE  1024

// GTK 전역 위젯
static GtkWidget *entry_username;
static GtkWidget *entry_password;
static GtkWidget *label_message;
static GtkApplication *app_global;  // GTK 애플리케이션 인스턴스

// 게임 보드 데이터 (0=빈칸, 1=흑, 2=백)
static int board_data[BOARD_SIZE][BOARD_SIZE] = {0};
static int current_player = 1;  // 1=흑, 2=백

static void launch_game_window(void);
static gboolean check_winner(int x, int y, int player);
static void on_app_activate(GtkApplication *app, gpointer user_data);
static void on_login_clicked(GtkButton *button, gpointer user_data);
static void on_register_clicked(GtkButton *button, gpointer user_data);

// ────────────────────────────────────────────────────────────────
// 네트워크: 서버에 op(id/pw) 요청 후 응답 문자열을 반환
static const char* send_request(const char* op, const char* id, const char* pw) {
    int sock;
    struct sockaddr_in serv_addr;
    static char buf[BUFFER_SIZE];
    char msg[BUFFER_SIZE];
    int n;

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "error: socket failed";

    // 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_HOST);

    // 서버에 연결
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return "error: connect failed";
    }

    // "op id pw" 형태로 메시지 작성
    snprintf(msg, sizeof(msg), "%s %s %s", op, id, pw);
    send(sock, msg, strlen(msg), 0);

    // 응답 수신
    n = recv(sock, buf, sizeof(buf)-1, 0);
    if (n <= 0) {
        close(sock);
        return "error: no response";
    }
    buf[n] = '\0';

    close(sock);
    return buf;
}

static void on_app_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Login / Register");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // UI 요소 생성
    GtkWidget *label_user = gtk_label_new("아이디:");
    GtkWidget *label_pass = gtk_label_new("비밀번호:");
    entry_username = gtk_entry_new();
    entry_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry_password), FALSE);
    GtkWidget *btn_login    = gtk_button_new_with_label("로그인");
    GtkWidget *btn_register = gtk_button_new_with_label("회원가입");
    label_message = gtk_label_new("");

    // 그리드에 배치
    gtk_grid_attach(GTK_GRID(grid), label_user,     0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_username, 1, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), label_pass,     0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_password, 1, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_login,      1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_register,   2, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_message,  0, 3, 3, 1);

    // 버튼 이벤트 연결
    g_signal_connect(btn_login,    "clicked", G_CALLBACK(on_login_clicked),    NULL);
    g_signal_connect(btn_register, "clicked", G_CALLBACK(on_register_clicked), NULL);

    gtk_widget_show_all(window);
}


// ────────────────────────────────────────────────────────────────
// GTK 핸들러: 회원가입 버튼 클릭
static void on_register_clicked(GtkButton *button, gpointer user_data) {
    const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(entry_password));
    const char *resp = send_request("register", username, password);

    if (strcmp(resp, "success") == 0) {
        gtk_label_set_text(GTK_LABEL(label_message), "회원가입 성공!");
    } else if (strcmp(resp, "exists") == 0) {
        gtk_label_set_text(GTK_LABEL(label_message), "이미 존재하는 아이디입니다.");
    } else {
        gtk_label_set_text(GTK_LABEL(label_message), resp);
    }
}

// ────────────────────────────────────────────────────────────────
// GTK 핸들러: 로그인 버튼 클릭
static void on_login_clicked(GtkButton *button, gpointer user_data) {
    const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(entry_password));
    const char *resp = send_request("login", username, password);

    if (strcmp(resp, "success") == 0) {
        gtk_label_set_text(GTK_LABEL(label_message), "로그인 성공!");
        launch_game_window();
    } else {
        gtk_label_set_text(GTK_LABEL(label_message), "로그인 실패: 아이디 또는 비밀번호 틀림");
    }
}

// ────────────────────────────────────────────────────────────────
// GTK: 오목판 그리기
static void draw_board(GtkWidget *widget, cairo_t *cr, gpointer data) {
    // 배경 칠하기
    cairo_set_source_rgb(cr, 0.9, 0.8, 0.6);
    cairo_paint(cr);

    // 격자 그리기
    cairo_set_source_rgb(cr, 0, 0, 0);
    for (int i = 0; i < BOARD_SIZE; i++) {
        cairo_move_to(cr, CELL_SIZE, CELL_SIZE + i * CELL_SIZE);
        cairo_line_to(cr, CELL_SIZE * BOARD_SIZE, CELL_SIZE + i * CELL_SIZE);
        cairo_move_to(cr, CELL_SIZE + i * CELL_SIZE, CELL_SIZE);
        cairo_line_to(cr, CELL_SIZE + i * CELL_SIZE, CELL_SIZE * BOARD_SIZE);
    }
    cairo_stroke(cr);

    // 돌 그리기
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (board_data[x][y] != 0) {
                if (board_data[x][y] == 1)
                    cairo_set_source_rgb(cr, 0, 0, 0);
                else
                    cairo_set_source_rgb(cr, 1, 1, 1);

                cairo_arc(cr,
                          CELL_SIZE + x * CELL_SIZE,
                          CELL_SIZE + y * CELL_SIZE,
                          CELL_SIZE/2 - 2,
                          0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }
}

// ────────────────────────────────────────────────────────────────
// GTK: 클릭 이벤트 핸들러 (돌 놓기)
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    int x = (event->x - CELL_SIZE / 2) / CELL_SIZE;
    int y = (event->y - CELL_SIZE / 2) / CELL_SIZE;

    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && board_data[x][y] == 0) {
        board_data[x][y] = current_player;

        // 승리 판정 추가
        if (check_winner(x, y, current_player)) {
            gtk_widget_queue_draw(widget);
            const char *winner = (current_player == 1) ? "흑 (X)" : "백 (O)";
            GtkWidget *dialog = gtk_message_dialog_new(NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_OK,
                "플레이어 %s 승리!", winner);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return TRUE;  // 클릭 차단 (게임 종료 후 처리는 너가 결정해도 됨)
        }

        current_player = 3 - current_player;  // 턴 교대
        gtk_widget_queue_draw(widget);        // 화면 다시 그림
    }

    return TRUE;
}


// ────────────────────────────────────────────────────────────────
// 오목 게임 창 생성
static void launch_game_window() {
    // 1) 새 창
    GtkWidget *game_win = gtk_application_window_new(app_global);
    gtk_window_set_title(GTK_WINDOW(game_win), "Omok Game");
    gtk_window_set_default_size(GTK_WINDOW(game_win), 600, 600);

    // 2) 그리기 영역
    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 600, 600);
    gtk_container_add(GTK_CONTAINER(game_win), drawing_area);

    // 3) draw_board와 on_button_press 연결
    g_signal_connect(drawing_area, "draw",
                     G_CALLBACK(draw_board), NULL);
    g_signal_connect(drawing_area, "button-press-event",
                     G_CALLBACK(on_button_press), NULL);
    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK);

    // 4) 모두 보이기
    gtk_widget_show_all(game_win);
}

static gboolean check_winner(int x, int y, int player) {
    const int dx[] = {1, 0, 1, 1};  // 가로, 세로, ↘ 대각선, ↗ 대각선
    const int dy[] = {0, 1, 1, -1};

    for (int dir = 0; dir < 4; dir++) {
        int count = 1;

        // 앞으로
        for (int step = 1; step < 5; step++) {
            int nx = x + dx[dir] * step;
            int ny = y + dy[dir] * step;
            if (nx < 0 || ny < 0 || nx >= BOARD_SIZE || ny >= BOARD_SIZE) break;
            if (board_data[nx][ny] != player) break;
            count++;
        }

        // 뒤로
        for (int step = 1; step < 5; step++) {
            int nx = x - dx[dir] * step;
            int ny = y - dy[dir] * step;
            if (nx < 0 || ny < 0 || nx >= BOARD_SIZE || ny >= BOARD_SIZE) break;
            if (board_data[nx][ny] != player) break;
            count++;
        }

        if (count >= 5) return TRUE;
    }
    return FALSE;
}

// ────────────────────────────────────────────────────────────────
// 메인 함수
int main(int argc, char **argv) {
    app_global = gtk_application_new("com.example.omokclient", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app_global, "activate", G_CALLBACK(on_app_activate), NULL);
    int status = g_application_run(G_APPLICATION(app_global), argc, argv);
    g_object_unref(app_global);
    return status;
}
