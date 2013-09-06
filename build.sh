g++ -Wall -I/usr/local/log4cxx/include -I/usr/local/include/opencv -I/usr/include/curl -I../include -I /opt/ayonix/faceid/pro/3.4/sdk/include/ -O0 -g3 -Wall -c -fmessage-length=0 -fopenmp -o FaceUploader.o FaceUploader.cpp
g++ -L/opt/ayonix/faceid/pro/3.4/sdk/lib/ -L../lib/ -o FaceUpLoader  ./FaceUploader.o   -lopencv_core -lqpidclient -lAyonixFaceID -lpthread -lqpidmessaging -lcurl -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_contrib -lopencv_legacy -lopencv_flann -luuid -llog4cxx -Wall



