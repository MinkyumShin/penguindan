#include <gtk/gtk.h>

#define BOARD_SIZE 19
#define CELL_SIZE 30

// 바둑판 데이터: 0 = 빈칸, 1 = 흑, 2 = 백
static int board[BOARD_SIZE][BOARD_SIZE] = {0};
static int current_player = 1; // 1부터 시작 (흑)

// 바둑판 그리기
static void draw_board(GtkWidget *widget, cairo_t *cr) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    // ✅ 연갈색 배경 채우기
    cairo_set_source_rgb(cr, 0.9, 0.8, 0.6); // 밝은 나무색
    cairo_paint(cr);

    // 🟫 검정색 선으로 바둑판 그리기
    cairo_set_source_rgb(cr, 0, 0, 0); // 검정색 선
    for (int i = 0; i < BOARD_SIZE; i++) {
        cairo_move_to(cr, CELL_SIZE, CELL_SIZE + i * CELL_SIZE);
        cairo_line_to(cr, CELL_SIZE * BOARD_SIZE, CELL_SIZE + i * CELL_SIZE);

        cairo_move_to(cr, CELL_SIZE + i * CELL_SIZE, CELL_SIZE);
        cairo_line_to(cr, CELL_SIZE + i * CELL_SIZE, CELL_SIZE * BOARD_SIZE);
    }
    cairo_stroke(cr);

    // 🔴 흑백 돌 그리기
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] != 0) {
                if (board[i][j] == 1)
                    cairo_set_source_rgb(cr, 0, 0, 0); // 흑돌
                else
                    cairo_set_source_rgb(cr, 1, 1, 1); // 백돌

                cairo_arc(cr, CELL_SIZE + i * CELL_SIZE, CELL_SIZE + j * CELL_SIZE,
                          CELL_SIZE / 2 - 2, 0, 2 * G_PI);
                cairo_fill(cr);
            }
        }
    }
}
// draw 신호 핸들러
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    draw_board(widget, cr);
    return FALSE;
}
// 마우스 클릭 핸들러
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    int x = (event->x - CELL_SIZE / 2) / CELL_SIZE;
    int y = (event->y - CELL_SIZE / 2) / CELL_SIZE;

    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
        if (board[x][y] == 0) {
            board[x][y] = current_player;
            current_player = 3 - current_player; // 1 ↔ 2 교대
            gtk_widget_queue_draw(widget); // 화면 다시 그리기
        }
    }
    return TRUE;
}
// 프로그램이 활성화될 때 호출되는 함수
static void on_app_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *drawing_area;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "GTK 오목 게임");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);

    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 600, 600);
    gtk_container_add(GTK_CONTAINER(window), drawing_area);

    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "button-press-event", G_CALLBACK(on_button_press), NULL);
    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK);

    gtk_widget_show_all(window);
}
// 메인 함수
int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.omok", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
