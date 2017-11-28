all: httpd hw3.cgi

httpd: httpd.cpp config.h
	g++ httpd.cpp -o httpd
hw3.cgi: cgi.cpp config.h
	g++ cgi.cpp -o hw3.cgi