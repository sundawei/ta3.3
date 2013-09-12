#include <stdio.h>
#include "AyonixGenAge.h"
#include <ctype.h>
#include <string.h>

#define SAFE_CALL(call, callname)                       \
        res = call;                                     \
        if(res != AYNX_OK)                              \
        {                                               \
                printf(callname "() error: %d\n", res); \
                return res;                             \
        }

const char *ageStrings[] = {"0-10", "5-15", "10-20", "15_25", "20-30", "25_35", "30-40", "35-45", "40-50", "45+"};
const char *MaleFemale[] = {"male", "female"};

void *engine2 = 0;
char enginePath[100]={0};


char *trimwhitespace(char *str)
{
	char *end;
	while(isspace(*str)) str++;
	if(*str == 0)  // All spaces?
        return str;
	// Trim trailing space
	 end = str + strlen(str) - 1;
	while(end > str && isspace(*end)) end--;
	*(end+1) = 0;
        return str;
}

int  getGenderAge(char* img,int size,int &igender,int &iage)
{
	int res;

	if(engine2==0)
	{
		FILE *iFile = fopen("./aga.cfg", "r");
		if(!iFile)
		{
        	printf("Cannot read config file: aga.ini\n");
        	//LOG4CXX_TRACE(logger,"Cannot read config file: aga.ini");
		}
		fgets(enginePath, 100, iFile);
		fclose(iFile);
		//printf("%s,%s\n",enginePath,trimwhitespace(enginePath));

		SAFE_CALL(AGAInitialize(trimwhitespace(enginePath), &engine2), "AGAInitialize");
	}
	AynxImage *img0 = 0;
	AynxFaces *faces = 0;
	AynxGenderEx gender;
	AynxAge age;
	//FILE * ftmp = fopen("temp.jpg","wb");
	//fwrite(img,size,1,ftmp);
	//fclose(ftmp);

	SAFE_CALL(AGALoadImage("temp.jpg", &img0), "AGALoadImage");
	SAFE_CALL(AGADetectLargestFace(engine2, img0, &faces), "AGADetectLargestFace");	

	if(faces->nFaces > 0)
    {
		//printf("Detected face: (%d, %d) (%dx%d)\n\n", faces->faces[0].location.x, faces->faces[0].location.y, faces->faces[0].location.w, faces->faces[0].location.h);
		//printf("Detected face: (%d, %d) (%dx%d)\n\n", faces->faces[0].location.x, faces->faces[0].location.y, faces->faces[0].location.w, faces->faces[0].location.h);
		SAFE_CALL(AGAPreprocessFaces(engine2, faces), "AGAPreprocessFaces");
        SAFE_CALL(AGAEstimageGenderEx(engine2, faces, &gender), "AGAEstimageGenderEx");
        SAFE_CALL(AGAEstimageAge(engine2, faces, &age), "AGAEstimageAge");

       // printf("Gender estimate: %.2f%% male, %.2f%% female\n", gender.pMale * 100, gender.pFemale * 100);
        //LOG4CXX_TRACE(logger,"Gender estimate: " << gender.pMale * 100 <<" male, "<<gender.pFemale * 100 <<" female");
       // printf("Age estimate: %s\n", ageStrings[age-1]);
       // LOG4CXX_TRACE(logger,"Age estimate: " << ageStrings[age-1]);

        igender = gender.pMale>gender.pFemale?0:1;
        iage = age-1;
    }
    else
    {
    	//LOG4CXX_TRACE(logger,"Failed 2 get face for genderage");
    	printf("Failed 2 get face for genderage\n");
    }
    SAFE_CALL(AGAReleaseFaces(faces), "AGAReleaseFaces");
    SAFE_CALL(AGAReleaseImage(img0), "AGAReleaseImage");
    //SAFE_CALL(AGAFinalize(engine), "AGAFinalize");
    return 0;
}
int main(int argc,char* argv[])
{
	int ia=-1,ig=-1;
	getGenderAge(0,0,ig,ia);
	//printf("%d,%d",ig,ia);
	printf("%d\n",ig);
	printf("%d\n",ia);
	return 0;
}