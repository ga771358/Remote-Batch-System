#include "config.h"

typedef struct info {
	int connfd;
	int batch_file;
	char msg[MAX_BUF];
	char* pos;
	int left;
	bool canwrite;
} Info;

Info server_info[5];

typedef struct data {
	char ip[20];
	int port;
	char filename[100];
} Data;

Data server_data[5];
char response[MAX_BUF],msg[MAX_BUF];

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
	 
	//char query[MAX_BUF] = "h1=140.113.216.36&p1=1234&f1=t1.txt&h2=140.113.216.36&p2=1235&f2=t2.txt&h3=140.113.216.36&p3=1236&f3=t3.txt&h4=140.113.216.36&p4=1237&f4=t4.txt&h5=140.113.216.36&p5=1238&f5=t5.txt";
	char parameter[15][100] = {0};
	char* query = getenv("QUERY_STRING");
	//cout << query << endl;
	int id = 0, para_num = 1;
	char* token = strtok(query,"&");
	strcpy(parameter[0],token);
	while(token = strtok(NULL, "&")) strcpy(parameter[i++],token);

	for(int k = 0; k < para_num; k++) {
		strtok(parameter[k],"=");
		char* val = strtok(NULL, "&");
		if(val != NULL) {
		 	if(parameter[k][0] == 'h') strcpy(server_data[id].ip,val); 
		 	else if(parameter[k][0] == 'p') server_data[id].port = atoi(val); 
		 	else if(parameter[k][0] == 'f') sprintf(server_data[id++].filename, "%s" ,val); //need to modify
	 	}
	}

	int server_num = id, maxfd = 0, total_conn = 0;
	cout << "Content-type: text/html\n\n";
	cout << "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" /><title>Network Programming Homework 3</title></head>";
	cout << "<body bgcolor=#336699><font face=\"Courier New\" size=2 color=#FFFF99>";
	cout << "<table width=\"800\" border=\"1\">";
	cout << "<tr>";
	for(int id = 0; id < server_num; id++) cout << "<td>" << server_data[id].ip << "</td>";
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
		inet_pton(AF_INET,server_data[id].ip, &client_sin.sin_addr);
		client_sin.sin_port = htons(server_data[id].port);

		if(connect(server.connfd, (struct sockaddr *) &client_sin, sizeof(client_sin)) < 0) {
			//perror("connect");
			close(server.connfd);
			server.connfd = -1;
		}
		else {
			total_conn++;
			FD_SET(server.connfd, &rset);
			FD_SET(server.connfd, &wset);
			if(server.connfd > maxfd) maxfd = server.connfd;
    		server.batch_file = open(server_data[id].filename , O_RDONLY);
    		server.canwrite = 0;
    		//perror("open");
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
	 				cout << "<script>document.all['m" << id << "'].innerHTML += \"";
	 				for(int pos = 0,len = strlen(msg); pos != len; pos++) {
	 					if(msg[pos] == '\n') cout << "<br>";
	 					else if(msg[pos] == '<') cout << "&lt;";
	 					else if(msg[pos] == '>') cout << "&gt;";
	 					else if(msg[pos] == '"') cout << "&quot;";
	 					else if(msg[pos] == ' ') cout << "&nbsp;";
	 					else {
	 						if(msg[pos] == '%') server.canwrite = 1;
	 						cout << msg[pos];
	 					}
	 				}
	 				cout << "\";</script>";
	 			}
	 			else {
	 				close(server.connfd);
	 				FD_CLR(server.connfd,&rset);
	 				FD_CLR(server.connfd,&wset);
	 				server.connfd = -1;
	 				total_conn--;
	 			}
	 		}
	 		if(FD_ISSET(server.connfd, &result_wset)) {
	 			if(server.left != 0) {
	 				int nwrite = write(server.connfd, server.pos, server.left);
	 				
	 				server.pos = server.msg + nwrite;
	 				server.left -= nwrite;
	 			}
	 			else if(server.canwrite){
	 				memset(server.msg, 0 , MAX_BUF);
	 				memset(add_br, 0 , MAX_BUF);
	 				int nread = readline(server.batch_file, server.msg);
	 				int nwrite = write(server.connfd, server.msg, nread);
	 				server.canwrite = 0;
	 				server.pos = server.msg + nwrite;
	 				server.left = nread - nwrite;
	 				strtok(server.msg,"\r\n");
	 				cout << "<script>document.all['m" << id << "'].innerHTML += \"<b>";
	 				for(int pos = 0,len = strlen(server.msg); pos != len; pos++) {
	 					if(server.msg[pos] == '>') cout << "&gt;";
	 					else if(server.msg[pos] == '<') cout << "&lt;";
	 					else if(server.msg[pos] == '\n') cout << "error!" << endl;
	 					else if(server.msg[pos] == ' ') cout << "&nbsp;";
	 					else cout << server.msg[pos];
	 				}
	 				cout << "</b><br>\";</script>";
 		 			if(nread == 0 || strncmp("exit",server.msg,4) == 0) {
	 					shutdown(server.connfd, SHUT_WR);
	 					FD_CLR(server.connfd, &wset);
	 				}
	 			}
	 		}
	 	}
	 }

	 cout << "</font></body></html>";
	 return 0;
 }