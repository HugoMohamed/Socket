/********************
*                   *
*	Hugo MOHAMED	*
*					*
********************/

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"


int main(void)
{
	WSADATA wsaData;  //  WSA stands for "Windows Socket API"
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult = 0;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		cout << "WSAStartup failed with error: " << iResult << endl;
		return -1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;  // Address Format Internet
	hints.ai_socktype = SOCK_STREAM;  //  Provides sequenced, reliable, two-way, connection-based byte streams
	hints.ai_protocol = IPPROTO_TCP;  // Indicates that we will use TCP
	hints.ai_flags = AI_PASSIVE;  // Indicates that the socket will be passive => suitable for bind()

	// Resolve the server address and port
	//       => defines port number and gets local IP address (in "result" struc)
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo failed with error: " << iResult << endl;
		WSACleanup();
		return -1;
	}

	// Create a SOCKET for connecting to server
	//     (note that we could have defined the 3 socket parameter using "hints"...)
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return -1;
	}

	// Setup the TCP listening socket
	//     (for "bind()", we need the info returned by "getaddrinfo()")
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	// Note:  "ai_addr" is a struct containing IP address and port
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}

	freeaddrinfo(result);

	// define the maximum number of connections
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}

	// Accept a client socket
	//   Notes:
	//          1- "accept()" is a blocking function
	//          2- It returns a new socket of the connection
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		cout << "accept failed with error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return -1;
	}

	// No longer need server socket
	closesocket(ListenSocket);

	bool done;
	int i, cumul;

	// Receive until the peer shuts down the connection using the shutdown() function.
	do {
		// IMPORTANT: The client will send a C string terminated by '\0' (not a C++ string)
		//            We will therefore loop until we receive '\0'
		done = false;
		cumul = 0;
		while (!done) {
			iResult = recv(ClientSocket,
				&recvbuf[cumul],
				recvbuflen,  // not the number of bytes to be received !!!
				0);
			if (iResult != 0) { // if connection has not been shutdown (some bytes received)
				for (i = cumul; i < cumul + iResult; i++) {
					if (recvbuf[i] == '\0')
						done = true;
				}
				cumul += iResult; // increment the byte counter
			}
			else {  // if connection has been shutdown by the other party (the client), quit the loop 
				done = true;
			}
		}

		if (cumul > 0)
		{
			cout << "Bytes received: " << cumul << endl;
			ifstream image(recvbuf, ifstream::binary);
			int i = 0;
			int length;
			char * sendbuf;
			
			// length of image
			image.seekg(0, image.end);
			length = image.tellg();
			image.seekg(0, image.beg);

			sendbuf = new char[length];
			char c;
			for (int i = 0; i < length; i++)
			{
				image.get(c);
				sendbuf[i] = c;
			}
			// Echo the content of "recvbuf" back to the sender
			// Therefore, we have to send "cumul" bytes 
			//iSendResult = send(ClientSocket, sendbuf, length+1, 0);

			int x = 0;
			while(x < length)
			{
				char tmpSendBuf[DEFAULT_BUFLEN];
				// Subtracting the 512 first char of sendbuf in tmpSendBuf
				for (int j = 0; j < DEFAULT_BUFLEN; j++)
				{
					tmpSendBuf[j] = sendbuf[x + j];
				}
				/*for (int n = 0; n < length - DEFAULT_BUFLEN +1; n++)
				{
					sendbuf[i] = sendbuf[DEFAULT_BUFLEN + i];
				}*/
				iSendResult = send(ClientSocket, tmpSendBuf, DEFAULT_BUFLEN, 0);
				cout << "Bytes sent: " << iSendResult << endl;

				x += iSendResult;
			}

			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed with error: " << WSAGetLastError() << endl;
				closesocket(ClientSocket);
				WSACleanup();
				return -1;
			}
			cout << "Bytes sent: " << x << endl;
		}
		
		else if (cumul == 0)
			cout << "Connection closing..." << endl;
		else {
			cout << "recv failed with error: " << WSAGetLastError() << endl;
			closesocket(ClientSocket);
			WSACleanup();
			return -1;
		}

	} while (cumul > 0);

	// Shutdown the connection since we're done.
	// This will cause the other party (the client) to exit any recv() function waiting for incoming data.
	// The recv() in the client program will then return the value 0.
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed with error: " << WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}