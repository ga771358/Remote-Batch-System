#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
#define MAX_BUF 1024

typedef struct info {
	int connfd;
	int batch_file;
	char msg[MAX_BUF];
	char* pos;
	int left;
} Info;

Info server_info[5];

typedef struct data {
	char ip[20];
	int port;
	char filename[20];
} Data;

Data server_data[5];
char response[MAX_BUF],msg[MAX_BUF],add_br[MAX_BUF];

int readline(int fd,char* ptr) {
	char* now = ptr;
    while(read(fd, now, 1) > 0) {
        if(*now !='\n') ++now;
        else return now-ptr+1;
    }
    close(fd);
    return now-ptr;
}

int main(int argc, char* argv[],char* envp[]) {
	 
	char query[MAX_BUF] = "h1=140.113.216.36&p1=1234&f1=t1.txt&h2=140.113.216.36&p2=1235&f2=t2.txt&h3=140.113.216.36&p3=1236&f3=t3.txt&h4=140.113.216.36&p4=1237&f4=t4.txt&h5=140.113.216.36&p5=1238&f5=t5.txt";
	//query = getenv("QUERY_STRING");
	cout << query << endl;

	int id = 0;
	for(char* parameter = strtok(query,"="),*val; parameter != NULL; parameter = strtok(NULL, "=")) {
	 	if(parameter[0] == 'h') {
	 		val = strtok(NULL, "&");
	 		strcpy(server_data[id].ip,val); 
	 	}
	 	else if(parameter[0] == 'p') {
	 		val = strtok(NULL, "&");
	 		server_data[id].port = atoi(val); 
	 	}
	 	else if(parameter[0] == 'f') {
	 		val = strtok(NULL, "&");
	 		strcpy(server_data[id++].filename,val); 
	 	}
	}

	int server_num = id, maxfd = 0, total_conn = 0;
	cout << "Content-type: text/html\r\n\r\n";
	cout << "<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />\n<title>Network Programming Homework 3</title>\n</head>\n";
	cout << "<body bgcolor=#336699>\n<font face=\"Courier New\" size=2 color=#FFFF99>\n";
	cout << "<table width=\"800\" border=\"1\">\n";
	cout << "<tr>\n";
	for(int id = 0; id < server_num; id++) cout << "<td>" << server_data[id].ip << "</td>";
	cout << "</tr>\n<tr>\n";
	for(int id = 0; id < server_num; id++) cout << "<td valign=\"top\" id=\"m" << id << "\"></td>";
	cout << "</tr>\n</table>\n";
			
	struct sockaddr_in client_sin;
	fd_set rset, wset, result_rset, result_wset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);

	for(int id = 0; id < server_num; id++) {
	 	Info& server = server_info[id];
 		server.connfd = socket(AF_INET, SOCK_STREAM, 0);
 		client_sin.sin_family = AF_INET;
		inet_pton(AF_INET,server_data[id].ip, &client_sin.sin_addr);
		client_sin.sin_port = htons(server_data[id].port);

		if(connect(server.connfd, (struct sockaddr *) &client_sin, sizeof(client_sin)) < 0) {
			perror("connect");
			close(server.connfd);
			server.connfd = -1;
		}
		else {
			total_conn++;
			FD_SET(server.connfd, &rset);
			FD_SET(server.connfd, &wset);
			if(server.connfd > maxfd) maxfd = server.connfd;
    		server.batch_file = open(server_data[id].filename , O_RDONLY);
    		perror("open");
			server.left = 0;
		}
	}

	 while(total_conn) {

	 	result_rset = rset, result_wset = wset;
	 	select(maxfd+1, &result_rset, &result_wset, NULL, NULL);
	 	for(int id = 0; id < server_num; id++ ) {
	 		Info& server = server_info[id];
	 		if(server.connfd < 0) continue;
	 		
	 		if(FD_ISSET(server.connfd, &result_rset)) {
	 			memset(msg, 0, MAX_BUF);
	 			memset(add_br, 0, MAX_BUF);
	 			if(read(server.connfd, msg, MAX_BUF) > 0) {
	 				for(int pos = 0,len = strlen(msg); pos != len; pos++) {
	 					if(msg[pos] == '\n') strcat(add_br,"<br>");
	 					else if(msg[pos] == '<') strcat(add_br,"&lt;");
	 					else if(msg[pos] == '>') strcat(add_br,"&gt;");
	 					else strncat(add_br,&msg[pos],1);
	 					
	 				}
	 				sprintf(response,"<script>document.all['m%d'].innerHTML += \"%s\";</script>",id,add_br);
 		 			cout << response << endl;
	 			}
	 			else {
	 				close(server.connfd);
	 				FD_CLR(server.connfd,&rset);
	 				FD_CLR(server.connfd,&wset);
	 				server.connfd = -1;
	 				total_conn--;
	 				if(total_conn == 0) cout << "</font>\n</body>\n</html>\n";
	 			}
	 		}
	 		if(FD_ISSET(server.connfd, &result_wset)) {
	 			if(server.left != 0) {
	 				int nwrite = write(server.connfd, server.pos, server.left);
	 				
	 				server.pos = server.msg + nwrite;
	 				server.left -= nwrite;
	 			}
	 			else {
	 				memset(server.msg, 0 , MAX_BUF);
	 				memset(add_br, 0 , MAX_BUF);
	 				int nread = readline(server.batch_file, server.msg);
	 	
	 				int nwrite = write(server.connfd, server.msg, nread);

	 				server.pos = server.msg + nwrite;
	 				server.left = nread - nwrite;
	 				for(int pos = 0,len = strlen(server.msg); pos != len; pos++) {
	 					if(server.msg[pos] == '>') strcat(add_br,"&gt;");
	 					else if(server.msg[pos] == '<') strcat(add_br,"&lt;");
	 					else if(server.msg[pos] == '\n') strcat(add_br, "<br>");
	 					else if(server.msg[pos] == '\r');
	 					else strncat(add_br,&server.msg[pos],1);
	 				}
	 				sprintf(response,"<script>document.all['m%d'].innerHTML += \"% <b>%s</b><br>\";",id,add_br);
 		 			cout << response << endl;
 		 			if(nread == 0 || strncmp("exit",server.msg,4) == 0) {
	 					shutdown(server.connfd, SHUT_WR);
	 					FD_CLR(server.connfd, &wset);
	 				}
	 			}
	 			//usleep(200000);//wait for response
	 		}
	 	}
	 }

	 return 0;
 }