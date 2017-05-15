#pragma comment (lib, "Ws2_32.lib")

#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>
#include <sstream>

using namespace std;

void new_connect_to_server(SOCKET my_socket);

int main() {

	// Это для задействования Dll библиотеки винды отвечающей за сокеты.
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);
	if (WSAStartup(DllVersion, &wsaData)) {
		int error = WSAGetLastError();
		cout << "Error init winsock." << endl;
		cout << "Error: " << error << endl;
		return error;
	}

	// Структура определяющая на какой адрес и порт будем стучаться.
	SOCKADDR_IN saddr;
	int saddrlen = sizeof(saddr);
	inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);
	saddr.sin_port = htons(2000);
	saddr.sin_family = AF_INET;

	// Создаем сокет.
	SOCKET msock;
	// MSDN: Если всё норм, socket() вернет дескриптор сокета, если ошибка, вернется INVALID_SOCKET. Последнюю ошибку получить функцией WSAGetLastError.
	if (INVALID_SOCKET == (msock = socket(AF_INET, SOCK_STREAM, NULL))) {
		int error = WSAGetLastError();
		cout << "Error create socket." << endl;
		cout << "Error: " << error << endl;
		return error;
	}

	// Устанавливаем соединение.
	// MSDN: If no error occurs, connect returns zero.Otherwise, it returns SOCKET_ERROR, and a specific error code can be retrieved by calling WSAGetLastError.
	// On a blocking socket, the return value indicates success or failure of the connection attempt.
	if (SOCKET_ERROR == (connect(msock, (SOCKADDR*)&saddr, saddrlen))) {
		int error = WSAGetLastError();
		cout << "Connection error." << endl;
		cout << "Error: " << error << endl;
		return error;
	}
	else {
		cout << "Connected!" << endl;
		new_connect_to_server(msock);
		//while (true) {
		//	char ping_pong_buff[1] = { '0' };
		//	int message_len;
		//	send(msock, ping_pong_buff, sizeof(ping_pong_buff), 0);
		//	cout << "Ping -->" << endl;
		//	message_len = recv(msock, ping_pong_buff, sizeof(ping_pong_buff), 0);
		//	if (message_len > 0) { // Если приняли что-то больше 0 значит сервер еще с нами. Если 0 соединение закрыто корректно. Если -1 соединение потеряно/разорвано некорректно.
		//			
		//		if (ping_pong_buff[0] == '0') {
		//			cout << "Pong <--" << endl;
		//			//Sleep(1000);
		//		}
		//		else {
		//			if (ping_pong_buff[0] == '1') {
		//				cout << "Action!" << endl;
		//			}
		//		}
		//	}
		//	else {
		//		cout << "Server not requested :( " << endl;
		//		closesocket(msock);
		//		break;
		//	}
		//}
	}

	system("pause");

}


void new_connect_to_server(SOCKET msock) {


	char *login = "dimasik_l";
	send(msock, login, strlen(login) + 1, 0);
	char *mpasswd = "ololo911";
	send(msock, mpasswd, strlen(mpasswd) + 1, 0);

	while (true) {
		char ping_pong_buff[1] = { '0' };
		int message_len;
		send(msock, ping_pong_buff, sizeof(ping_pong_buff), 0);
		cout << "Ping -->" << endl;
		message_len = recv(msock, ping_pong_buff, sizeof(ping_pong_buff), 0);
		if (message_len > 0) { // Если приняли что-то больше 0 значит сервер еще с нами. Если 0 соединение закрыто корректно. Если -1 соединение потеряно/разорвано некорректно.

			if (ping_pong_buff[0] == '0') {
				cout << "Pong <--" << endl;
				//Sleep(1000);
			}
			else {
				if (ping_pong_buff[0] == '1') {
					cout << "Action!" << endl;
				}
			}
		}
		else {
			cout << "Server not requested :( " << endl;
			closesocket(msock);
			break;
		}
	}
}

