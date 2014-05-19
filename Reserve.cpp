#include <stdio.h>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/typeof/typeof.hpp> 
#include <boost/foreach.hpp>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Session.h>
#include <qpid/types/Variant.h>
#include <cstdlib>
#include <sstream>
#include <my_global.h>
#include <mysql.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>  
#include <unistd.h>  
#include <sys/stat.h>  
#include <ctype.h>
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include "AyonixTypes.h"
#include "AyonixTypesEx.h"
#include "AyonixFaceID.h"
#include <log4cxx/logger.h>    
#include <log4cxx/logstring.h> 
#include <log4cxx/propertyconfigurator.h> 
using namespace std;
using namespace qpid::messaging;
using namespace qpid::types;
using std::stringstream;
using std::string;
using namespace log4cxx; 
using namespace boost::property_tree;

#define _TCHAR char
#define _tprintf printf
#define _tfopen fopen
#define _fgetts fgets
#define _tcsstr strstr
#define _T(x) x
#ifndef _MAX_PATH
#define _MAX_PATH       1024
#endif


char sql_server[30] = {0};
char sql_user[30] = {0};
char sql_passwd[30] = {0};
char sql_db[30] = {0};
char qpid_server[256] = {0};
char qpid_address[256] = {0};

struct R_Setting
{
	string sql_server;
	string sql_user;
	string sql_passwd;
	string sql_db;
	string qpid_server;
	string qpid_address;
};


LoggerPtr logger=0;
void *engine =0;
int DAYS = 30;

void prtlog(const char* arg)
{
	printf("%s\n",arg);
}

void* InitializeEngine(_TCHAR* fileName)
{
	FILE* fp = _tfopen(fileName, _T("rt"));
	if (fp != 0) {
		int res = 0;
		//void* engine = 0;

		_TCHAR path[_MAX_PATH];
		_fgetts(path, _MAX_PATH, fp);

		if(isspace(path[strlen(path)-1]))
			path[strlen(path)-1] = 0;

		res = AFIDInitialize(path, &engine);
		if (res != AYNX_OK) {
			prtlog("AFIDInitialize failed");
			exit(1);
		}
		fclose(fp);
		return engine;
	}
	else {
		exit(1);
	}
}

void FinalizeEngine(void* engine)
{

	int res = AFIDFinalize(engine);
	if (res == AYNX_OK)
	{

	}
	else
	{
		prtlog("AFIDFinalize failed");
	}
}
void finish_with_error(MYSQL *con)
{
	fprintf(stderr, "%s\n", mysql_error(con));
	mysql_close(con);
	exit(1);        
}


typedef vector<R_Setting> SeT;
SeT readset( std::ifstream & is )//read server settings
{
	using boost::property_tree::ptree;
	ptree pt;
	read_xml(is, pt);

	SeT ans;
	BOOST_FOREACH( ptree::value_type const& v, pt.get_child("servers") ) 
	{
		if( v.first == "server" ) 
		{
			R_Setting f;
			f.sql_server = v.second.get<std::string>("sql_server");
			f.sql_user = v.second.get<std::string>("sql_user");
			f.sql_passwd = v.second.get<std::string>("sql_passwd");
			f.sql_db = v.second.get<std::string>("sql_db");
			f.qpid_server = v.second.get<std::string>("qpid_server");
			f.qpid_address = v.second.get<std::string>("qpid_address");
			ans.push_back(f);
		}
	}
	return ans;
}
int split(const string& str, vector<string>& ret_, string sep = ",")
{
	if (str.empty())
	{
		return 0;
	}

	string tmp;
	string::size_type pos_begin = str.find_first_not_of(sep);
	string::size_type comma_pos = 0;

	while (pos_begin != string::npos)
	{
		comma_pos = str.find(sep, pos_begin);
		if (comma_pos != string::npos)
		{
			tmp = str.substr(pos_begin, comma_pos - pos_begin);
			pos_begin = comma_pos + sep.length();
		}
		else
		{
			tmp = str.substr(pos_begin);
			pos_begin = comma_pos;
		}

		if (!tmp.empty())
		{
			ret_.push_back(tmp);
			tmp.clear();
		}
	}
	return 0;
}
int SaveBinaryFile(void* data, unsigned int size, _TCHAR* name)
{
	FILE* fp = _tfopen(name, _T("wb"));
	if (fp == 0) {
		//LOG4CXX_TRACE(logger,"failed to create " << name);
		prtlog("failed to create bin file");
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
void getfacesafid(vector<string>& allfaces,string suuid)
{
	//升级 日期目录下的afid文件,重新用多张人脸数据enroll
	unsigned int nImages = allfaces.size();
	int res = 0;

	// allocate faces storage
	AynxFaces** faces = (AynxFaces **)malloc(nImages * sizeof(AynxFaces *));
	if (faces == 0) {
		prtlog("failed to allocate faces");
		return;
	}
	memset(faces, 0, nImages * sizeof(AynxFaces *));

	// allocate faces collection for enrollment
	AynxFacesCollection facesCollection;
	facesCollection.nFaces = 0;
	facesCollection.faces = (AynxFaces *)malloc(nImages * sizeof(AynxFaces));
	if (facesCollection.faces == 0) {
		prtlog("failed to allocate faces collection");
		return;
	}
	memset(facesCollection.faces, 0, nImages * sizeof(AynxFaces));

	// detect and preprocess faces in the images and put them to faces collection for enrollment
	for (unsigned int i = 0; i < nImages; i++) {

		AynxImage *img = 0;
		//res = AFIDLoadImageA(imagePath, &img);
		//LOG4CXX_TRACE(logger,"begin decode "<<allfaces.at(i).size());
		char dbs[64]={0};
		sprintf(dbs,"begin decode %ld",allfaces.at(i).size());
		prtlog(dbs);
		res = AFIDDecodeImage((char*)allfaces.at(i).data(),allfaces.at(i).size(),&img);
		prtlog("end decode");
		if (res == AYNX_OK) {


			// detect faces
			res = AFIDDetectFaces(engine, img, &faces[i]);

			if (res != AYNX_OK) {
				sprintf(dbs,"failed to detect faces %d",res);
				prtlog(dbs);
				return ;
			}

			// put detected faces to faces collection for enrollment
			memcpy(facesCollection.faces + i, faces[i], sizeof(AynxFaces));
			facesCollection.nFaces++;

			if (faces[i]->nFaces == 0) {
				prtlog("no faces");
				AFIDReleaseImage(img);
				continue;
			}

			// preprocess faces
			res = AFIDPreprocessFaces(engine, faces[i]);
			if (res != AYNX_OK) {
				sprintf(dbs,"failed to preprocess faces %d",res);
				prtlog(dbs);
				return;
			}


			//LOG4CXX_TRACE(logger," " << faces[i]->faces[0].location.x<<" "<<faces[i]->faces[0].location.y<<" "<<faces[i]->faces[0].location.w<<" "<<faces[i]->faces[0].location.h);
		}
		else {

			//LOG4CXX_TRACE(logger,"failed to read " <<res);
			return;
		}
		if(img!=0)
			AFIDReleaseImage(img);
	}

	void* afid = 0;
	unsigned int size = 0;
	// enroll from faces collection
	//LOG4CXX_TRACE(logger,"update enrolling... 3");
	prtlog("update enrolling... 3");

	string safiddata;

	res = AFIDEnrollPerson(engine, &facesCollection, &afid, &size);
	if (res == AYNX_OK) {
		//LOG4CXX_TRACE(logger,"ok");
		prtlog("ok");
		//mkdir(GetDateString().c_str(),0775);
		//mkdir(pkgdir.c_str(),0775);
		char fn[100]={0};
		//memset(fn,0,100);
		//strcpy(fn,(GetDateString()+string("/")+suuid).c_str());
		//strcpy(fn,(pkgdir+string("/")+suuid).c_str());
		sprintf(fn,"./%s",suuid.c_str());
		res = SaveBinaryFile(afid,size,fn);
		safiddata.assign((char*)afid,size);
		if (res != AYNX_OK)
		{
		//	LOG4CXX_TRACE(logger,"failed to save afid");
			prtlog("failed to save afid");
		}
		//LOG4CXX_TRACE(logger,"success enrolling.... 2");
		prtlog("save new afid 2 ");
		prtlog(fn);
		prtlog("success enrolling.... 2");
		AFIDReleaseAfid(afid);
	}
	else
	{
		AFIDReleaseAfid(afid);
		//LOG4CXX_TRACE(logger,"failed "<<res<<" face count "<<facesCollection.nFaces);
		prtlog("failed 2013");
	}

	// free resources
	for (unsigned int i = 0; i < nImages; i++)
		AFIDReleaseFaces(faces[i]);
	free(faces);
	free(facesCollection.faces);

	//升级 数据库中safid的afid和最后入库时间
	//if(safiddata.size()>0)
	//	UpdateDBAfid(suuid,safiddata,newfacetime);
}


int gatherfaces(char* uuidssql,std::vector<string>& facedata)
{
	char SQL[256]={0};

	sprintf(SQL,"select * from faceall where uuid in %s order by  dt asc",uuidssql);//order by time, fist data will be the final record

	MYSQL *con = mysql_init(NULL);

	if (con == NULL)
	{
		fprintf(stderr, "mysql_init() failed\n");
		exit(1);
	}  

	if (mysql_real_connect(con, sql_server, sql_user, sql_passwd, 
		sql_db, 0, NULL, 0) == NULL) 
	{
		finish_with_error(con);
	} 


	if (mysql_query(con, SQL))
	{  
		finish_with_error(con);
	}

	MYSQL_RES *result = mysql_store_result(con);

	if (result == NULL) 
	{
		finish_with_error(con);
	}  

	int num_fields = mysql_num_fields(result);

	MYSQL_ROW row;
	//MYSQL_FIELD *field;
	std::vector<string> valluuid;


	int tc=0;
	while ((row = mysql_fetch_row(result)))
	{
		unsigned long *lengths;
		lengths = mysql_fetch_lengths(result);
		if(lengths==NULL)
		{
			break;
		}

		for(int i = 0; i < num_fields; i++)
		{
			if(i==1)
			{
				string sp;
				sp.assign(row[i],lengths[i]);

				facedata.push_back(sp);
				//allfaces.push_back(sp);
				//char fn[20]={0};
				//sprintf(fn,"./%04d.jpg",tc);
				//FILE *fp=fopen(fn,"wb");
				//fwrite(row[i],lengths[i],1,fp);
				//fclose(fp);
			}
			//if(i==12)
			//{
				//printf("%s\n",row[i]);
				//valluuid.push_back(row[i]);
			//}
		}
		tc++;
		if(tc>=30)
			break;
	}

	mysql_free_result(result);
	mysql_close(con);
	return 0;
}
void getbdirs(long tttm,int days,vector<string>& salldirs)//find available date dirs 
{
        for(int i=0;i<days;i++)
        {
                long atm = tttm - i*24*3600;
                struct tm *tm_atm = localtime(&atm);
                char adir[16] = {0};
                strftime(adir,16,"./%Y-%m-%d",tm_atm);
                DIR   *dp;
                if((dp=opendir(adir))==NULL)
                {
                       // LOG4CXX_TRACE(logger," "<<adir <<" not exists");
                }
                else
                {
                      //  LOG4CXX_TRACE(logger,"add a date dir "<<adir);
                        salldirs.push_back(adir);
                }
                closedir(dp);

        }
}
void getdirfiles(const char* sdir,unsigned int & count,vector<string>& fname)//get filelist from dir
{
	std::vector<string> vdirs;
	getbdirs(time(0),DAYS,vdirs);
	count = 0;
	if(vdirs.size()==0||strcmp(sdir,"./bdata")==0)//bddir spec
	{	

		DIR   *dp;   
		struct   dirent   *dirp;    
		if((dp=opendir(sdir))==NULL)   
		{   
			//LOG4CXX_TRACE(logger,"no dir " << sdir);
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
		//	LOG4CXX_TRACE(logger,"push file " << sall);

			ret++;   
		}	 
		count += ret;
		closedir(dp);
	}
	else
	{
		for(int i=0;i<static_cast<int>(vdirs.size());i++)
		{
			DIR   *dp;   
			struct   dirent   *dirp;    
			if((dp=opendir(vdirs.at(i).c_str()))==NULL)   
			{   
			//	LOG4CXX_TRACE(logger,"no dir " << sdir);
				count = 0;
				closedir(dp);
				continue ;   
			}


			int ret = 0;

			while((dirp=readdir(dp))!=NULL)   
			{	   

				if(strcmp(dirp->d_name,".")==0   ||   strcmp(dirp->d_name,"..")==0)   
					continue;
				string sall = vdirs.at(i)+string("/")+string(dirp->d_name);

				fname.push_back(sall);
				//LOG4CXX_TRACE(logger,"push file " << sall);

				ret++;   
			}	 
			count += ret;
			closedir(dp);
		}
	}

}


void chang2sameuuid(std::vector<string> uuids,const char* gender,const char* age)//test passed
{
	MYSQL *con = mysql_init(NULL);
	char *lsbuf=new char[uuids.size()*64];
	//printf("%ld\n",uuids.size()*64);

	memset(lsbuf,0,uuids.size()*64);
	strcat(lsbuf," (");
	for(unsigned int i=0;i<uuids.size();i++)
	{
		strcat(lsbuf,(string("'")+uuids.at(i)+string("',")).c_str());
	}
	strcat(lsbuf,"'samsun')");

	prtlog(lsbuf);

	//prtlog("1");

	

	//prtlog("1.1");
	if (con == NULL)
	{
		prtlog("mysql_init() failed");
		return;
	}  
	//prtlog("1.2");
	if (mysql_real_connect(con, sql_server, sql_user, sql_passwd, 
		sql_db, 0, NULL, 0) == NULL) 
	{
		prtlog("1.21");
		finish_with_error(con);
		return;
	} 


	const char* qs="UPDATE faceall SET uuid='%s',gender='%s',age='%s' WHERE uuid in %s";
	char *query=new char[uuids.size()*64+300];
	int len= snprintf(query,uuids.size()*64+300,qs,uuids.at(0).c_str(),gender,age,lsbuf);
	prtlog(query);


//*
	if (mysql_real_query(con, query, len))
	{
		delete [] query;
		delete [] lsbuf;
		finish_with_error(con);
		return;
	} 
//*/
	const char* qs2="UPDATE face set ldt = (select dt from faceall where uuid='%s' order by dt desc limit 1),gender='%s',age='%s' where uuid = '%s'";
	char *query2 = new char[512];
	int len2 = snprintf(query2,512,qs2,uuids.at(0).c_str(),gender,age,uuids.at(0).c_str());
	prtlog(query2);
//*
	if(mysql_real_query(con,query2,len2))
	{
		delete [] query2;
		finish_with_error(con);
		return;
	}

//*/




	mysql_close(con);
	
	delete [] query;
	delete [] query2;
	delete [] lsbuf;


}

void delexcessuuids(std::vector<string> uuids)
{
	char *lsbuf=new char[uuids.size()*64];
	memset(lsbuf,0,uuids.size()*64);
	strcat(lsbuf," (");
	for(unsigned int i=1;i<uuids.size();i++)
	{
		strcat(lsbuf,(string("'")+uuids.at(i)+string("',")).c_str());
	}
	strcat(lsbuf,"'samsun')");

	MYSQL *con = mysql_init(NULL);

	if (con == NULL)
	{
		return;
	}  

	if (mysql_real_connect(con, sql_server, sql_user, sql_passwd, 
		sql_db, 0, NULL, 0) == NULL) 
	{
		finish_with_error(con);
		return;
	} 

	const char* qs="DELETE from face WHERE uuid in %s";
	char *query=new char[uuids.size()*64+200];

	int len= snprintf(query,uuids.size()*64+200,qs,lsbuf);
	prtlog(query);

//*
	if (mysql_real_query(con, query, len))
	{
		delete [] query;
		delete [] lsbuf;
		finish_with_error(con);
		return;
	} 
//*/
	mysql_close(con);
	
	delete [] query;
	delete [] lsbuf;


}
void combinfaces(std::vector<string> uuids,const char* gender,const char* age)
{
	//faces to be combined, fist data will be us as the final one

	//2:find all corresponding face record and there face data in table faceall, regenerate the afid data with this face data
	char *lsbuf=new char[uuids.size()*64];
	memset(lsbuf,0,uuids.size()*64);
	strcat(lsbuf," (");
	for(unsigned int i=0;i<uuids.size();i++)
	{
		strcat(lsbuf,(string("'")+uuids.at(i)+string("',")).c_str());
	}
	strcat(lsbuf,"'samsun')");

	std::vector<string> v;
	gatherfaces(lsbuf,v);//get all same face jpg data
	getfacesafid(v,uuids.at(0));//regenerate afid, afid file save 2 file ./uuids.at(0)

	//1:delete all data with same face in face table and remove afid data files, change to the new afid file
	unsigned int count=0;
	std::vector<string> vf;
	getdirfiles("",count,vf);//get all afid file saved on disk
	for(unsigned int p=1;p<uuids.size();p++)
	{
		for(unsigned int q=0;q<vf.size();q++)
		{
			int sfaidlen = vf.at(q).size();
			string cs = vf.at(q).substr(sfaidlen-36,36);
			if(uuids.at(p)==cs)
			{
				unlink(vf.at(q).c_str());
				prtlog("unlink ");
				prtlog(vf.at(q).c_str());

			}
		}
	}

	for(unsigned int t=0;t<vf.size();t++)//replace the origin afid file of these faces on hd disk
	{
		int sfaidlen = vf.at(t).size();
		string cs = vf.at(t).substr(sfaidlen-36,36);
		if(uuids.at(0)==cs)
		{
			rename((string("./")+uuids.at(0)).c_str(),vf.at(t).c_str());
			prtlog("rename ");
			prtlog((string("./")+uuids.at(0)).c_str());
			prtlog(" to ");
			prtlog(vf.at(t).c_str());
			prtlog("\n");
			break;
		}
	}


	//3:change all afid data with the same face afid and uuid

	chang2sameuuid(uuids,gender,age);//change faceall same face with sam uuid(uuids.at(0))
	delexcessuuids(uuids);//delete face table same face record only first appear record left. 
	
	//4:tell user the operation result.

	if(lsbuf!=NULL)
	{
		delete [] lsbuf;
		lsbuf=NULL;
	}
}


int mergeface(const char* ids,const char* gender,const char* age)
{
	char SQL[256]={0};

	sprintf(SQL,"select * from face where id in %s order by dt asc",ids);//order by time, fist data will be the final record
	prtlog(SQL);

	MYSQL *con = mysql_init(NULL);

	if (con == NULL)
	{
		fprintf(stderr, "mysql_init() failed\n");
		exit(1);
	}  

	if (mysql_real_connect(con, sql_server, sql_user, sql_passwd, 
		sql_db, 0, NULL, 0) == NULL) 
	{
		finish_with_error(con);
	} 


	if (mysql_query(con, SQL))
	{  
		finish_with_error(con);
	}

	MYSQL_RES *result = mysql_store_result(con);

	if (result == NULL) 
	{
		finish_with_error(con);
	}  

	int num_fields = mysql_num_fields(result);

	MYSQL_ROW row;
	//MYSQL_FIELD *field;
	std::vector<string> valluuid;


	int tc=0;
	while ((row = mysql_fetch_row(result)))
	{
		unsigned long *lengths;
		lengths = mysql_fetch_lengths(result);
		if(lengths==NULL)
		{
			break;
		}

		for(int i = 0; i < num_fields; i++)
		{
			if(i==1)
			{
				//string sp;
				//sp.assign(row[i],lengths[i]);
				//allfaces.push_back(sp);
				//char fn[20]={0};
				//sprintf(fn,"./%04d.jpg",tc);
				//FILE *fp=fopen(fn,"wb");
				//fwrite(row[i],lengths[i],1,fp);
				//fclose(fp);
			}
			if(i==12)
			{
				//printf("%s\n",row[i]);
				valluuid.push_back(row[i]);
			}
		}
		tc++;
		if(tc>=30)
			break;
	}

	if(valluuid.size()>0)
	{
		combinfaces(valluuid,gender,age);
	}

	printf("\n");

	mysql_free_result(result);
	mysql_close(con);
	return 0;
}
void changefv(char* sfv)
{
	using boost::property_tree::ptree;
	printf("%s\n",sfv);
	ifstream is("./rvdb_config.xml");
	ptree pt;
	read_xml(is, pt);
	is.close();
	ofstream is2("./rvdb_config.xml");
	pt.put( "servers.server.fv", sfv );
	write_xml(is2,pt);
	is2.close();
	return;
}
void getdirfiles100(const char* sdir,unsigned int & count,vector<string>& fname)//get filelist from dir
{
	std::vector<string> vdirs;
	getbdirs(time(0),100,vdirs);
	count = 0;
	if(vdirs.size()==0||strcmp(sdir,"./bdata")==0)//bddir spec
	{	

		DIR   *dp;   
		struct   dirent   *dirp;    
		if((dp=opendir(sdir))==NULL)   
		{   
			//LOG4CXX_TRACE(logger,"no dir " << sdir);
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
		//	LOG4CXX_TRACE(logger,"push file " << sall);

			ret++;   
		}	 
		count += ret;
		closedir(dp);
	}
	else
	{
		for(int i=0;i<static_cast<int>(vdirs.size());i++)
		{
			DIR   *dp;   
			struct   dirent   *dirp;    
			if((dp=opendir(vdirs.at(i).c_str()))==NULL)   
			{   
			//	LOG4CXX_TRACE(logger,"no dir " << sdir);
				count = 0;
				closedir(dp);
				continue ;   
			}


			int ret = 0;

			while((dirp=readdir(dp))!=NULL)   
			{	   

				if(strcmp(dirp->d_name,".")==0   ||   strcmp(dirp->d_name,"..")==0)   
					continue;
				string sall = vdirs.at(i)+string("/")+string(dirp->d_name);

				fname.push_back(sall);
				//LOG4CXX_TRACE(logger,"push file " << sall);

				ret++;   
			}	 
			count += ret;
			closedir(dp);
		}
	}

}
void setnonface(char* uuid)
{
	std::vector<string> v;
	unsigned int ccc=0;
	string suuid=uuid;
	getdirfiles100("./",ccc,v);
	for(int i=0;i<v.size();i++)
	{
		printf("%s\n",v.at(i).c_str());
		int sfaidlen = v.at(i).length();
		string strsub=v.at(i).substr(sfaidlen-36,36);
		if(suuid == strsub)
		{
			string srtn=string("./bdata/")+strsub;
			rename(v.at(i).c_str(),srtn.c_str());
			printf("move afid 2 %s\n",srtn.c_str());
			break;
		}
	}
	std::vector<string>().swap(v);
}
int main(int argc, char** argv)
{
	//read config
	//*

	printf("%d\n",argc);
	for(int i=0;i<argc;i++)
	{
		printf("%s\n",argv[i]);
	}
	//return 0;

	ifstream is("./reserve_config.xml");
	SeT t=readset(is);
	is.close();
	if(t.size()>0)
	{

		strcpy(sql_server,t.at(0).sql_server.c_str());
		prtlog(sql_server);
		strcpy(sql_user,t.at(0).sql_user.c_str());
		prtlog(sql_user);
		strcpy(sql_passwd,t.at(0).sql_passwd.c_str());
		prtlog(sql_passwd);
		strcpy(sql_db,t.at(0).sql_db.c_str());
		prtlog(sql_db);
		strcpy(qpid_server,t.at(0).qpid_server.c_str());
		prtlog(qpid_server);
		strcpy(qpid_address,t.at(0).qpid_address.c_str());
		prtlog(qpid_address);
	}
	else
	{
		printf("reserve no config file\n"); 
		return 0;
	}
	//*/


	//	mkdir(GetDateString().c_str(),0775);
	//	LOG4CXX_TRACE(logger, GetDateString()); 

	char ffnn[20]={0};
	sprintf(ffnn,"%s","afid2.conf");
	InitializeEngine(ffnn);
	//*
	unsigned int mode = 1;
	int res = AFIDSetParam(engine, AYNX_AFID_ENGINE_PARAM_FR_MODE, &mode);
	if (res != AYNX_OK) {
		prtlog("failed to set recognition mode");
		//LOG4CXX_TRACE(logger, _T("failed to set recognition mode ") << res); 
		//LOG4CXX_TRACE(logger, _T("ta_reserve exit")); 
		exit(1);
	}
	//*/
	/*
	while(1)
	{
		break;
		Connection connection(qpid_server, "");
		connection.setOption("reconnect", true);
		try {
			connection.open();
			Session session = connection.createSession();
			Receiver receiver = session.createReceiver(qpid_address);
			Variant::Map content;
			decode(receiver.fetch(Duration::SECOND*60), content);
			string scmd=content["cmd"];
			string scontent=content["content"];

			session.acknowledge();
			receiver.close();
			connection.close();
		}
		catch(const std::exception& error) {
			prtlog(error.what());
			connection.close();
		}
	}
	//*/
	/*
	//test split
	string s="6,8,9";
	std::vector<string> v;
	split(s,v);
	for(unsigned int i=0;i<v.size();i++)
	{
	printf("%s\n",v.at(i).c_str());
	}
	//*/	
	//mergeface("(1,2,8)");

/*
	unsigned int count=0;
	std::vector<string> vf;
	getdirfiles("",count,vf);
	for(unsigned int i=0;i<count;i++)
	{
		//printf(vf.at(i).c_str());
		//printf("\n");

		int sfaidlen = vf.at(i).size();
		string cs = vf.at(i).substr(sfaidlen-36,36);
		printf("%s\n",cs.c_str());
		
	}
//*/
//	unlink("./abc.txt");
	/*
	std::vector<string> v;
	v.push_back("5161BDEB-F8A2-84E2-4D6E-1BDE00000000");
	v.push_back("84D19792-83A0-A08C-BC52-E85000000000");
	v.push_back("49AB28DF-3EBB-EAE1-557C-9E1500000000");
	//chang2sameuuid(v);
	delexcessuuids(v);
	//*/

	if(argc==4)//example: ./resrve "(1,2,3)"
	{
		mergeface(argv[1],argv[2],argv[3]);
	}
	if(argc==2)
	{
		changefv(argv[1]);
	}
	if(argc==3)
	{
		setnonface(argv[1]);
	}
	FinalizeEngine(engine);
	prtlog("main end");
	return 0;
}