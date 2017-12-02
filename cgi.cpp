#include "config.h"

typedef struct info {
	int connfd;
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
	close(server.connfd);
	server.connfd = -1;
}

int main(int argc, char* argv[],char* envp[]) {
	 
	//char query[MAXBUF] = "h1=140.113.216.36&p1=1234&f1=t1.txt&h2=140.113.216.36&p2=1235&f2=t2.txt&h3=140.113.216.36&p3=1236&f3=t3.txt&h4=140.113.216.36&p4=1237&f4=t4.txt&h5=140.113.216.36&p5=1238&f5=t5.txt";
	char* parameter[15] = {0}, ip[100];
	char* query = getenv("QUERY_STRING");
	//cout << query << endl;
	int id = 0, para_num = 0;
	parameter[para_num++] = strtok(query, "&");
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

	int server_num = id, maxfd = 0, total_conn = 0;
	cout << "Content-type: text/html\n\n" << endl;
	cout << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" /><title>Network Programming Homework 3</title></head>" << endl;
	cout << "<body bgcolor=#336699><font face=\"Courier New\" size=2 color=#FFFF99>" << endl;
	cout << "<table width=\"800\" border=\"1\">" << endl;
	cout << "<tr>";
	for(int id = 0; id < server_num; id++) cout << "<td>" << server_data[id].domain_name << "</td>";
	cout << "</tr><tr>";
	for(int id = 0; id < server_num; id++) cout << "<td valign=\"top\" id=\"m" << id << "\"></td>";
	cout << "</tr></table>";
			
	struct sockaddr_in client_sin;
	fd_set rset, wset, result_rset, result_wset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);

	for(int id = 0; id < server_num; id++) {
	 	Info& server = server_info[id];
 		server.connfd = socket(AF_INET, SOCK_STREAM, 0);
 		client_sin.sin_family = AF_INET;
 		if(hostname_to_ip(server_data[id].domain_name, ip)) close_socket(server);
 		else {
			inet_pton(AF_INET, ip, &client_sin.sin_addr);
			client_sin.sin_port = htons(server_data[id].port);

			if(connect(server.connfd, (struct sockaddr *) &client_sin, sizeof(client_sin)) < 0) {
				close_socket(server);
			}
			else {
				total_conn++;
				FD_SET(server.connfd, &rset);
				FD_SET(server.connfd, &wset);
				if(server.connfd > maxfd) maxfd = server.connfd;
	    		server.file = fopen(server_data[id].filename , "r");
	    		server.cansend = 0;
				server.left = 0;
			}
		}
	}

	while(total_conn) {

	 	result_rset = rset, result_wset = wset;
	 	select(maxfd+1, &result_rset, &result_wset, NULL, NULL);
	 	for(int id = 0; id < server_num; id++ ) {
	 		Info& server = server_info[id];
	 		if(server.connfd < 0) continue;
	 		
	 		if(FD_ISSET(server.connfd, &result_rset)) {
	 			char msg[MAXBUF] = {0};
	 			if(read(server.connfd, msg, MAXBUF) > 0) {
	 				cout << "<script>document.all['m" << id << "'].innerHTML += \"";
	 				for(int pos = 0,len = strlen(msg); pos != len; pos++) {
	 					if(msg[pos] == '\n') cout << "<br>";
	 					else if(msg[pos] == '<') cout << "&lt;";
	 					else if(msg[pos] == '>') cout << "&gt;";
	 					else if(msg[pos] == '"') cout << "&quot;";
	 					else if(msg[pos] == ' ') cout << "&nbsp;";
	 					else if(msg[pos] == '\r') ;
	 					else {
	 						if(msg[pos] == '%') server.cansend = 1;
	 						cout << msg[pos];
	 					}
	 				}
	 				cout << "\";</script>" << endl;
	 			}
	 			else {
	 				FD_CLR(server.connfd,&rset);
	 				FD_CLR(server.connfd,&wset);
	 				close_socket(server);
	 				total_conn--;
	 			}
	 		}
	 		if(FD_ISSET(server.connfd, &result_wset)) {
	 			if(server.left != 0) {
	 				int nwrite = write(server.connfd, server.pos, server.left);
	 				server.pos += nwrite;
	 				server.left -= nwrite;
	 			}
	 			else if(server.cansend){
	 				memset(server.msg, 0 , MAXBUF);
	 				int nread = 0;
	 				if(fgets(server.msg, MAXBUF, server.file) != NULL)  {
		 				nread = strlen(server.msg);
		 				int nwrite = write(server.connfd, server.msg, nread);
		 				server.pos = server.msg + nwrite;
		 				server.left = nread - nwrite;
		 				server.cansend = 0;
		 				
		 				strtok(server.msg,"\r\n");
		 				cout << "<script>document.all['m" << id << "'].innerHTML += \"<b>";
		 				for(int pos = 0,len = strlen(server.msg); pos != len; pos++) {
		 					if(server.msg[pos] == '>') cout << "&gt;";
		 					else if(server.msg[pos] == '<') cout << "&lt;";
		 					else if(server.msg[pos] == ' ') cout << "&nbsp;";
		 					else cout << server.msg[pos];
		 				}
		 				cout << "</b><br>\";</script>" << endl;
	 				}
 		 			if(nread == 0 || strncmp("exit",server.msg, 4) == 0) {
 		 				fclose(server.file);
	 					shutdown(server.connfd, SHUT_WR);
	 					FD_CLR(server.connfd, &wset);
	 				}
	 			}
	 		}
	 	}
	}
	cout << "</font></body></html>" << endl;
	return 0;
 }