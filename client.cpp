#include <iostream>
#include <string>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
//���Ǻ� ������, window���� �ڵ� �ۼ� ����
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

//���� �ּҿ� ��Ʈ�� �ٲٰ� �ʹٸ� ���ڷ� ���� ����
string send_request_to_server(const string& op, const string& id, const string& pw, const string& host, int port = 12345) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return "error: WSAStartup failed";
    } 
#endif

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        return "error: ���� ���� ����";
    }

    // Ŭ���̾�Ʈ���� ������ �����ϱ� ���� �ܰ�
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());

    //������ ����
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        closesocket(sock);
        return "error: ���� ����";
    }

    //���� �� ���� ���̵� ��� ����
    string msg = op + " " + id + " " + pw;
    send(sock, msg.c_str(), msg.size(), 0);


    //�������� ���� ����, 
    /*
    * id_exists ����
    * cannot_find_id ����
    * not_match_pw ����
    * success ����
    */
    char buf[1024];
    int received = recv(sock, buf, sizeof(buf) - 1, 0);
    if (received <= 0) {
        closesocket(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return "error: ���� ����";
    }

    buf[received] = '\0';
    string response(buf);

    closesocket(sock);

#ifdef _WIN32
    WSACleanup();
#endif
    return response;
}
