#include <iostream>
#include <string>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
//조건부 컴파일, window에서 코드 작성 위함
#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#endif

using namespace std;

//서버 주소와 포트를 바꾸고 싶다면 인자로 전달 가능
string send_request_to_server(const string& op, const string& id, const string& pw, const string& host, int port = 12345) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return "error: WSAStartup failed";
    } 
#endif

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        return "error: 소켓 생성 실패";
    }

    // 클라이언트에서 서버와 연결하기 위한 단계
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());

    //서버와 연결
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        closesocket(sock);
        return "error: 연결 실패";
    }

    //연결 후 동작 아이디 비번 보냄
    string msg = op + " " + id + " " + pw;
    send(sock, msg.c_str(), msg.size(), 0);


    //서버에서 응답 받음, 
    /*
    * id_exists 실패
    * cannot_find_id 실패
    * not_match_pw 실패
    * success 성공
    */
    char buf[1024];
    int received = recv(sock, buf, sizeof(buf) - 1, 0);
    if (received <= 0) {
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return "error: 응답 없음";
    }

    buf[received] = '\0';
    string response(buf);

    closesocket(sock);

#ifdef _WIN32
    WSACleanup();
#endif
    return response;
}
