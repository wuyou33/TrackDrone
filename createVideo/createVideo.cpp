/*********************************************************/
/******
/* Folder Image	: Images
/* Command	: ./createVideo nameVideo
/*
/*********************************************************/

//#include "stdafx.h"
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <stdio.h>
#include <time.h>



/*main*/
int main(int argc, char* argv[])
{
	
	if(argc!=2){
		printf("error parameters\n");	
		exit(1);
	}

	int i;
	char c;

	system("ls ./Images > listImages.txt");
	FILE* file = fopen("./listImages.txt","r");

	char line[64] = "";
	char nameImage[64] = "";
	char pathImage[64] = "";
	char nameVideo[64] = "";
	
	IplImage* frame = NULL;

	sprintf(nameVideo,"%s.avi",argv[1]);
	CvVideoWriter * video = cvCreateVideoWriter(nameVideo,CV_FOURCC('X','V','I','D'),15,cvSize(176,144),1);


 	//int i;
	if(file != NULL){
		while(fgets(line,64,file)!=NULL) {

			for(i=0;i<64;i++)
				nameImage[i] = '\0';
		
			i = 0;
			c = line[i];
			while(c!=10){
				nameImage[i] = line[i];
				c = line[++i];
			}
		
			printf("%s\n",nameImage);
			sprintf(pathImage,"./Images/%s",nameImage);

			frame = cvLoadImage(pathImage);

			cvWriteFrame(video,frame);
		
		}
	}

	fclose(file);



	cvDestroyAllWindows();
	
	return 0;
}
	
