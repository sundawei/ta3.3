#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/typeof/typeof.hpp> 
#include <boost/foreach.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h> 
using namespace boost::property_tree;
using namespace std;

struct C_Setting
{
	string place;
	string pos;
	string camname;
	string mjpeg;
	string qurl;
	string qaddress;
	string workmode;
	string m0;
	string m1;
	string m2;
};
typedef vector<C_Setting> SeT;
SeT readset( std::ifstream & is )
{
	using boost::property_tree::ptree;
	ptree pt;
	read_xml(is, pt);

	SeT ans;
	BOOST_FOREACH( ptree::value_type const& v, pt.get_child("cameras") ) 
	{
		if( v.first == "camera" ) 
		{
			C_Setting f;
			f.place = v.second.get<std::string>("place");
			f.pos = v.second.get<std::string>("pos");
			f.camname = v.second.get<std::string>("camname");
			f.mjpeg = v.second.get<std::string>("mjpeg");
			f.qurl = v.second.get<std::string>("qurl");
			f.qaddress = v.second.get<std::string>("qaddress");
			f.workmode = v.second.get<std::string>("workmode");
			f.m0 = v.second.get<std::string>("m0");
			f.m1 = v.second.get<std::string>("m1");
			f.m2 = v.second.get<std::string>("m2");

			ans.push_back(f);
		}
	}
	return ans;
}


int main(int argc,char* argv[])
{
	pid_t child[16]={0};
	ifstream is("./ta_config.xml");
	SeT t=readset(is);
	printf("%d\n",t.size());
	int i=0;
	//for(i=0;i<t.size();i++)
	//{
	//	printf("%s\n",t.at(i).camname.c_str());
	//	char *cmd =new char[1024];
	//	sprintf(cmd,"%s %s %s %s %s %s %s %s %s %s %s","./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str());
	//	system(cmd);
	//	delete [] cmd;
	//printf("execl error %d\n",errno);

	//}
	if(t.size()>0)//find configs begin to fork one by one
	{
		child[0] = fork();
		if(0==child[0])
		{
			//child1
			i=0;	
			char *cmd =new char[1024];
			execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
			delete [] cmd;
			exit(EXIT_SUCCESS);
		}
		if(t.size()>1)
		{	
			i=1;
			child[1] = fork();
			if(0==child[1])
			{
				//child2

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);
			}
		}
		if(t.size()>2)
		{
			i=2;
			child[2] = fork();
			if(0==child[2])
			{
				//child3

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);
			}	
		}
		if(t.size()>3)
		{
			i=3;
			child[3] = fork();
			if(0==child[3])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>4)
		{
			i=4;
			child[4] = fork();
			if(0==child[4])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>5)
		{
			i=5;
			child[5] = fork();
			if(0==child[5])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>6)
		{
			i=6;
			child[6] = fork();
			if(0==child[6])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>7)
		{
			i=7;
			child[7] = fork();
			if(0==child[7])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>8)
		{
			i=8;
			child[8] = fork();
			if(0==child[8])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>9)
		{
			i=9;
			child[9] = fork();
			if(0==child[9])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>10)
		{
			i=10;
			child[10] = fork();
			if(0==child[10])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>11)
		{
			i=11;
			child[11] = fork();
			if(0==child[11])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>12)
		{
			i=12;
			child[12] = fork();
			if(0==child[12])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>13)
		{
			i=13;
			child[13] = fork();
			if(0==child[13])
			{
				//child

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>14)
		{
			i=14;
			child[14] = fork();
			if(0==child[14])
			{
				//childi

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}

		if(t.size()>15)
		{
			i=15;
			child[15] = fork();
			if(0==child[15])
			{
				//child4

				char *cmd =new char[1024];
				execl("./FaceUpLoader",t.at(i).place.c_str(),t.at(i).pos.c_str(),t.at(i).camname.c_str(),t.at(i).mjpeg.c_str(),t.at(i).qurl.c_str(),t.at(i).qaddress.c_str(),t.at(i).workmode.c_str(),t.at(i).m0.c_str(),t.at(i).m1.c_str(),t.at(i).m2.c_str(),NULL);
				delete [] cmd;
				exit(EXIT_SUCCESS);	
			}	
		}
		int tall=i;
		while(t.size()>0)
		{
			int pw=0;
			for(int q=0;q<=i;q++)
			{
				pw=waitpid(child[q],NULL,WNOHANG);
				if(pw==child[q])
				{
					printf("[process %d end]\n",pw);
					printf("try 2 restart ended process\n");
					child[q]=fork();
					if(0==child[q])				
					{

						char *cmd =new char[1024];
						execl("./FaceUpLoader",t.at(q).place.c_str(),t.at(q).pos.c_str(),t.at(q).camname.c_str(),t.at(q).mjpeg.c_str(),t.at(q).qurl.c_str(),t.at(q).qaddress.c_str(),t.at(q).workmode.c_str(),t.at(q).m0.c_str(),t.at(q).m1.c_str(),t.at(q).m2.c_str(),NULL);
						delete [] cmd;
						exit(EXIT_SUCCESS);
					}	
					//tall--;
				}
			}
			sleep(1);
			//printf("[ %d wait child end]\n",getpid());

		}
	}
	return 0;
}
