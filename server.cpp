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
#pragma comment(lib, "w2s_32.lib")
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

}