g++ rv2db.cpp `mysql_config --cflags --libs` -L/opt/ayonix/genderage/1.0/sdk/lib -I/opt/ayonix/genderage/1.0/sdk/include -I/usr/local/log4cxx/include -I/opt/ayonix/faceid/pro/3.4/sdk/include/ -lqpidclient -lqpidmessaging -lAyonixFaceID -lAyonixGenAge -fopenmp -llog4cxx -g -Wall -o rv2db



