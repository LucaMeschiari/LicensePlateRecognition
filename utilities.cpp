#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"

//SetDllDirectory("C:\OpenCV2.3\build\bin\Debug");

// Write the passed text onto the passed image
void write_text_on_image(IplImage* image, int top_row, int top_column, char* text)
{
	CvFont font;
	double hScale=0.5;
	double vScale=0.5;
	int    lineWidth=1;
	cvInitFont(&font,CV_FONT_VECTOR0, hScale,vScale,0,lineWidth);
	unsigned char colour[4] = { 0,0,255, 0 };
	cvPutText(image,text,cvPoint(top_column,top_row+12), &font, cvScalar(colour[0],colour[1],colour[2]));
}
// The following two global variables and mouse callback routine allow the developer to easily add
// a facility to allow the user to see image values for one image in a named window.
IplImage* image_for_on_mouse_show_values=NULL;
char* window_name_for_on_mouse_show_values=NULL;

void on_mouse_show_values( int event, int x, int y, int flags, void* )
{
	static IplImage* local_image = NULL;
	static int pixel_step = 0;
	static int width_step = 0;

    if (( !image_for_on_mouse_show_values ) || ( !window_name_for_on_mouse_show_values ))
        return;

	if ((local_image) && ((image_for_on_mouse_show_values->width != local_image->width) || 
						  (image_for_on_mouse_show_values->height != local_image->height)))
	{
		cvReleaseImage( &local_image );
		local_image = NULL;
	}
	if (local_image == NULL)
	{
		local_image = cvCloneImage( image_for_on_mouse_show_values );
		width_step = local_image->widthStep;
		pixel_step = local_image->widthStep/local_image->width;
	}

	if ((x < local_image->width) && (y < local_image->height))
	{
		cvCopyImage( image_for_on_mouse_show_values, local_image );
		char curr_point_text[100];
		unsigned char* curr_point = GETPIXELPTRMACRO( local_image, x, y, width_step, pixel_step );
		if (strncmp(local_image->colorModel,"RGB",3) == 0)
			sprintf(curr_point_text,"%4s = %d %d %d",local_image->colorModel,curr_point[2],curr_point[1],curr_point[0]);
		else sprintf(curr_point_text,"%4s = %d %d %d",local_image->colorModel,curr_point[0],curr_point[1],curr_point[2]);
		write_text_on_image(local_image,1,1,curr_point_text);
		sprintf(curr_point_text,"Position = %d %d",x,y);
		write_text_on_image(local_image,20,1,curr_point_text);
		cvShowImage( window_name_for_on_mouse_show_values, local_image );
	}
}
