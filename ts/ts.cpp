#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <arpa/inet.h>
#include <sys/socket.h>
#endif // __linux
#include <iostream>
#include <thread>
#include <set>
#include <mutex>

using namespace std;

//
void usage() {
	cout << "echo-server:\n";
	cout << "syntax : echo-server <port> [-e[-b]]\n";
	cout << "sample : echo-server 1234 -e -b\n";
}
//

struct Param {
	bool echo{false};
	bool broadcast{false}; //
	uint16_t port{0};

	bool parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-e") == 0) {
				echo = true;
				//cout << "-e" << endl;
				continue;
			}
			//
			if (strcmp(argv[i], "-b") == 0) {
				broadcast = true;
				//cout << "-b" << endl;
				continue;
			}
			//
			port = stoi(argv[i]);
			//cout << port << endl;
		}
		return port != 0;
	}
} param;

//
set <int> sd_list;
mutex set_mutex;
//

void recvThread(int sd) {
	cout << "connected\n";
	static const int BUFSIZE = 65536;
	char buf[BUFSIZE];
	while (1) {
		ssize_t res = recv(sd, buf, BUFSIZE - 1, 0);
		if (res == 0 || res == -1) {
			cerr << "recv return " << res;
			perror(" ");
			break;
		}
		buf[res] = '\0';
		cout << buf;
		cout.flush();


		if (param.broadcast) {
			//
			set_mutex.lock();
			for (auto i : sd_list) {
				long res_ = send(i, buf, res, 0);
				if (res_ == 0 || res_ == -1) {
					cerr << "send return " << res;
					perror(" ");
					sd_list.erase(i);
				}
			}
			set_mutex.unlock();
			//
		}

		else if (param.echo) {
			res = send(sd, buf, res, 0);
			if (res == 0 || res == -1) {
				cerr << "send return " << res;
				perror(" ");
				break;
			}
		}
		
	}
	//
	set_mutex.lock();
	sd_list.erase(sd);
	set_mutex.unlock();
	//
	cout << "disconnected\n";
	close(sd);
}

int main(int argc, char* argv[]) {
	if (!param.parse(argc, argv)) {
		usage();
		return -1;
	}


	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket");
		return -1;
	}

	int res;
#ifdef __linux__
	int optval = 1;
	res = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (res == -1) {
		perror("setsockopt");
		return -1;
	}
#endif // __linux

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(param.port);

	ssize_t res2 = ::bind(sd, (struct sockaddr *)&addr, sizeof(addr));
	if (res2 == -1) {
		perror("bind");
		return -1;
	}

	res = listen(sd, 5); // 큐에 5개까지 저장 가능
	if (res == -1) {
		perror("listen");
		return -1;
	}

	while (true) {
		struct sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_sd = accept(sd, (struct sockaddr *)&cli_addr, &len);
		if (cli_sd == -1) {
			perror("accept");
			break;
		}
		//
		set_mutex.lock();
		sd_list.insert(cli_sd);
		set_mutex.unlock();
		//
		thread* t = new thread(recvThread, cli_sd);
		t->detach();
	}
	close(sd);
}
