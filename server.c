// server.c
// gcc server.c -o server.out -pthread
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_ID_LEN    32
#define MAX_PW_LEN    32
#define BUFFER_SIZE  1024
#define SERVER_PORT  8000    // 클라이언트와 동일한 포트 사용

// 사용자 정보 구조체
typedef struct User {
    char id[MAX_ID_LEN];
    char pw[MAX_PW_LEN];
    int win;
    int lose;
    double win_rate;
    struct User* next;
} User;

static User* user_list = NULL;
static pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

// 파일에서 사용자 목록 로드
void load_users() {
    FILE* fp = fopen("userdb.txt", "r");
    if (!fp) return;
    char id[MAX_ID_LEN], pw[MAX_PW_LEN];
    int win, lose;
    double win_rate;
    while (fscanf(fp, "%31s %31s %d %d %lf",
                  id, pw, &win, &lose, &win_rate) == 5) {
        User* u = malloc(sizeof(User));
        strncpy(u->id, id, MAX_ID_LEN);
        strncpy(u->pw, pw, MAX_PW_LEN);
        u->win = win;
        u->lose = lose;
        u->win_rate = win_rate;
        u->next = user_list;
        user_list = u;
    }
    fclose(fp);
}

// 메모리상의 사용자 목록을 파일에 저장
void save_users() {
    FILE* fp = fopen("userdb.txt", "w");
    if (!fp) return;
    for (User* u = user_list; u; u = u->next) {
        fprintf(fp, "%s %s %d %d %.2f\n",
                u->id, u->pw, u->win, u->lose, u->win_rate);
    }
    fclose(fp);
}

// 클라이언트 한 명을 처리하는 스레드 함수
void* handle_client(void* arg) {
    int client_sock = (intptr_t)arg;
    char buf[BUFFER_SIZE];
    int received = recv(client_sock, buf, sizeof(buf)-1, 0);
    if (received <= 0) {
        close(client_sock);
        return NULL;
    }
    buf[received] = '\0';

    // "op id pw" 파싱
    char* op = strtok(buf, " ");
    char* id = strtok(NULL, " ");
    char* pw = strtok(NULL, " ");
    char msg[BUFFER_SIZE];

    pthread_mutex_lock(&db_mutex);
    if (strcmp(op, "register") == 0) {
        // 회원가입: ID 중복 확인
        User* cur = user_list;
        int exists = 0;
        while (cur) {
            if (strcmp(cur->id, id) == 0) { exists = 1; break; }
            cur = cur->next;
        }
        if (exists) {
            strcpy(msg, "exists");        // 이미 존재
        } else {
            // 새 사용자 생성
            User* u = malloc(sizeof(User));
            strncpy(u->id, id, MAX_ID_LEN);
            strncpy(u->pw, pw, MAX_PW_LEN);
            u->win = 0; u->lose = 0; u->win_rate = 0.0;
            u->next = user_list;
            user_list = u;
            save_users();                  // 변경사항 파일에 저장
            strcpy(msg, "success");       // 성공
        }
    }
    else if (strcmp(op, "login") == 0) {
        // 로그인: ID 존재 및 비밀번호 확인
        User* cur = user_list;
        int found = 0;
        while (cur) {
            if (strcmp(cur->id, id) == 0) { found = 1; break; }
            cur = cur->next;
        }
        if (!found) {
            strcpy(msg, "cannot_find_id");   // ID 없음
        }
        else if (strcmp(pw, cur->pw) != 0) {
            strcpy(msg, "not_match_pw");     // 비밀번호 불일치
        }
        else {
            strcpy(msg, "success");          // 로그인 성공
        }
    }
    else {
        strcpy(msg, "invalid_op");           // 지원되지 않는 명령
    }
    pthread_mutex_unlock(&db_mutex);

    // 클라이언트로 결과 전송
    send(client_sock, msg, strlen(msg), 0);
    close(client_sock);
    return NULL;
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;

    // 사용자 DB 로드
    load_users();

    // 서버 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket failed");
        return 1;
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;      // 모든 인터페이스
    server_addr.sin_port        = htons(SERVER_PORT);

    // 바인드
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_sock);
        return 1;
    }

    // 리스닝
    if (listen(server_sock, SOMAXCONN) < 0) {
        perror("listen failed");
        close(server_sock);
        return 1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    // 클라이언트 연결 반복 수락
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock,
                                 (struct sockaddr*)&client_addr,
                                 &client_len);
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }
        // 처리 스레드 생성
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void*)(intptr_t)client_sock);
        pthread_detach(tid);
    }

    close(server_sock);
    return 0;
}
