#include <iostream>
#include <map>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <sstream>

#ifdef _WIN32
#include<WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <arap/inet.h>
#include <unistd.h>
#define	SOCKET int
#define INVALID_SOCKET -1
#define closesoket close
#endif

using namespace std;

struct UserInfo {
	string pw;
	int win;
	int lose;
	double win_rate;
};

map<string, UserInfo> userdb; // id, userinfo
mutex db_mutex;

void load_users() {
	ifstream ifs("userdb.txt");
	string id, pw;
	int win, lose;
	double win_rate;

	while (ifs >> id >> pw >> win >> lose >> win_rate) {
		userdb[id] = { pw, win, lose, win_rate };
	}
}

void save_users(const string& id, const string& pw, int win, int lose, double win_rate) {
	ofstream ofs("userdb.txt");
	ofs << id << ' ' << pw << ' ' << win << ' ' << lose << ' ' << win_rate << '\n';
}

void handle_client(SOCKET client_socket) {
	char buf[1024];
	int received = recv(client_socket, buf, sizeof(buf) - 1, 0);
	if (received <= 0) {
		closesocket(client_socket);
		return;
	}

	buf[received] = '\0';
	stringstream ss(buf);
	string op;
	ss >> op;

	lock_guard<mutex> lock(db_mutex);
	if (op == "registration") {
		string id, pw;
		ss >> id >> pw;
		if (userdb.find(id) != userdb.end()) {
			string msg = "id_exists";
			send(client_socket, msg.c_str(), sizeof(msg), 0);
		}
		else {
			userdb[id] = { pw, 0, 0, 0 };	// 비번, 승 0, 패 0, 승률 0
		}
	}
	else if (op == "login") {	//로그인하려는 경우
		string id, pw, msg;
		ss >> id >> pw;
		if (userdb.find(id) == userdb.end()) {	//아이디가 존재하는지부터 확인, 없는 경우
			msg = "cannot_find_id";
			send(client_socket, msg.c_str(), sizeof(msg), 0);
		}
		else if (pw != userdb[id].pw) {	//아이디는 맞는데 비밀번호가 틀림
			msg = "not_match_pw";
			send(client_socket, msg.c_str(), sizeof(msg), 0);
		}
		else {
			msg = "success";
			send(client_socket, msg.c_str(), sizeof(msg), 0);
		}
	}
	closesocket(client_socket);
}

int main() {
#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
	load_users();

	SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == INVALID_SOCKET) {
		cerr << "소켓 생성 실패" << '\n';
		return 1;
	}


	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY; // 모든 IP에서 수신
	server_addr.sin_port = htons(1234); // 포트 설정

	if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		cerr << "Bind failed." << '\n';
		return 1;
	}

	if (listen(server_socket, SOMAXCONN) < 0) {
		cerr << "Listen failed." << endl;
		return 1;
	}

	cout << "Server is running and waiting for connections..." << endl;

	while (true) {
		sockaddr_in client_addr{};
		socklen_t client_size = sizeof(client_addr);

		SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_size);
		if (client_socket == INVALID_SOCKET) {
			cerr << "Accept failed." << endl;
			continue;
		}

		// 클라이언트 처리 스레드 생성
		thread client_thread(handle_client, client_socket);
		client_thread.detach(); // 독립 실행
	}

#ifdef _WIN32
	WSACleanup();
#else
	close(server_socket);
#endif

	return 0;
}