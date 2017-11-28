#include "config.h"

int TcpListen(struct sockaddr_in* servaddr,socklen_t servlen,int port){
    int listenfd,reuse = 1;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) cout << "socket error" << endl;
    
    bzero(servaddr, servlen);
    servaddr->sin_family = AF_INET;
    servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr->sin_port = htons(port);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    if(bind(listenfd, (struct sockaddr*) servaddr, servlen) < 0) cout << "bind error" << endl;
    if(listen(listenfd, MAXCONN) < 0) cout << "listen error" << endl; /*server listen port*/
    return listenfd;
}

void removezombie(int signo){    
    while ( waitpid(-1 , NULL, WNOHANG) > 0 ) cout << "remove zombie!" << endl;
}

int readline(int fd,char* ptr) {
	char* now = ptr;
	memset(ptr, 0 , MAXBUF);
    while(read(fd, now, 1) > 0) {
        if(*now !='\n') ++now;
        else return now-ptr+1;
    }
    return now-ptr;
}

int main(int argc, char* argv[]){

	struct sockaddr_in cli_addr, serv_addr;
	socklen_t clilen = sizeof(cli_addr);
    if(argv[1] == NULL) return 0;
 
 	char ROOT[MAXBUF] = "/home/liuyikuan/RemoteBatchSystem/";
 	chdir(ROOT);
    signal(SIGCHLD, removezombie);
    int listenfd = TcpListen(&serv_addr, sizeof(serv_addr), atoi(argv[1]));

    while(true) {

    	int connfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);

    	int pid = fork();
    	if(pid == 0){
    		
    		char request[MAXBUF],content[MAXBUF] = {0};
    		
			readline(connfd, request);
			while(readline(connfd, content)) if(strcmp(content, "\r\n") == 0) break;
			
			char* type = strtok(request, "/");
			if(strncmp("GET", type, 3) == 0) {
				char* url = strtok(NULL, " "), *protocol = strtok(NULL, "\r\n");
				char full_path[MAXBUF],name[MAXBUF],code[MAXBUF];
				strtok(url, ".");
				char* extension = strtok(NULL,"?");
				sprintf(name, "%s.%s", url , extension);
				sprintf(full_path, "%s%s", ROOT, name);
				cout << "full_path: " << full_path << endl;
				cout << "file_name: " << name << endl;
				if(extension != NULL && strcmp("cgi", extension) == 0) {
					
					struct stat sb;
					if(stat(full_path, &sb) == 0 && sb.st_mode & S_IXUSR) {
						char* parameter = strtok(NULL, " ");
						sprintf(code,"%s 200 OK\r\n", protocol);
    					write(connfd, code, strlen(code));

						if(parameter != NULL) setenv("QUERY_STRING", parameter, 1);
						else setenv("QUERY_STRING", "h1=&p1=&f1=&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5=", 1);
						int pid = fork();
						if(pid == 0) {
							cout << "exec cgi!" << endl;
							dup2(connfd,0);
							dup2(connfd,1);
							execlp(full_path, name, NULL);
						}
					}
					else {
						sprintf(code,"%s 404 Not Found\r\n\r\n", protocol);
    					write(connfd, code, strlen(code));
					}
				}
				else {
					int file_fd = open(full_path , O_RDONLY), n;
					if(file_fd > 0) {

						sprintf(code,"%s 200 OK\r\n",protocol);
    					if(strncmp("htm", extension, 3) == 0 ) sprintf(content,"%sContent-Type: %s\r\n\r\n", code, "text/html");
    					else sprintf(content,"%sContent-Type: %s\r\n\r\n", code, "text/plain");
    					write(connfd, content, strlen(content));

    					memset(content, 0 , MAXBUF);
    					while(n = read(file_fd, content, MAXBUF)) {
    						write(connfd, content, n);
    						memset(content, 0 , MAXBUF);
    					}

    					close(file_fd);
					}
					else {
						sprintf(code,"%s 404 Not Found\r\n\r\n", protocol);
    					write(connfd, code, strlen(code));
					}
				}
			}
    		exit(0);
    	}
    	else close(connfd);
    }
}
