// gcc main.c omok_ai_module.c result_dialog.c -o main.out \
//     $(pkg-config --cflags --libs gtk+-3.0)
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "omok_ai.h"
#include "result_dialog.h"

#define BOARD_SIZE   19
#define CELL_SIZE    30
#define AI_DEPTH     2     // 변경: 깊이를 2로 낮춰 초기 과부하 방지

#define SERVER_HOST  "127.0.0.1"
#define SERVER_PORT  8000
#define BUFFER_SIZE  1024

/* ─── 전역 ─── */
GtkWidget      *entry_username;
GtkWidget      *entry_password;
GtkWidget      *label_message;
GtkApplication *global_app;

static char    current_user[32];
static int     board_data[BOARD_SIZE][BOARD_SIZE];
static int     current_player;
static gboolean ai_mode;

/* ─── 프로토타입 ─── */
static const char* send_request(const char* op, const char* id, const char* result);
static void      on_register_clicked(GtkButton*, gpointer);
static void      on_login_clicked   (GtkButton*, gpointer);
static void      show_mode_selection(GtkApplication*);
static void      launch_omok_game   (GtkApplication*, gboolean);
static gboolean  draw_board         (GtkWidget*, cairo_t*, gpointer);
static gboolean  on_button_press    (GtkWidget*, GdkEventButton*, gpointer);
static gboolean  check_winner       (int, int, int);
static void      activate           (GtkApplication*, gpointer);

/* ─── 서버 통신 ─── */
static const char* send_request(const char* op,
                                const char* id,
                                const char* result) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return "error: socket failed";

    struct sockaddr_in serv = {0};
    serv.sin_family = AF_INET;
    serv.sin_port   = htons(SERVER_PORT);
    serv.sin_addr.s_addr = inet_addr(SERVER_HOST);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        close(sock); return "error: connect failed";
    }

    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "%s %s %s", op, id, result);
    send(sock, msg, strlen(msg), 0);

    static char buf[BUFFER_SIZE];
    int n = recv(sock, buf, sizeof(buf)-1, 0);
    if (n <= 0) {
        close(sock); return "error: no response";
    }
    buf[n]='\0';
    close(sock);
    return buf;
}

/* ─── 회원가입 ─── */
static void on_register_clicked(GtkButton *button, gpointer _){
    const char *u = gtk_entry_get_text(GTK_ENTRY(entry_username));
    const char *p = gtk_entry_get_text(GTK_ENTRY(entry_password));

    FILE *f = fopen("users.txt","r");
    if (f){
        char uu[32], pp[32];
        while (fscanf(f,"%31s %31s", uu, pp)!=EOF){
            if (!strcmp(uu,u)){
                gtk_label_set_text(GTK_LABEL(label_message),"이미 존재하는 아이디입니다.");
                fclose(f);
                return;
            }
        }
        fclose(f);
    }
    f = fopen("users.txt","a");
    if (!f){
        gtk_label_set_text(GTK_LABEL(label_message),"파일 쓰기 실패");
        return;
    }
    fprintf(f,"%s %s\n",u,p);
    fclose(f);
    gtk_label_set_text(GTK_LABEL(label_message),"회원가입 성공!");
}

/* ─── 로그인 ─── */
static void on_login_clicked(GtkButton *button, gpointer _){
    const char *u = gtk_entry_get_text(GTK_ENTRY(entry_username));
    const char *p = gtk_entry_get_text(GTK_ENTRY(entry_password));

    FILE *f = fopen("users.txt","r");
    if (!f){
        gtk_label_set_text(GTK_LABEL(label_message),"사용자 데이터 없음");
        return;
    }
    char uu[32], pp[32];
    while (fscanf(f,"%31s %31s",uu,pp)!=EOF){
        if (!strcmp(uu,u) && !strcmp(pp,p)){
            fclose(f);
            strncpy(current_user, u, sizeof(current_user)-1);
            current_user[sizeof(current_user)-1]='\0';
            GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(button));
            gtk_widget_destroy(win);
            show_mode_selection(global_app);
            return;
        }
    }
    fclose(f);
    gtk_label_set_text(GTK_LABEL(label_message),"로그인 실패: 아이디 또는 비밀번호 틀림");
}

/* ─── 모드 선택 ─── */
static void show_mode_selection(GtkApplication *app){
    GtkWidget *dlg = gtk_message_dialog_new(NULL,
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
        "플레이 모드를 선택하세요.");
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
        "Two-Player", RESULT_RESTART,
        "vs AI",       RESULT_MODE_SELECT,
        NULL);
    gint resp = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    // RESULT_RESTART를 Two-Player, RESULT_MODE_SELECT를 vs AI 용으로 재해석
    ai_mode = (resp == RESULT_MODE_SELECT);
    launch_omok_game(app, ai_mode);
}

/* ─── 오목 창 ─── */
static void launch_omok_game(GtkApplication *app, gboolean ai_flag){
    global_app = app;
    memset(board_data,0,sizeof(board_data));
    current_player = BLACK;
    evaluation_count = 0;

    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win),
        ai_flag ? "Omok vs AI" : "Omok Two-Player");
    gtk_window_set_default_size(GTK_WINDOW(win),600,600);

    GtkWidget *area = gtk_drawing_area_new();
    gtk_widget_set_size_request(area,600,600);
    gtk_container_add(GTK_CONTAINER(win), area);

    g_signal_connect(area, "draw",             G_CALLBACK(draw_board),     NULL);
    g_signal_connect(area, "button-press-event",
                     G_CALLBACK(on_button_press), GINT_TO_POINTER(ai_flag));
    gtk_widget_add_events(area, GDK_BUTTON_PRESS_MASK);

    gtk_widget_show_all(win);
}

/* ─── 보드 그리기 ─── */
static gboolean draw_board(GtkWidget *w, cairo_t *cr, gpointer _){
    cairo_set_source_rgb(cr,.9,.8,.6); cairo_paint(cr);
    cairo_set_source_rgb(cr,0,0,0);
    for(int i=0;i<BOARD_SIZE;i++){
        cairo_move_to(cr,CELL_SIZE, CELL_SIZE+i*CELL_SIZE);
        cairo_line_to(cr,CELL_SIZE*BOARD_SIZE, CELL_SIZE+i*CELL_SIZE);
        cairo_move_to(cr,CELL_SIZE+i*CELL_SIZE, CELL_SIZE);
        cairo_line_to(cr,CELL_SIZE+i*CELL_SIZE, CELL_SIZE*BOARD_SIZE);
    }
    cairo_stroke(cr);
    for(int x=0;x<BOARD_SIZE;x++)for(int y=0;y<BOARD_SIZE;y++){
        if (board_data[x][y]!=EMPTY){
            if (board_data[x][y]==BLACK) cairo_set_source_rgb(cr,0,0,0);
            else                         cairo_set_source_rgb(cr,1,1,1);
            cairo_arc(cr,
                      CELL_SIZE+x*CELL_SIZE,
                      CELL_SIZE+y*CELL_SIZE,
                      CELL_SIZE/2-2,0,2*G_PI);
            cairo_fill(cr);
        }
    }
    return FALSE;
}

/* ─── 승리 체크 ─── */
static gboolean check_winner(int x,int y,int p){
    const int dx[4]={1,0,1,1}, dy[4]={0,1,1,-1};
    for(int dir=0;dir<4;dir++){
        int cnt=1;
        for(int s=1;s<5;s++){
            int nx=x+dx[dir]*s, ny=y+dy[dir]*s;
            if(nx<0||ny<0||nx>=BOARD_SIZE||ny>=BOARD_SIZE||board_data[nx][ny]!=p) break;
            cnt++;
        }
        for(int s=1;s<5;s++){
            int nx=x-dx[dir]*s, ny=y-dy[dir]*s;
            if(nx<0||ny<0||nx>=BOARD_SIZE||ny>=BOARD_SIZE||board_data[nx][ny]!=p) break;
            cnt++;
        }
        if(cnt>=5) return TRUE;
    }
    return FALSE;
}

/* ─── 클릭 이벤트 ─── */
static gboolean on_button_press(GtkWidget *w, GdkEventButton *e, gpointer data){
    int x=(e->x-CELL_SIZE/2)/CELL_SIZE;
    int y=(e->y-CELL_SIZE/2)/CELL_SIZE;
    gboolean vs_ai = GPOINTER_TO_INT(data);
    if(x<0||x>=BOARD_SIZE||y<0||y>=BOARD_SIZE||board_data[x][y]!=EMPTY) return TRUE;

    // 일반 모드
    // ── Two-Player 모드 ──
    if (!vs_ai) {
        // 1) 착수
        board_data[x][y] = current_player;
        gtk_widget_queue_draw(w);

        // 2) 승패 판정
        if (check_winner(x, y, current_player)) {
            // 결과 다이얼로그
            ResultAction act = show_result_dialog(
                GTK_WINDOW(gtk_widget_get_toplevel(w)),
                TRUE  /* is_win: 사용자 승리 */
            );

            if (act == RESULT_RESTART) {
                // 보드 리셋
                memset(board_data, 0, sizeof(board_data));
                current_player = BLACK;
                gtk_widget_queue_draw(w);

            } else if (act == RESULT_MODE_SELECT) {
                // 모드 선택으로 돌아가기
                gtk_widget_destroy(gtk_widget_get_toplevel(w));
                show_mode_selection(global_app);

            } else {
                // 종료
                g_application_quit(G_APPLICATION(global_app));
            }
        } else {
            // 3) 턴 교체
            current_player = 3 - current_player;
        }
        return TRUE;
    }
    // vs AI 모드: 사용자 착수
    board_data[x][y]=BLACK;
    gtk_widget_queue_draw(w);
    if(check_winner(x,y,BLACK)){
        send_request("result", current_user, "win");
        ResultAction act = show_result_dialog(
            GTK_WINDOW(gtk_widget_get_toplevel(w)), TRUE);
        if(act==RESULT_RESTART){
            memset(board_data,0,sizeof(board_data));
            current_player=BLACK;
            gtk_widget_queue_draw(w);
        }else if(act==RESULT_MODE_SELECT){
            gtk_widget_destroy(gtk_widget_get_toplevel(w));
            show_mode_selection(global_app);
        }else{
            g_application_quit(G_APPLICATION(global_app));
        }
        return TRUE;
    }

    // AI 한 수
    MoveResult ai = minimax_search_ab(
        board_data, AI_DEPTH, 1, -DBL_MAX, DBL_MAX);
    if(ai.x>=0&&ai.y>=0){
        board_data[ai.x][ai.y]=WHITE;
        gtk_widget_queue_draw(w);
        if(check_winner(ai.x,ai.y,WHITE)){
            send_request("result", current_user, "lose");
            ResultAction act = show_result_dialog(
                GTK_WINDOW(gtk_widget_get_toplevel(w)), FALSE);
            if(act==RESULT_RESTART){
                memset(board_data,0,sizeof(board_data));
                current_player=BLACK;
                gtk_widget_queue_draw(w);
            }else if(act==RESULT_MODE_SELECT){
                gtk_widget_destroy(gtk_widget_get_toplevel(w));
                show_mode_selection(global_app);
            }else{
                g_application_quit(G_APPLICATION(global_app));
            }
            return TRUE;
        }
    }

    return TRUE;
}

/* ─── 로그인/회원가입 창 ─── */
static void activate(GtkApplication *app, gpointer _){
    global_app=app;
    GtkWidget *win=gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win),"로그인 / 회원가입");
    gtk_window_set_default_size(GTK_WINDOW(win),300,200);

    GtkWidget *grid=gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(win),grid);

    GtkWidget *lbl_u=gtk_label_new("아이디:");
    GtkWidget *lbl_p=gtk_label_new("비밀번호:");
    entry_username = gtk_entry_new();
    entry_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry_password),FALSE);
    GtkWidget *btn_login=gtk_button_new_with_label("로그인");
    GtkWidget *btn_reg  =gtk_button_new_with_label("회원가입");
    label_message     =gtk_label_new("");

    gtk_grid_attach(GTK_GRID(grid), lbl_u, 0,0,1,1);
    gtk_grid_attach(GTK_GRID(grid), entry_username, 1,0,2,1);
    gtk_grid_attach(GTK_GRID(grid), lbl_p, 0,1,1,1);
    gtk_grid_attach(GTK_GRID(grid), entry_password, 1,1,2,1);
    gtk_grid_attach(GTK_GRID(grid), btn_login, 1,2,1,1);
    gtk_grid_attach(GTK_GRID(grid), btn_reg,   2,2,1,1);
    gtk_grid_attach(GTK_GRID(grid), label_message, 0,3,3,1);

    g_signal_connect(btn_login, "clicked", G_CALLBACK(on_login_clicked), NULL);
    g_signal_connect(btn_reg,   "clicked", G_CALLBACK(on_register_clicked), NULL);

    gtk_widget_show_all(win);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new(
        "com.example.omok", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app,"activate",G_CALLBACK(activate),NULL);
    int status = g_application_run(G_APPLICATION(app),argc,argv);
    g_object_unref(app);
    return status;
}
