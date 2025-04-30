#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

GtkWidget *entry_username;
GtkWidget *entry_password;
GtkWidget *label_message;

// 회원가입
void on_register_clicked(GtkButton *button, gpointer user_data) {
    const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(entry_password));

    // 이미 존재하는 사용자 검사
    FILE *file = fopen("users.txt", "r");
    if (file) {
        char file_user[32], file_pass[32];
        while (fscanf(file, "%s %s", file_user, file_pass) != EOF) {
            if (strcmp(file_user, username) == 0) {
                gtk_label_set_text(GTK_LABEL(label_message), "이미 존재하는 아이디입니다.");
                fclose(file);
                return;
            }
        }
        fclose(file);
    }

    // 새 사용자 추가
    file = fopen("users.txt", "a");
    if (!file) {
        gtk_label_set_text(GTK_LABEL(label_message), "파일 쓰기 실패");
        return;
    }
    fprintf(file, "%s %s\n", username, password);
    fclose(file);

    gtk_label_set_text(GTK_LABEL(label_message), "회원가입 성공!");
}

// 로그인
void on_login_clicked(GtkButton *button, gpointer user_data) {
    const char *username = gtk_entry_get_text(GTK_ENTRY(entry_username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(entry_password));

    FILE *file = fopen("users.txt", "r");
    if (!file) {
        gtk_label_set_text(GTK_LABEL(label_message), "사용자 데이터 없음");
        return;
    }

    char file_user[32], file_pass[32];
    while (fscanf(file, "%s %s", file_user, file_pass) != EOF) {
        if (strcmp(file_user, username) == 0 && strcmp(file_pass, password) == 0) {
            gtk_label_set_text(GTK_LABEL(label_message), "로그인 성공! 게임 시작 준비...");
            fclose(file);
            // TODO: 여기서 오목 게임 창으로 넘어가기
            return;
        }
    }
    fclose(file);
    gtk_label_set_text(GTK_LABEL(label_message), "로그인 실패: 아이디 또는 비밀번호 틀림");
}

// GTK 창 생성
void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *label_username;
    GtkWidget *label_password;
    GtkWidget *button_login;
    GtkWidget *button_register;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "로그인 / 회원가입");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    label_username = gtk_label_new("아이디:");
    label_password = gtk_label_new("비밀번호:");
    entry_username = gtk_entry_new();
    entry_password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry_password), FALSE); // 비밀번호 가리기
    button_login = gtk_button_new_with_label("로그인");
    button_register = gtk_button_new_with_label("회원가입");
    label_message = gtk_label_new("");

    gtk_grid_attach(GTK_GRID(grid), label_username, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_username, 1, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), label_password, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_password, 1, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), button_login, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_register, 2, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_message, 0, 3, 3, 1);

    g_signal_connect(button_login, "clicked", G_CALLBACK(on_login_clicked), NULL);
    g_signal_connect(button_register, "clicked", G_CALLBACK(on_register_clicked), NULL);

    gtk_widget_show_all(window);
}

// 메인 함수
int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.login", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    return status;
}
