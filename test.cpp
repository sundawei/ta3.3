#include <stdio.h>
#include <stdlib.h>


void getga(int& g,int& a)
{
	FILE *fp=popen("./getGenderAge","r");
	if(fp==NULL)
		return ;
	char buf[10]={0};
	
	int count=0;
	while(fgets(buf,10,fp)!=NULL)
	{
		//printf("%d,%d\n",++count,atoi(buf));
		count++;
		if(count==1)
			g=atoi(buf);
		if(count==2)
		{
			a=atoi(buf);
		}

	}

	if(pclose(fp)==-1)
	{
		printf("error pclose\n");
	}
}


int main(int argc,char* argv[])
{
	int ia=-1,ig=-1;
	getga(ig,ia);
	printf("%d,%d\n",ig,ia);

	return 0;
}