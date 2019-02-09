/********************
*                   *
*	Hugo MOHAMED	*
*					*
********************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char *sendbuf;

	char recvbuf[10000];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	

	// Validate the parameters
	if (argc != 2) {
		cout << "usage: " << argv[0] <<  " image.png" << endl;
        return -1;
	}
	ofstream image("receivedImage.png", fstream::binary);
	sendbuf = argv[1];

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult << endl;
		return -1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;  // Address Format Unspecified or AF_INET (for IPv4)
	hints.ai_socktype = SOCK_STREAM;  // Provides sequenced, reliable, two-way, connection-based byte streams 
	hints.ai_protocol = IPPROTO_TCP;  // Indicates that we will use TCP

	// Resolve the server address and port
	//       => defines port number and gets remote IP address (in "result" struc)
	//iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult << endl;
		WSACleanup();
		return -1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			cout << "socket failed with error: " << WSAGetLastError() << endl;
			WSACleanup();
			return -1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		// Note:  "ai_addr" is a struct containing IP address and port
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		cout << "Unable to connect to server!" << endl;
		WSACleanup();
		return -1;
	}

	// Send the whole content of sendbuf (15 bytes including '\0')
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf) + 1, 0);
	if (iResult == SOCKET_ERROR) {
		cout << "send failed with error: " << WSAGetLastError() << endl;
		closesocket(ConnectSocket);
		WSACleanup();
		return -1;
	}

	cout << "Bytes Sent: " << iResult << endl;

	// Shutdown the connection since no more data will be sent.
	// This will cause the other party (the server) to exit any recv() function waiting for incoming data.
	// The recv() in the server program will then return the value 0.
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed with error: " << WSAGetLastError() << endl;
		closesocket(ConnectSocket);
		WSACleanup();
		return -1;
	}

	bool done;
	int i, cumul;

	// Receive until the peer shuts down the connection
	do {
		// IMPORTANT: The server will send a C string terminated by '\0' (not a C++ string) 
		//            We will therefore loop until we receive '\0'
		done = false;
		cumul = 0;
		while (!done) {
			iResult = recv(ConnectSocket,
				&recvbuf[cumul],
				recvbuflen,    // not the number of bytes to be received !!!
				0);
			if (iResult != 0) { // if connection has not been shutdown (some bytes received)
				for (i = cumul; i < cumul + iResult; i++) {
					if (recvbuf[i] == '\0')
						done = true;
				}
				cumul += iResult;  // increment the byte counter
			}
			else {  // if connection has been shutdown by the other party (the server), quit the loop 
				done = true;
			}
		}

		if (cumul > 0) {
			cout << "Bytes received: " << iResult << endl;
		}
		else if (cumul == 0)
			cout << "Connection closed" << endl;
		else
			cout << "recv failed with error: " << WSAGetLastError() << endl;

		image.write(recvbuf, DEFAULT_BUFLEN);
	} while (cumul > 0);

	// cleanup
	image.close();
	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}
