rm -f reserve
g++ Reserve.cpp `mysql_config --cflags --libs` -I/usr/local/log4cxx/include -I/opt/ayonix/faceid/pro/3.4/sdk/include/ -lqpidclient -lqpidmessaging -lAyonixFaceID -fopenmp -llog4cxx -g -Wall -o reserve
chmod +x reserve
