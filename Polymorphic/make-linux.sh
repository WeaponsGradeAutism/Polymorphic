gcc 'database.c' 'database.h' 'httpd.c' 'httpd.h' 'main.c' 'vector.c' 'vector.h' -lsqlite3 -lmicrohttpd -I./ -std=c11 -O2 -o poly
