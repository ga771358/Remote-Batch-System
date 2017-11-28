all: httpd hw3.cgi

httpd: httpd.cpp config.h
	g++ -g httpd.cpp -o httpd
hw3.cgi: cgi.cpp config.h
	g++ -g cgi.cpp -o hw3.cgi