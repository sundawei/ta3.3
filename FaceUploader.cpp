/*
* FaceUploader.cpp
*
*  Created on: Jun 5, 2013
*      Author: sun
*/
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <cv.h>
#include <highgui.h>
#include <curl.h>
#include <pthread.h>
#include "AyonixTypes.h"
#include "AyonixTypesEx.h"
#include "AyonixFaceID.h"

#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <stdlib.h>


#include <sstream>
#include <uuid/uuid.h>

#include <log4cxx/logger.h>    
#include <log4cxx/logstring.h> 
#include <log4cxx/propertyconfigurator.h> 

using namespace log4cxx; 
using namespace qpid::messaging;
using namespace qpid::types;

using std::stringstream;
using std::string;
using namespace std;

#define _TCHAR char
#define _tprintf printf
#define _tfopen fopen
#define _fgetts fgets
#define _tcsstr strstr
#define _T(x) x

#ifndef _MAX_PATH
#define _MAX_PATH       1024
#endif


#define   IPCKEY 0x88F
#define   MSG_FILE "./msgq" 
#define   BUFFER 1024*1024*5 
#define   PERM S_IRUSR|S_IWUSR 


vector<Message> msgQ;
//change send msg by system v

key_t v_key; 
int v_msgid; 
struct msgtype { 
    long mtype; 
    char buffer[BUFFER]; 
};
struct RealMsg{
	long msglen;
	char* buffer;
};

struct msgtype SMSG;
struct msgtype RMSG;


pthread_mutex_t file_mutex; 

char FBUFFER[1024*1024*5];

int WF_count=0;

char place[64] = {0};
char pos[64] = {0};
char camname[64] = {0};
char mjpeg[64] = {0};
char qurl[64] = {0};
char qaddress[64] = {0};
char workmode[64] = {0};
char m0[64] = {0};
char m1[64] = {0};
char m2[64] = {0};

using namespace cv;

LoggerPtr logger=0;

struct MemoryStruct {
	char *memory;
	size_t size;
};

struct SendParam {
	string a;
	string b;
	string c;
	IplImage* img;
};





typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID, UUID;
GUID CreateGuid()
{
    GUID guid;
#ifdef WIN32
    CoCreateGuid(&guid);
#else
    uuid_generate(reinterpret_cast<unsigned char *>(&guid));
#endif
    return guid;
}
std::string GuidToString(const GUID &guid)
{
    char buf[64] = {0};
#ifdef __GNUC__
    snprintf(
#else // MSVC
    _snprintf_s(
#endif
                buf,
                sizeof(buf),
                 "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                guid.Data1, guid.Data2, guid.Data3,
                guid.Data4[0], guid.Data4[1],
                guid.Data4[2], guid.Data4[3],
                guid.Data4[4], guid.Data4[5],
                guid.Data4[6], guid.Data4[7]);
        return std::string(buf);
}



void EncodeImg2Jpg(IplImage* img,char* buffer,int & len)
{
	//IplImage* fIplImageHeader;
	//fIplImageHeader = cvCreateImageHeader(cvSize(160, 120), 8, 3);
	//fIplImageHeader->imageData = (char*) memblock;

	vector<int> p;
	p.push_back(CV_IMWRITE_JPEG_QUALITY);
	p.push_back(100);
	vector<unsigned char> buf;
	cv::imencode(".jpg", (Mat)img, buf, p);

	len = buf.size();
	memcpy(buffer,buf.data(),buf.size());

	std::vector<unsigned char>().swap(buf);

	//cvReleaseImageHeader(&fIplImageHeader);
}

void getdirfiles(const char* sdir,unsigned int & count,vector<string>& fname)
{
	/*
	count = getdirfilecount(sdir);
	if(count = 0)
	return ;
	*/
	DIR   *dp;   
	struct   dirent   *dirp;    
	if((dp=opendir(sdir))==NULL)   
	{   
		LOG4CXX_TRACE(logger,"no dir " << sdir);
		count = 0;
		return ;   
	}


	int ret = 0;

	while((dirp=readdir(dp))!=NULL)   
	{   

		if(strcmp(dirp->d_name,".")==0   ||   strcmp(dirp->d_name,"..")==0)   
			continue;
		string sall = string(sdir)+string("/")+string(dirp->d_name);

		fname.push_back(sall);
		LOG4CXX_TRACE(logger,"push file " << sall);

		ret++;   
	} 
	count = ret;

}

int get_filesize(const char *filename)
{
    struct stat f_stat;
    if (stat(filename, &f_stat) == -1) 
    {
        return -1;
    }
    return f_stat.st_size;
}

int sneFace2Qpid(Message message)
{
	int ret = 0;
	std::string connectionOptions =  "";

	Connection connection(qurl, connectionOptions);
	connection.setOption("reconnect", true);
	try {
		connection.open();
		Session session = connection.createSession();
		Sender sender = session.createSender(qaddress);
		sender.send(message, true);
		connection.close();
		LOG4CXX_TRACE(logger,"send a message"); 
		ret = 1;

	} catch(const std::exception& error) {
		std::cout << error.what() << std::endl;
		LOG4CXX_TRACE(logger,error.what()); 
		connection.close();
		ret = 0;
	}
	return ret;
}
string GetDateString()
{
	time_t now;
	struct tm *tm_now;
	char    sdatetime[20];

	time(&now);
	tm_now = localtime(&now);
	strftime(sdatetime, 20, "./%Y-%m-%d", tm_now);
	return std::string(sdatetime);
}

void  *SendMessageFormFile(void *arg)
{
	while(1)
	{
		sleep(1);
		pthread_mutex_lock(&file_mutex);
		if(msgQ.size()>0)
		{
			for(int i=0;i<msgQ.size();i++)
			{
				sneFace2Qpid(msgQ.at(i));
			}
			vector<Message>().swap(msgQ);
		}
		pthread_mutex_unlock(&file_mutex);

/*
		if(-1!=msgrcv(v_msgid, &RMSG, sizeof(struct msgtype), 1, 0))
		{
			struct RealMsg* msg1=(struct RealMsg*)(RMSG.buffer);

			Message msg;
			msg.setContent(msg1->buffer,msg1->msglen);
			while(1!=sneFace2Qpid(msg));
		}
		else
		{
			LOG4CXX_TRACE(logger,"msgrcv error "<<strerror(errno)); 
		}
*/

		/*
		string sdir=GetDateString();
		sdir+=string("-Mes-")+string(pos)+string("/");
		vector<string> v;
		unsigned int count;
		

		getdirfiles(sdir.c_str(),count,v);
		if(count>0)
		{
			for(int i=0;i<v.size();i++)
			{
				pthread_mutex_lock(&file_mutex);
				int size = get_filesize(v.at(i).c_str());
				FILE* fp=fopen(v.at(i).c_str(),"rb");
				fread(FBUFFER,size,1,fp);
				fclose(fp);
				pthread_mutex_unlock(&file_mutex);

				Message msg;
				msg.setContent(FBUFFER,size);

				while(1!=sneFace2Qpid(msg));
				remove(v.at(i).c_str());
				//break;

			}

		}
		v.clear();
		std::vector<string>().swap(v);
		
		sleep(5);
		//*/

	}
	return 0;

}


void SaveMessage2Disk(char* buffer, int length)
{
	//string sdir=GetDateString();
	//sdir+=string("-Mes-")+string(pos)+string("/");
	//mkdir(sdir.c_str(),0775); 

	LOG4CXX_TRACE(logger,"encode and send msg ");
	Variant::Map content;
	content["place"] = place;
	content["pos"] = pos;
	content["camname"] = camname;
	content["workmode"] = workmode;
	content["m0"] = m0;
	content["m1"] = m1;
	content["m2"] = m2;
	content["int"] = time(0);
	string spic;
	spic.assign(buffer,length);
	content["picture"] = spic;
	content["uuid"] = GuidToString(CreateGuid());	
	Message message;
	encode(content, message);

	pthread_mutex_lock(&file_mutex);
	if(msgQ.size()<50)
	msgQ.push_back(message);
	pthread_mutex_unlock(&file_mutex);

/*
	memset(&SMSG,0,sizeof(SMSG));
	SMSG.mtype = 1;
	struct RealMsg* sm = (struct RealMsg*)(SMSG.buffer);
	sm->msglen  = message.getContentSize();
	memcpy(sm->buffer,message.getContentPtr(),sm->msglen);

	if(-1==msgsnd(v_msgid, &SMSG, sizeof(struct msgtype), 0))
	{
		LOG4CXX_TRACE(logger,"msgsnd error "<<strerror(errno));
	}
	else 
	{
		LOG4CXX_TRACE(logger,"send a message len = "<<sm->msglen);
	}
*/


/*
	string sfile=sdir+string(content["uuid"]);
	pthread_mutex_lock(&file_mutex);
	FILE* fp=fopen(sfile.c_str(),"wb");
	fwrite(message.getContentPtr(),message.getContentSize(),1,fp);
	fclose(fp);
	pthread_mutex_unlock(&file_mutex);
	LOG4CXX_TRACE(logger,"save a message "<<sfile<<" size="<<message.getContentSize()); 
//*/
}
int SendFace2Qpid(char* buffer,int length)
{
	/*
	int ret = 0;
	std::string connectionOptions =  "";

	Connection connection(qurl, connectionOptions);
	connection.setOption("reconnect", true);
	try {
		connection.open();
		Session session = connection.createSession();
		Sender sender = session.createSender(qaddress);

		Message message;
		Variant::Map content;
		content["place"] = place;
		content["pos"] = pos;
		content["camname"] = camname;
		content["workmode"] = workmode;
		content["m0"] = m0;
		content["m1"] = m1;
		content["m2"] = m2;
		content["int"] = time(0);
		string spic;
		spic.assign(buffer,length);
		content["picture"] = spic;
		content["uuid"] = GuidToString(CreateGuid());
		encode(content, message);
		sender.send(message, true);
		connection.close();
		ret = 1;
//*/		
		SaveMessage2Disk(buffer,length);
		return 1;
/*		
		LOG4CXX_TRACE(logger,"send a message "<<content["int"]<<" "<<content["uuid"]); 
	} catch(const std::exception& error) {
		std::cout << error.what() << std::endl;
		LOG4CXX_TRACE(logger,error.what()); 
		connection.close();
		ret = 0;
	}
	return ret;
//*/	
}

void  *SendInfo(void *arg)
{
	SendParam* ppm = (SendParam*)arg;
	char* SendBuffer=new char[1024*1024*4];
	int piclen=0;
	EncodeImg2Jpg(ppm->img,SendBuffer,piclen);
	printf("[sam]piclen = %d\n",piclen);
	int ret = SendFace2Qpid(SendBuffer,piclen);
	while(ret==0)
	{
		ret = SendFace2Qpid(SendBuffer,piclen);
	}

	delete [] SendBuffer;
	cvReleaseImage(&ppm->img);
	delete [] (char*)arg;
	return 0;

}

void cropImage(IplImage* src,IplImage* & dstimg,CvRect r)
{
	dstimg = cvCreateImage(cvSize(r.width,r.height),src->depth,src->nChannels);
	(((Mat)(src))(r)).convertTo( ((Mat)(dstimg)), ((Mat&)(dstimg)).type(),1,0);
}
void *engine =0;

void* InitializeEngine(_TCHAR* fileName)
{
	LOG4CXX_TRACE(logger,"initializing engine... "); 
	FILE* fp = _tfopen(fileName, _T("rt"));
	if (fp != 0) {
		int res = 0;
		//void* engine = 0;

		_TCHAR path[_MAX_PATH];
		_fgetts(path, _MAX_PATH, fp);

		if(isspace(path[strlen(path)-1]))
			path[strlen(path)-1] = 0;
		LOG4CXX_TRACE(logger," " << path); 

		res = AFIDInitialize(path, &engine);
		if (res != AYNX_OK) {
			LOG4CXX_TRACE(logger,"ifailed to initialize engine " << res); 
			exit(1);
		}

		LOG4CXX_TRACE(logger,"ok"); 
		fclose(fp);
		return engine;
	}
	else {
		LOG4CXX_TRACE(logger,"failed to find "<< fileName); 
		exit(1);
	}
}

void FinalizeEngine(void* engine)
{
	LOG4CXX_TRACE(logger,"finalizing engine..."); 
	int res = AFIDFinalize(engine);
	if (res == AYNX_OK)
	{
		LOG4CXX_TRACE(logger,"ok"); 
	}	
	else
	{
		LOG4CXX_TRACE(logger,"failed"); 
	}
}

int SaveBinaryFile(void* data, unsigned int size, _TCHAR* name)
{
	FILE* fp = _tfopen(name, _T("wb"));
	if (fp == 0) {
		LOG4CXX_TRACE(logger,"failed to create" <<name); 
		return AYNX_ERR_FILE_OPEN;
	}
	if (fwrite(data, size, 1, fp) == 1) {
		fclose(fp);
		return AYNX_OK;
	}
	else {
		fclose(fp);
		return AYNX_ERR_FILE_WRITE;
	}
}


const char* pFormat = "%Y-%m-%d-%H-%M-%S";
void Enroll(int argc, _TCHAR* argv[],IplImage* img0,char* jpgdata,int jpglen)
{


	// set 1:N mode for enrollment
	unsigned int mode = 1;
	int res = AFIDSetParam(engine, AYNX_AFID_ENGINE_PARAM_FR_MODE, &mode);
	if (res != AYNX_OK) {
		LOG4CXX_TRACE(logger,"failed to set recognition mode" << res <<" exit"); 
		exit(1);
	}

	unsigned int nImages = 1;
	// allocate faces storage
	AynxFaces** faces = (AynxFaces **)malloc(nImages * sizeof(AynxFaces *));
	if (faces == 0) {
		LOG4CXX_TRACE(logger,"failed to allocate faces exit"); 
		exit(1);
	}
	memset(faces, 0, nImages * sizeof(AynxFaces *));

	// allocate faces collection for enrollment
	AynxFacesCollection facesCollection;
	facesCollection.nFaces = 0;
	facesCollection.faces = (AynxFaces *)malloc(nImages * sizeof(AynxFaces));
	if (facesCollection.faces == 0) {
		LOG4CXX_TRACE(logger,"failed to allocate faces collection exit"); 
		exit(1);
	}
	memset(facesCollection.faces, 0, nImages * sizeof(AynxFaces));


	// detect and preprocess faces in the images and put them to faces collection for enrollment
	for (unsigned int i = 0; i < nImages; i++) {

		AynxImage *img = 0;
		res = AFIDDecodeImage(jpgdata,jpglen,&img);
		if (res == AYNX_OK) {


			// detect faces
			res = AFIDDetectFaces(engine, img, &faces[i]);
			if (res != AYNX_OK) {
				LOG4CXX_TRACE(logger,"failed to detect faces "<<res<<" exit"); 
				exit(1);
			}

			// put detected faces to faces collection for enrollment
			memcpy(facesCollection.faces + i, faces[i], sizeof(AynxFaces));
			facesCollection.nFaces++;

			if (faces[i]->nFaces == 0) {
				AFIDReleaseImage(img);
				continue;
			}

			// preprocess faces
			res = AFIDPreprocessFaces(engine, faces[i]);
			if (res != AYNX_OK) {
				LOG4CXX_TRACE(logger,"failed to preprocess face "<<res<<" exit"); 
				exit(1);
			}
			for( int t=0;t<faces[i]->nFaces;t++)
			{
				if(faces[i]->faces[t].mugshot.location.x>0&& faces[i]->faces[t].mugshot.location.y>0 &&(faces[i]->faces[t].mugshot.location.w>0&&faces[i]->faces[t].mugshot.location.h>0) )
				{
					IplImage *aface;

					int xx=faces[i]->faces[t].mugshot.location.x-5;
					int yy=faces[i]->faces[t].mugshot.location.y-5;
					int ww=faces[i]->faces[t].mugshot.location.w+5;
					int hh=faces[i]->faces[t].mugshot.location.h+5;
					if(xx<0)
						xx=0;
					if(yy<0)
						yy=0;
					if(xx+ww>(img0->width-1))
					{
						ww=img0->width-1-xx;
					}
					if(yy+hh>(img0->height-1))
					{
						hh=img0->height-1-hh;
					}
					cropImage(img0,aface,cvRect(xx,yy,ww,hh));
					char* pq=new char[sizeof(SendParam)];
					SendParam* op = (SendParam*)pq;
					op->img =  cvCloneImage(aface);
					//pthread_t  tid;
					//pthread_create( &tid, NULL, SendInfo, op );
					SendInfo(op);
					WF_count++;
					if(WF_count%500==0)
					{
						//system("sync");
						//system("echo 3 >/proc/sys/vm/drop_caches");
						//LOG4CXX_TRACE(logger,"try 2 free cache memory!");
					}
					cvReleaseImage(&aface);


				}
				_tprintf(_T("[%d]ok. %d,%d %dx%d face rectangle\n"),getpid(),
					faces[i]->faces[t].mugshot.location.x,
					faces[i]->faces[t].mugshot.location.y,
					faces[i]->faces[t].mugshot.location.w,
					faces[i]->faces[t].mugshot.location.h);
			}


		}
		else {
			LOG4CXX_TRACE(logger,"failed to read "<<res);
			exit(1);
		}
		AFIDReleaseImage(img);
	}
	for (unsigned int i = 0; i < nImages; i++)
		AFIDReleaseFaces(faces[i]);
	free(faces);
	free(facesCollection.faces);
}

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */
		LOG4CXX_TRACE(logger,"not enough memory (realloc returned NULL)");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}


void getPic(char* pic,long & picsize)
{
	CURL *curl_handle;
	CURLcode res;

	struct MemoryStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	curl_easy_setopt(curl_handle, CURLOPT_URL, mjpeg);
	// curl_easy_setopt(curl_handle, CURLOPT_URL, "http://root:SPadmin01@219.101.248.183:8101/jpg/image.jpg");
	//curl_easy_setopt(curl_handle, CURLOPT_URL, "http://root:agent@192.168.1.105/jpg/image.jpg");
	curl_easy_setopt(curl_handle, CURLOPT_FORBID_REUSE, 1);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);



	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	/* some servers don't like requests that are made without a user-agent
	field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */
	res = curl_easy_perform(curl_handle);

	/* check for errors */
	if(res != CURLE_OK) {
		LOG4CXX_TRACE(logger,"curl_easy_perform() failed: "<<curl_easy_strerror(res));
		picsize = 0;
	}
	else {
		memcpy(pic,chunk.memory,chunk.size);
		picsize = chunk.size;
	}
	curl_easy_cleanup(curl_handle);

	if(chunk.memory)
		free(chunk.memory);
	chunk.memory=0;
	curl_global_cleanup();

	return ;
}
char buffer[1024*1024*5];
long nsize=0;

void  *GetUrlPic(void *arg)
{
	while(1)
	{
		getPic(buffer,nsize);
		if(nsize>0)
		{
			//cv::Mat imgbuf = cv::Mat(800, 600,CV_8U , buffer);

			std::vector<char> data1(buffer, buffer + nsize);
			//cv::Mat imgMat = cv::imdecode(imgbuf, CV_LOAD_IMAGE_COLOR);
			cv::Mat imgMat = cv::imdecode(Mat(data1), CV_LOAD_IMAGE_COLOR);

			IplImage img=imgMat;
			//printf("image size = %d,%d\n",img.width,img.height);

			Enroll(0,0,&img,buffer,nsize);
			data1.clear();
			std::vector<char>().swap(data1);


		}
		usleep(1000*200);
		if(cvWaitKey(20)==27)
			break;
	}
	cvDestroyAllWindows();

	return 0;
}



int main( int argc, char** argv )
{
	PropertyConfigurator::configure("ta_faceuploader_logconfig.cfg"); 
	logger = Logger::getLogger("Trace_FaceUpLoader"); 
/*
	if((v_key=ftok(MSG_FILE,IPCKEY))==-1) 
    { 
        LOG4CXX_TRACE(logger,"Creat Key Error: " << strerror(errno));
        exit(1); 
    }

    if((v_msgid=msgget(v_key, PERM|IPC_CREAT|IPC_EXCL))==-1) 
    {
        LOG4CXX_TRACE(logger,"Creat Message Error: " << strerror(errno));
        exit(1);
    } 

    LOG4CXX_TRACE(logger,"v_msgid = " << v_msgid);
*/    
    //printf("msqid = %d/n", msgid);



	pthread_mutex_init(&file_mutex,NULL); 

	if(argc!=10)
	{
		LOG4CXX_TRACE(logger,"pid = " << getpid() << "bad start args argc= "<<argc); 
	}
	else
	{
		strcpy(place,argv[0]);
		strcpy(pos,argv[1]);
		strcpy(camname,argv[2]);
		strcpy(mjpeg,argv[3]);
		strcpy(qurl,argv[4]);
		strcpy(qaddress,argv[5]);
		strcpy(workmode,argv[6]);
		strcpy(m0,argv[7]);
		strcpy(m1,argv[8]);
		strcpy(m2,argv[9]);

	}
	pthread_t  tid,tidsnd;
	int  ret;
	engine = InitializeEngine("./afid.conf");
	ret = pthread_create( &tid, NULL, GetUrlPic, 0 );
	ret = pthread_create( &tidsnd, NULL, SendMessageFormFile, 0);
	while(1)
	{
		sleep(10000000);
	}
	FinalizeEngine(engine);
	pthread_mutex_destroy(&file_mutex);
	return 0;
}
