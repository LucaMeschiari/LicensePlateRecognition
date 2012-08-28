//
// license.cpp
//
// https://github.com/LucaMeschiari/LicensePlateRecognition
//
// Copyright (c) 2012 Luca Meschiari <meschial@tcd.ie>  <http://www.lm-app.com>.
//
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// See <http://www.gnu.org/licenses/> for a copy of the
// GNU General Public License.
//

#ifdef _CH_
#pragma package <opencv>
#endif

#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"

#define NUM_IMAGES 9
#define NUMBER_OF_KNOWN_CHARACTERS 10

// Character features
typedef struct tLicensePlateCharacterFeatures_tag {
	bool number;					//Used to check if it is a number
	CvRect boundaryRect;	
	int xCentre;					//Coordinates of the centre
	int yCentre;
	int n_Holes;					//number of holes
	int areaRect;					//Area of the rect
	float chRatio;					//ConvexHull/Rect ratio
	int n_conc;						//number of concavitues
	int conc_ypos[5];				//Concavities data
	int conc_dir[5];
	int conc_ystart[5];
	int conc_ydepth[5];
	int conc_depth[5];
	int hole_ypos[5];				//Hole y position
} tLicensePlateCharacterFeatures;

//Function to extract feature from a contour
tLicensePlateCharacterFeatures getContourData(CvSeq* contours,CvSeq* contours_Holes){
	tLicensePlateCharacterFeatures charFeatures;
	
	
	CvRect boundingRect=cvBoundingRect(contours);
	//Check if the size is inside certain limits
	if ((boundingRect.height>27)&&(boundingRect.height<39)&&(boundingRect.width>2)&&(boundingRect.width<25)&&(boundingRect.x>15)){
		
		//Size and position
		charFeatures.boundaryRect=boundingRect;
		charFeatures.n_Holes=0;

		charFeatures.xCentre=boundingRect.x+(boundingRect.width/2);
		charFeatures.yCentre=boundingRect.y+(boundingRect.height/2);

		//Holes
		for(CvSeq* contour = contours_Holes ; contour != 0; contour = contour->h_next ){
			CvRect boundingRectHole=cvBoundingRect(contour);
			if ((boundingRectHole.height>2)&&(boundingRectHole.height<27)&&(boundingRectHole.width>2)&&(boundingRectHole.width<20)){
				int xCentreHole=boundingRectHole.x+(boundingRectHole.width/2);
				int yCentreHole=boundingRectHole.y+(boundingRectHole.height/2);
				if ((xCentreHole>boundingRect.x)&&(xCentreHole<(boundingRect.x+boundingRect.width))&&(yCentreHole>boundingRect.y)&&(yCentreHole<(boundingRect.y+boundingRect.height))){
					charFeatures.n_Holes++;
					charFeatures.hole_ypos[(charFeatures.n_Holes-1)]=yCentreHole-charFeatures.yCentre;
				}
			}
		}

		//Convex Hull Ratio
		CvMemStorage* storageHull = cvCreateMemStorage(0);
		CvMemStorage* storageAreaHull = cvCreateMemStorage(0);
		CvSeq* Hull = 0;
		Hull=cvConvexHull2(contours,storageHull,1,0);
		CvPoint* hullPointArray = (CvPoint*)malloc(sizeof(CvPoint)*Hull->total);  
		CvSeq* ptseq  = cvCreateSeq( CV_SEQ_CONTOUR|CV_32SC2, sizeof(CvContour), 
                                     sizeof(CvPoint),  storageAreaHull ); 

		for(int  i = 0; i < Hull->total; i++ ) 
        { 
            CvPoint pt = **CV_GET_SEQ_ELEM( CvPoint*, Hull, i ); 
            cvSeqPush( ptseq , &pt ); 
        } 

		double areaHull=cvContourArea(ptseq,CV_WHOLE_SEQ);
		charFeatures.areaRect=boundingRect.height*boundingRect.width;
		charFeatures.chRatio=areaHull/charFeatures.areaRect;

		//release areaHull storage

		cvReleaseMemStorage(&storageAreaHull);

		//Concavities
		//Hull=cvConvexHull2(contours,storageHull,1,0);
		charFeatures.n_conc=0;
		CvMemStorage* storageConc = cvCreateMemStorage(0);
		CvSeq* Conc = 0;
		Conc=cvConvexityDefects(contours,Hull,storageConc);
		//Convert Seq to array
		CvConvexityDefect* defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect)*Conc->total);     
        cvCvtSeqToArray(Conc,defectArray, CV_WHOLE_SEQ); 

		//Release storage
		cvReleaseMemStorage(&storageConc);
		cvReleaseMemStorage(&storageHull);

		for(int i=0; i<sizeof(defectArray);i++){
			if(defectArray[i].depth>1){
				charFeatures.n_conc++;
				if((defectArray[i].start->x<defectArray[i].depth_point->x)&&(defectArray[i].end->x<defectArray[i].depth_point->x)){
					charFeatures.conc_dir[(charFeatures.n_conc-1)]=-1;
				}
				else{
					charFeatures.conc_dir[(charFeatures.n_conc-1)]=1;
				}

				//Get Concavity data
				//Negative ->, positive <-
				charFeatures.conc_ypos[(charFeatures.n_conc-1)]=((defectArray[i].start->y+defectArray[i].end->y)/2)-charFeatures.yCentre;
				charFeatures.conc_ystart[(charFeatures.n_conc-1)]=defectArray[i].start->y;
				charFeatures.conc_ydepth[(charFeatures.n_conc-1)]=defectArray[i].depth_point->y;
				charFeatures.conc_depth[(charFeatures.n_conc-1)]=defectArray[i].depth;
			}
			
		}     

		charFeatures.number=true;
	}
	else{
		charFeatures.number=false;
	}
	return charFeatures;
}

//Get features data for the sample numbers, this function is used only in the developing phase and it is not used in the submitted application for the distribution application.
void analyze_sample_number( IplImage* source,IplImage* result,int i)
{
	IplImage* binary_image = cvCreateImage( cvGetSize(source), 8, 1 );
	IplImage* binary_image2 = cvCreateImage( cvGetSize(source), 8, 1 );
	cvConvertImage( source, binary_image );
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvMemStorage* storage2 = cvCreateMemStorage(0);
	CvSeq* contours = 0;
	CvSeq* contours_Holes = 0;
	tLicensePlateCharacterFeatures features;

	//Add sharpening
	cv::GaussianBlur((cv::Mat)binary_image, (cv::Mat)binary_image2, cv::Size(0, 0), 3);
	cv::addWeighted((cv::Mat)binary_image, 1.5, (cv::Mat)binary_image2, -0.5, 0, (cv::Mat)binary_image);

	//Threshold and morphological correction
	cvAdaptiveThreshold(binary_image,binary_image,255,0,0,33,0);
	cvMorphologyEx( binary_image, binary_image, NULL, NULL, CV_MOP_CLOSE, 1 );
	cvCopy(binary_image,binary_image2);
	cvNot(binary_image2,binary_image2);

	//Find countours for number and holes
	cvFindContours( binary_image, storage, &contours_Holes, sizeof(CvContour),	CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
	cvFindContours( binary_image2, storage2, &contours, sizeof(CvContour),	CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
	
	
	//Draw contours and analyze sample numbers
	if (result)
	{
		cvZero( result );
		for(CvSeq* contour = contours ; contour != 0; contour = contour->h_next )
		{
			CvScalar color = CV_RGB( rand()&255, rand()&255, rand()&255 );

			features=getContourData(contour,contours_Holes);

			if(features.number){
				printf("\n%d) -- h:  %3d -- l:  %3d  ||| x: %3d -- y:%3d  |||  Area Rect: %d  Ratio: %f\nConcavities %d: ",i, features.boundaryRect.height, features.boundaryRect.width,features.xCentre,features.yCentre, features.areaRect, features.chRatio,features.n_conc); 
				for (int a=0;a<features.n_conc;a++){
				printf(" %d) Dir: %d, Pos: %d |",a, features.conc_dir, features.conc_ypos);
				}
			}

			cvDrawContours( result, contour, color, color, -1, CV_FILLED, 8 );
			
		}
		for(CvSeq* contour = contours_Holes ; contour != 0; contour = contour->h_next )
		{
			CvScalar color = CV_RGB( rand()&255, rand()&255, rand()&255 );
			cvDrawContours( result, contour, color, color, -1, CV_FILLED, 8 );
		}
	}

	//release images
	cvReleaseImage(&binary_image);
	cvReleaseImage(&binary_image2);

	//Release storage
	cvReleaseMemStorage(&storage);
	cvReleaseMemStorage(&storage2);
	
}


//Matching function, it uses the features to recognize numbers
int matchFeatures(tLicensePlateCharacterFeatures features){
	if(features.boundaryRect.width<10){		//width used for 1
				return 1;
	}
	if(features.n_Holes>0){
		if (features.n_Holes==2){			//2 holes used for 8
			return 8;
		}
		if (features.n_Holes==1){			//Number of holes, position of the hole(used for 6 and 9) and Convex Hull ratio (used for 4), if not 9,6,4 it is 0
			if(features.hole_ypos[0]>3){
				return 6;
			}
			if (features.chRatio<0.7){
					return 4;
				}
			if(features.hole_ypos[0]<-3){
					return 9;	
			}
			return 0;						
		}
	}else{
		if(features.chRatio<0.7){		//No holes and convex hull ratio for 7
			return 7;
		}else if(features.n_conc==1){	//1 concavity for 3
				return 3;
		}
		else if(features.n_conc==2){	//2 concavities with the same orientation -> 3
			if((features.conc_dir[0]==-1)&&(features.conc_dir[1]==-1)){
				return 3;
			}
			if(((features.conc_dir[0]==1)&&(features.conc_dir[1]==-1))||((features.conc_dir[0]==-1)&&(features.conc_dir[1]==1))){  //2 or 5
				if(features.conc_ydepth[0]<features.conc_ydepth[1]){		//2 Concavities with different orientation for 2 and 5
					return 2;												//y position of the concavities used for differentiate 2 and 5
				}
				else{
					return 5;
				}
			}
			
		}
	}

	return 0;

}

//Function to analyze a license plate
void analyze_plate( IplImage* source,IplImage* result)
{
	//Two images to get the holes and the numbers
	IplImage* binary_image = cvCreateImage( cvGetSize(source), 8, 1 );
	IplImage* binary_image2 = cvCreateImage( cvGetSize(source), 8, 1 );
	cvConvertImage( source, binary_image );
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvMemStorage* storage2 = cvCreateMemStorage(0);
	CvSeq* contours = 0;
	CvSeq* contours_Holes = 0;
	
	//Add Sharpening
	cv::GaussianBlur((cv::Mat)binary_image, (cv::Mat)binary_image2, cv::Size(1, 1), 3);
	cv::addWeighted((cv::Mat)binary_image, 1.5, (cv::Mat)binary_image2, -0.5, 0, (cv::Mat)binary_image);

	//Add Threshold and Morphological correction
	cvAdaptiveThreshold(binary_image,binary_image,255,0,0,33,0);
	cvMorphologyEx( binary_image, binary_image, NULL, NULL, CV_MOP_CLOSE, 1 );

	cvCopy(binary_image,binary_image2);
	cvNot(binary_image2,binary_image2);

	//Find Holes and Numbers
	cvFindContours( binary_image, storage, &contours_Holes, sizeof(CvContour),	CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
	cvFindContours( binary_image2, storage2, &contours, sizeof(CvContour),	CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
	
	
	//Draw, analize and write the results
	if (result)
	{
		cvZero( result );
		for(CvSeq* contour = contours ; contour != 0; contour = contour->h_next )
		{
			CvScalar color = CV_RGB( rand()&255, rand()&255, rand()&255 );
			tLicensePlateCharacterFeatures features;
			features=getContourData(contour,contours_Holes);

			if(features.number){
				int n=matchFeatures(features);
				printf("\n-- %d) (%3d)-- h:  %3d -- l:  %3d  ||| x: %3d -- y:%3d  ||| Holes: %d \nArea Rect: %d  Ratio: %f\nConcavities %d: ",n,features.xCentre, features.boundaryRect.height, features.boundaryRect.width,features.xCentre,features.yCentre, features.n_Holes, features.areaRect, features.chRatio,features.n_conc); 
				for (int a=0;a<features.n_conc;a++){
					printf(" %d) Dir: %d, Pos: %d , yStart: %d, yDepth: %d, Depth: %d\n",a, features.conc_dir[a], features.conc_ypos[a],features.conc_ystart[a],features.conc_ydepth[a],features.conc_depth[a]);
				}
				
				char number[5];
				sprintf(number,"%d",n);
				write_text_on_image(source,features.boundaryRect.y,features.boundaryRect.x,number);
			}

			cvDrawContours( result, contour, color, color, -1, CV_FILLED, 8 );
			
		}
		for(CvSeq* contour = contours_Holes ; contour != 0; contour = contour->h_next )
		{
			CvScalar color = CV_RGB( rand()&255, rand()&255, rand()&255 );
			cvDrawContours( result, contour, color, color, -1, CV_FILLED, 8 );
		}
	}

	//release images
	cvReleaseImage(&binary_image);
	cvReleaseImage(&binary_image2);

	//Release storage
	cvReleaseMemStorage(&storage);
	cvReleaseMemStorage(&storage2);
	
}



int main( int argc, char** argv )
{
	int selected_image_num = 1;
	IplImage* selected_image = NULL;
	IplImage* sample_number_images[NUMBER_OF_KNOWN_CHARACTERS];
	IplImage* sample_number_an_images[NUMBER_OF_KNOWN_CHARACTERS];
	IplImage* connectedImage=NULL;
	IplImage* images[NUM_IMAGES];

	// Load all the sample images and determine feature values for these characters.
	for (int character=0; (character<NUMBER_OF_KNOWN_CHARACTERS); character++)
	{
		char filename[100];
		sprintf(filename,"./Real Numbers/%d.jpg",character); //./Real Numbers/%d.jpg
		if( (sample_number_images[character] = cvLoadImage(filename,-1)) == 0 )
			return 0;
	}

	// Load all the unknown license plate images.
	for (int file_num=1; (file_num <= NUM_IMAGES); file_num++)
	{
		char filename[100];
		sprintf(filename,"./LicensePlate%d.jpg",file_num);
		if( (images[file_num-1] = cvLoadImage(filename,-1)) == 0 )
			return 0;
	}

	//Get Data for each sample number --used only for the development
	/*for(int i=0;i<NUMBER_OF_KNOWN_CHARACTERS;i++){
		CvSeq* contours = 0;
		CvSeq* contours_Holes = 0;
		sample_number_an_images[i]=cvCreateImage( cvGetSize(sample_number_images[i]), IPL_DEPTH_8U, 3 );
		analyze_sample_number(sample_number_images[i],sample_number_an_images[i],i);
		char windowName[100];
		sprintf(windowName,"Sample %d",i);
		cvNamedWindow( windowName, 1 );
		cvShowImage( windowName, sample_number_an_images[i]);
	}*/	
	

	// Explain the User Interface
    printf( "\n\nHot keys: \n"
            "\tESC - quit the program\n");
    printf( "\t1..%d - select image\n",NUM_IMAGES);
    
	// Create display windows for images
    cvNamedWindow( "Original", 1 );
	cvNamedWindow( "ConnectedImage", 1 );

	// Create images to do the processing in.
	selected_image = cvCloneImage( images[selected_image_num-1] );
	connectedImage = cvCloneImage( images[selected_image_num-1] );

	// Setup mouse callback on the original image so that the user can see image values as they move the
	// cursor over the image.
    //cvSetMouseCallback( "Original", on_mouse_show_values, 0 );
	//window_name_for_on_mouse_show_values="Original";
	//image_for_on_mouse_show_values=selected_image;

	int user_clicked_key = 0;
	do {
		// Process image (i.e. setup and find the number of spoons)
		cvCopyImage( images[selected_image_num-1], selected_image );
		cvCopyImage( images[selected_image_num-1], connectedImage );
		
		printf("\nProcessing Image %d:  (Values not printed in order)\n",selected_image_num);
		analyze_plate( selected_image, connectedImage );

		
        cvShowImage( "Original", selected_image );
		cvShowImage( "ConnectedImage", connectedImage);
		

		// Wait for user input
        user_clicked_key = cvWaitKey(0);
		if ((user_clicked_key >= '1') && (user_clicked_key <= '0'+NUM_IMAGES))
		{
			selected_image_num = user_clicked_key-'0';
		}
	} while ( user_clicked_key != ESC );

	cvDestroyWindow("Original");
	cvDestroyWindow("ConnectedImage");
	
    return 1;
}
