#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"

#define SERVER_PORT 7799
#define MAXBUF 10250
#define MAXLEN 3000
#define STACK_SIZE 10000000
#define WM_SOCKET_NOTIFY (WM_USER + 1)
#define WM_SERVER_NOTIFY (WM_SOCKET_NOTIFY + 1);

typedef struct info {
	SOCKET connfd;
	FILE* file;
	char msg[MAXBUF];
	char* pos;
	int left;
	bool cansend;
} Info;
Info server_info[5];

typedef struct data {
	char domain_name[100];
	int port;
	char filename[100];
} Data;
Data server_data[5];

int hostname_to_ip(char* hostname, char* ip) {
	struct hostent *hst;
	struct in_addr **addr_list;
	memset(ip, 0 , 100);
	if((hst = gethostbyname(hostname)) == NULL) return 1;
	addr_list = (struct in_addr **) hst->h_addr_list;
	if(*addr_list == NULL) return 1;
	else strcpy(ip, inet_ntoa(*addr_list[0]));
	return 0;
}

void close_socket(Info& server) {
	closesocket(server.connfd);
	server.connfd = -1;
}

int server_num, total_conn;

int doCGI(HWND hwndEdit, HWND hwnd, SOCKET ssock) {
	
	char* parameter[15] = {0}, ip[100];
	
	int id = 0, para_num = 0;
	parameter[para_num++] = strtok(arg.QUERY_STRING, "&");
	while(parameter[para_num++] = strtok(NULL, "&"));

	for(int k = 0; k < para_num; k++) {
		strtok(parameter[k], "=");
		char* val = strtok(NULL, "&");
		if(val != NULL) {
		 	if(parameter[k][0] == 'h') strcpy(server_data[id].domain_name, val); 
		 	else if(parameter[k][0] == 'p') server_data[id].port = atoi(val); 
		 	else if(parameter[k][0] == 'f') sprintf(server_data[id++].filename, "%s" , val); 
	 	}
	}

	server_num = id,total_conn = 0;
	char output[MAXBUF] = {0};
	strcat(output, "Content-type: text/html\n\n");
	strcat(output, "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" /><title>Network Programming Homework 3</title></head>");
	strcat(output, "<body bgcolor=#336699><font face=\"Courier New\" size=2 color=#FFFF99>");
	strcat(output, "<table width=\"800\" border=\"1\">");
	strcat(output, "<tr>");
	send(ssock, output, strlen(output), 0);

	for(int id = 0; id < server_num; id++) {
		sprintf(output, "<td>%s</td>", server_data[id].domain_name);
		send(ssock, output, strlen(output), 0);
	}
	strcpy(output, "</tr><tr>");
	send(ssock, output, strlen(output), 0);
	for(int id = 0; id < server_num; id++) {
		cout << "<td valign=\"top\" id=\"m" << id << "\"></td>";
		sprintf(output, "<td valign=\"top\" id=\"m%d\"></td>", id);
		send(ssock, output, strlen(output), 0);
	}
	strcpy(output, "</tr></table>");
	send(ssock, output, strlen(output), 0);
			
	struct sockaddr_in client_sin;

	for(int id = 0; id < server_num; id++) {
		Info& server = server_info[id];
		WSADATA wsaData;
		int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (ret != NO_ERROR) {
			EditPrintf(hwndEdit, TEXT("WSAStartup function failed with error: %d\r\n"), r);
			return 1;
		}

		SOCKET csock;
		server.connfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (csock == INVALID_SOCKET) {
			EditPrintf(hwndEdit, TEXT("socket function failed with error: %ld\r\n"), WSAGetLastError());
			WSACleanup();
			return 1;
		}
 		
 		client_sin.sin_family = AF_INET;
 		if(hostname_to_ip(server_data[id].domain_name, ip)) close_socket(server);
 		else {
			inet_pton(AF_INET, ip, &client_sin.sin_addr);
			client_sin.sin_port = htons(server_data[id].port);

			if(connect(server.connfd, (struct sockaddr *) &client_sin, sizeof(client_sin)) < 0) {
				close_socket(server);
			}
			else {
				EditPrintf(hwndEdit, TEXT("connected to %s\r\n"), ip);
				total_conn++;
	    		server.file = fopen(server_data[id].filename , "r");
	    		server.cansend = 0;
				server.left = 0;
				int err = WSAAsyncSelect(csock, hwnd, WM_SERVER_NOTIFY, FD_CLOSE | FD_READ | FD_WRITE);
				if (err == SOCKET_ERROR) {
					EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
					closesocket(csock);
					WSACleanup();
					return 1;
				}
			}
		}
	}

	return 0;
 }

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf (HWND, TCHAR *, ...);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	
	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}

BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WSADATA wsaData;

	static HWND hwndEdit;
	static SOCKET msock, ssock;
	static struct sockaddr_in sa;

	int err;


	switch(Message) 
	{
		case WM_INITDIALOG:
			hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_LISTEN:

					WSAStartup(MAKEWORD(2, 0), &wsaData);

					//create master socket
					msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

					if( msock == INVALID_SOCKET ) {
						EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
						WSACleanup();
						return TRUE;
					}

					err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

					if ( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(msock);
						WSACleanup();
						return TRUE;
					}

					//fill the address info about server
					sa.sin_family		= AF_INET;
					sa.sin_port			= htons(SERVER_PORT);
					sa.sin_addr.s_addr	= INADDR_ANY;

					//bind socket
					err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
						WSACleanup();
						return FALSE;
					}

					err = listen(msock, 2);
		
					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\r\n"));
						WSACleanup();
						return FALSE;
					}
					else {
						EditPrintf(hwndEdit, TEXT("=== Server START ===\r\n"));
					}

					break;
				case ID_EXIT:
					EndDialog(hwnd, 0);
					break;
			};
			break;

		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;

		case WM_SOCKET_NOTIFY:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_ACCEPT:
					ssock = accept(msock, NULL, NULL);
					Socks.push_back(ssock);
					EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\r\n"), ssock, Socks.size());
					break;
				case FD_READ:
				//Write your code for read event here.
					char request[MAXBUF] = {0},content[MAXBUF] = {0};
    		
					int length = recv(ssock, request, MAXBUF, 0);
					EditPrintf(hwndEdit, TEXT("=== length: (%d) ===\r\n"), length);
					EditPrintf(hwndEdit, TEXT("=== buf: \r\n %s ===\r\n"), request);

					char* type = strtok(request, "/");
					if(strncmp("GET", type, 3) == 0) {
						char* url = strtok(NULL, " "),*protocol = strtok(NULL, "\r\n");
						if(protocol == NULL) protocol = url, url = NULL;
						char full_path[MAXLEN], name[MAXLEN], code[MAXLEN];
						strtok(url, ".");
						char* extension = strtok(NULL,"?");
						if(url == NULL) sprintf(full_path, "%s(null)",ROOT);
						else {
							if(extension == NULL) strcpy(name, url);
							else sprintf(name, "%s.%s", url , extension);
							sprintf(full_path, "%s%s", ROOT, name);
						}
						EditPrintf(hwndEdit, TEXT("=== full_path: (%s) ===\r\n"), full_path);
						EditPrintf(hwndEdit, TEXT("=== full_name: (%s) ===\r\n"), full_name);
						
						if(extension != NULL && strcmp("cgi", extension) == 0) {
							char QUERY_STRING[MAXLEN];
							struct stat sb;
							if(stat(full_path, &sb) == 0 && sb.st_mode & S_IXUSR) {
								char* parameter = strtok(NULL, " ");
								sprintf(code,"%s 200 OK\r\n", protocol);
								send(ssock, code, strlen(code), 0);

								if(parameter != NULL) strcpy(QUERY_STRING, "%s", parameter);
								else strcpy(QUERY_STRING, "h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5=");
								
								doCGI(hwndEdit, hwnd, ssock);
								
							}
							else {
								sprintf(code,"%s 404 Not Found\r\n\r\n", protocol);
		    					send(ssock, code, strlen(code), 0);
							}
						}
						else {
							FILE* fptr = fopen(full_path , "rb");
							if(fptr != NULL) {
								sprintf(code,"%s 200 OK\r\n",protocol);
		    					if(strncmp("htm", extension, 3) == 0 ) sprintf(content,"%sContent-Type: %s\r\n\r\n", code, "text/html");
		    					else sprintf(content,"%sContent-Type: %s\r\n\r\n", code, "text/plain");
		    					send(ssock, content, strlen(content), 0);
		    					EditPrintf(hwndEdit, TEXT("=== file: (%s) ===\r\n"), full_path);

		    					int fd = fileno(fptr); //if you have a stream (e.g. from fopen), not a file descriptor.
								struct stat info;
								fstat(fd, &info);
								int size = info.st_size;
		    					memset(content, 0 , MAXLEN);
		    					int fsize = fread(content, size, 1, fptr);
		    					if(fsize != size) OutputDebugString("fread failed"); 

		    					send(ssock, content, fsize, 0);
							}
							else {
								sprintf(code,"%s 404 Not Found\r\n\r\n", protocol);
		    					send(ssock, code, strlen(code), 0);
							}
							fclose(file_ptr);
							closesocket(ssock);
						}

					}
					break;
				case FD_WRITE:
				//Write your code for write event here
					if(total_conn == 0) {
						strcpy(output, "</font></body></html>");
						send(ssock, output, strlen(output), 0);
						close_socket(ssock);
					}
					break;
				case FD_CLOSE:
					break;
			};
			break;
		case WM_SERVER_NOTIFY:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_READ:
					char msg[MAXBUF] = {0};
					SOCKET csock = wParam;
					int nread = recv(csock, msg, MAXBUF, 0), id;
					for(id = 0; id < server_num; id++) if(csock == server_info[id].connfd) break;
					Info& server = server_info[id];
		 			if(nread > 0) {
		 				EditPrintf(hwndEdit, TEXT("=== [server] recv: START ===\r\n%s\r\n== recv: END ==\r\n"), msg);
		 				sprintf(output, "<script>document.all['m%d'].innerHTML += \"", id);
		 				
		 				for(int pos = 0,len = strlen(msg); pos != len; pos++) {
		 					if(msg[pos] == '\n') strcat(output, "<br>");
		 					else if(msg[pos] == '<') strcat(output, "&lt;");
		 					else if(msg[pos] == '>') strcat(output, "&gt;");
		 					else if(msg[pos] == '"') strcat(output, "&quot;");
		 					else if(msg[pos] == ' ') strcat(output, "&nbsp;");
		 					else if(msg[pos] == '\r') ;
		 					else {
		 						if(msg[pos] == '%') server.cansend = 1;
		 						strcat(output, msg[pos]);
		 					}
		 				}
		 				strcat(output, "\";</script>");
		 				send(ssock, output, strlen(output), 0);
		 			}
		 			else {
		 				close_socket(server);
		 			}
					break;
				case FD_WRITE:
					SOCKET csock = wParam;
					int id;
					for(id = 0; id < server_num; id++) if(csock == server_info[id].connfd) break;
					Info& server = server_info[id];

					if(server.left != 0) {
		 				int nwrite = send(server.connfd, server.pos, server.left, 0);
		 				server.pos += nwrite;
		 				server.left -= nwrite;
		 			}
		 			else if(server.cansend){
		 				memset(server.msg, 0 , MAXBUF);
		 				int nread = 0;
		 				if(fgets(server.msg, MAXBUF, server.file) != NULL)  {
			 				nread = strlen(server.msg);
			 				int nwrite = send(server.connfd, server.msg, nread, 0);
			 				server.pos = server.msg + nwrite;
			 				server.left = nread - nwrite;
			 				server.cansend = 0;
			 				
			 				strtok(server.msg,"\r\n");
			 				sprintf(output, "<script>document.all['m%d'].innerHTML += \"<b>", id);
			 				for(int pos = 0,len = strlen(server.msg); pos != len; pos++) {
			 					if(server.msg[pos] == '>') strcat(output, "&gt;");
			 					else if(server.msg[pos] == '<') strcat(output, "&lt;");
			 					else if(server.msg[pos] == ' ') strcat(output, "&nbsp;");
			 					else strcat(output, server.msg[pos]);
			 				}
			 				strcat(output, "</b><br>\";</script>");
			 				send(ssock, output, strlen(output), 0); //WM_SOCKET_NOTIFY?
		 				}
	 		 			if(nread == 0 || strncmp("exit",server.msg, 4) == 0) {
	 		 				fclose(server.file);
		 					shutdown(server.connfd, SHUT_WR);
		 				}
		 			}
					break;
				case FD_CLOSE:
					break;
			}
		default:
			return FALSE;


	};

	return TRUE;
}

int EditPrintf (HWND hwndEdit, TCHAR * szFormat, ...)
{
     TCHAR   szBuffer [1024] ;
     va_list pArgList ;

     va_start (pArgList, szFormat) ;
     wvsprintf (szBuffer, szFormat, pArgList) ;
     va_end (pArgList) ;

     SendMessage (hwndEdit, EM_SETSEL, (WPARAM) -1, (LPARAM) -1) ;
     SendMessage (hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) szBuffer) ;
     SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0) ;
	 return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0); 
}