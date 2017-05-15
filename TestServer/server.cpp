/*
Программа железяка: архитектуры нет, ООП нет, обработки исключений нет; населена багами.
*/

#pragma comment (lib, "Ws2_32.lib")

#include <thread>
#include <WinSock2.h>
#include <iostream>
#include <sstream> // stringstream
#include <WS2tcpip.h> // inet_pton

using std::cout;
using std::endl;
using std::thread;

void accept_new_client(SOCKET newConnect);
int get_reg_key(HKEY regHKEY, wchar_t *reg_tree, wchar_t *key, char **buff);
bool auth_client(SOCKET *clientsock);

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

	// Структура определяющая на какой адрес и порт будем вешать сокет.
	SOCKADDR_IN saddr;
	int saddrlen = sizeof(saddr);
	inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);
	saddr.sin_port = htons(2000);
	saddr.sin_family = AF_INET;

	// Создаем слушающий сокет.
	SOCKET sListen;
	// MSDN: Если всё норм, socket() вернет дескриптор сокета, если ошибка, вернется INVALID_SOCKET. Последнюю ошибку получить функцией WSAGetLastError.
	if (INVALID_SOCKET == (sListen = socket(AF_INET, SOCK_STREAM, NULL))) {
		int error = WSAGetLastError();
		cout << "Error create socket." << endl;
		cout << "Error: " << error << endl;
		return error;
	}

	// Bind-им слушающий сокет на адрес и порт заданные ранее в(SOCKADDR_IN)saddr.
	// MSDN: If no error occurs, bind returns zero. Otherwise, it returns SOCKET_ERROR, and a specific error code can be retrieved by calling WSAGetLastError.
	if (SOCKET_ERROR == (bind(sListen, (SOCKADDR*)&saddr, sizeof(saddr)))) {
		int error = WSAGetLastError();
		cout << "Bind socket error." << endl;
		cout << "Error: " << error << endl;
		return error;
	}

	// Начинаем слушать.
	// MSDN: If no error occurs, listen returns zero. Otherwise, a value of SOCKET_ERROR is returned, and a specific error code can be retrieved by calling WSAGetLastError.
	if (SOCKET_ERROR == (listen(sListen, SOMAXCONN))) {
		int error = WSAGetLastError();
		cout << "Listen socket error." << endl;
		cout << "Error: " << error << endl;
		return error;
	}

	// Создаем рабочий сокет, который будет брать клиентов у слушающего сокета. 
	// Надеюсь я правильно понял.
	SOCKET newConnect;
	while (true) { // Ждем появления клиентов.
				   // MSDN: If no error occurs, accept returns a value of type SOCKET that is a descriptor for the new socket. This returned value is a handle for the socket on which the actual connection is made.
				   // Otherwise, a value of INVALID_SOCKET is returned, and a specific error code can be retrieved by calling WSAGetLastError.
		if (INVALID_SOCKET == (newConnect = accept(sListen, (SOCKADDR*)&saddr, &saddrlen))) {
			int error = WSAGetLastError();
			cout << "Accept socket error." << endl;
			cout << "Error: " << error << endl;
			return error;
		}
		else {
			cout << "Client connected: " << newConnect << endl;
			// Костыль. ;) Когда соединились с клиентом, создаем поток обрабатывающий его, и отсоединяем его от главного процесса (detach).
			thread thr1(accept_new_client, newConnect);
			//thr1.join();
			thr1.detach();
		}
	}

}

void accept_new_client(SOCKET newConnect) {
	if (auth_client(&newConnect)) {
		while (true) // Работаем с подсоединившимся клиентом.
		{
			char ping_pong_buff[1] = { '0' }; // Буффер для ping - pong поддержания соединения. 
			int message_len;
			message_len = recv(newConnect, ping_pong_buff, sizeof(ping_pong_buff), 0);
			//cout << "Message_len = " << message_len << endl;
			if (message_len > 0) // Если приняли что-то больше 0 значит клиент еще с нами. Если 0 соединение закрыто корректно. Если -1 соединение потеряно/разорвано некорректно.

				if (ping_pong_buff[0] == '0') {
					cout << "Ping <-- " << newConnect << endl;
					send(newConnect, ping_pong_buff, sizeof(ping_pong_buff), 0);
					cout << "Pong --> " << newConnect << endl;
					Sleep(500);
				}
				else {
					if (ping_pong_buff[0] == '1') {
						cout << "Action! " << newConnect << endl;
					}
				}

			else { // Если ничего не прилетело значит клиента потеряли и закрываем сокет. :'(
				cout << "Client: " << newConnect << " was lost :( " << endl;
				closesocket(newConnect);
				break;
			}
		}
	}
	else {
		cout << "Client: " << newConnect << ". Authorization failed." << endl;
	}
}

/*
Функция авторизации клиента. Если авторизовался возвращает TRUE если нет FALSE.
*/
bool auth_client(SOCKET *clientsock) {

	bool status = FALSE;
	char *login, *password;
	if (-1 == get_reg_key(HKEY_CURRENT_USER, L"SOFTWARE\\Crutches", L"login", &login))
		return FALSE;
	cout << login << endl; // DEBUG.
	if (-1 == get_reg_key(HKEY_CURRENT_USER, L"SOFTWARE\\Crutches", L"password", &password)) {
		free(login);
		return FALSE;
	}
	cout << password << endl; // DEBUG.
							  // Блицкриииг! 3 часа до рассвета...
	char *client_login = (char*)calloc(1, sizeof(char)); // Выделяем один символ под логин.
	char *client_passwd = (char*)calloc(1, sizeof(char)); // Выделяем один символ под пароль.
	for (int i = 0; i < 3; i++) { // Количество попыток.

		int recv_len = 0; // Длинна принятого recv()-ом сообщения.
		int final_len = 0; // Длинна уже считанного recv()-ом в буфер т.е. уже лежащего в буфере.  
		do {
			if (SOCKET_ERROR == (recv_len = recv(*clientsock, client_login + final_len, 1, 0))) // Считываем данные в 
				break;
			final_len += recv_len;
			client_login = (char*)realloc(client_login, final_len + 1);
			if (*(client_login + (final_len - 1)) == '\0')
				break;
			if (final_len > 64)
				break;
		} while (recv_len != 0);
		*(client_login + final_len) = '\0';

		recv_len = 0;
		final_len = 0;
		do {
			if (SOCKET_ERROR == (recv_len = recv(*clientsock, client_passwd + final_len, 1, 0)))
				break;
			final_len += recv_len;
			client_passwd = (char*)realloc(client_passwd, final_len + 1);
			if (*(client_passwd + (final_len - 1)) == '\0')
				break;
			if (final_len > 64)
				break;
		} while (recv_len != 0);
		*(client_passwd + final_len) = '\0';

		if (!strcmp(login, client_login))
			if (!strcmp(password, client_passwd)) {
				status = TRUE;
				break;
			}

	}

	free(client_login);
	free(client_passwd);
	free(login);
	free(password);
	return status;
}

/*
Функция достает значение указанного параметра из реестра.
Возвращает размер значения. 4й параметр может быть NULL если нужна только длинна.
При ошибке возвращает -1.
*/
int get_reg_key(HKEY regHKEY, wchar_t *reg_tree, wchar_t *key, char **buff) {
	//int get_reg_key(HKEY regHKEY, char *reg_tree, char *key, char **buff) { // Работает.

	/*
	А тут трэш, угар и содомия!
	В начале открываем раздел, если открылся без ошибок, то
	запрашиваем RegQueryValueEx() длинну значения параметра "login", если ошибок нет то,
	проверяем что значение не пустое (lenbuf > 1), если True то,
	выделяем место под буфер для записи результата (valbuf = (PBYTE)calloc(lenbuf, sizeof(BYTE)+1)) если True то,
	еще раз выполняем запрос RegQueryValueEx() только теперь еще передаем функции указатель на буфер в котроый нужно записать результат.
	RegCloseKey() - закрывает раздел по переданному хэндлу.
	*/

	/*
	// Конвертируем char* в wchar_t* чтобы RegOpenKeyEx() её принял. Работает.
	int regtree_len = strlen(reg_tree);
	wchar_t *wregtree = (wchar_t*)calloc(regtree_len++, sizeof(wchar_t));
	mbstowcs_s(NULL, wregtree, regtree_len, reg_tree, regtree_len);
	*/

	LSTATUS cresult;
	DWORD lenbuf;
	//PBYTE valbuf;
	HKEY my_hkey_handle;

	if (ERROR_SUCCESS != (cresult = RegOpenKeyEx(regHKEY, reg_tree, 0, KEY_ALL_ACCESS, &my_hkey_handle))) {
		//if (ERROR_SUCCESS != (cresult = RegOpenKeyEx(regHKEY, wregtree, 0, KEY_ALL_ACCESS, &my_hkey_handle))) { // Работает.
		cout << "Error Open RegKey. :(" << endl;
		cout << "Error: " << cresult << endl;
		return -1;
	}
	else if (ERROR_SUCCESS != (cresult = RegQueryValueEx(my_hkey_handle, key, NULL, NULL, NULL, &lenbuf))) {
		cout << "Error Query RegKey. :(" << endl;
		cout << "Error: " << cresult << endl;
		RegCloseKey(my_hkey_handle);
		return -1;
	}
	else if (buff != NULL) {
		cout << "lenbuf: " << lenbuf << endl;
		if (NULL != (*buff = (char*)calloc(lenbuf, sizeof(BYTE) + 1))) {
			if (ERROR_SUCCESS != (cresult = RegQueryValueEx(my_hkey_handle, key, NULL, NULL, (LPBYTE)*buff, &lenbuf))) {
				cout << "Error 2_Query RegKey. :(" << endl;
				cout << "Error: " << cresult << endl;
				RegCloseKey(my_hkey_handle);
				return -1;
			}
			else {
				cout << "Success!\n" << "Key Value : " << *buff << endl;
				/*for (int i = 0; i < lenbuf; i++) // Если использовать строковый параметр то получаем через каждый символ - '\0' .
				printf("%c", *(valbuf + i));*/
				RegCloseKey(my_hkey_handle);
				return lenbuf;
			}
		}
		else {
			cout << "Calloc is error!" << endl;
			RegCloseKey(my_hkey_handle);
			return -1;
		}
	}
	else {
		cout << "Only len." << endl;
		RegCloseKey(my_hkey_handle);
		return lenbuf;
	}

}
