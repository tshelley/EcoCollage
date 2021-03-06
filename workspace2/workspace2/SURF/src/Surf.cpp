/*
 * Surf.cpp
 *
 *  Created on: Jun 18, 2013
 *      Author: brianna
 */

//**loads video from webcam, grabs frames computes SURF keypoints and descriptors**//  //** and marks them**//
//****author: achu_wilson@rediffmail.com****//

#include <stdlib.h>
#include <stdio.h>
#include <cv.h>
#include <highgui.h>

using namespace std;
int main(int argc, char** argv)
{
  CvMemStorage* storage = cvCreateMemStorage(0);
cvNamedWindow("Image", 1);
int key = 0;
static CvScalar red_color[] ={0,0,255};
CvCapture* capture = cvCreateCameraCapture(0);
CvMat *image = 0, *gray =0;
while( key != 'q' )
{
int firstFrame = gray == 0;
IplImage* frame = cvLoadImage( "photo24bw.jpg");
if(!frame)
break;
if(!gray)
{
image = cvCreateMat(frame->height, frame->width, CV_8UC1);
}

key = cvWaitKey(1);
//Wait 50mS
int c = cvWaitKey(10);
//If 'ESC' is pressed, break the loop
if((char)c==27 ) break;

//Convert the RGB image obtained from camera into Grayscale
cvCvtColor(frame, image, CV_BGR2GRAY);
//Define sequence for storing surf keypoints and descriptors
CvSeq *imageKeypoints = 0, *imageDescriptors = 0;
int i;

//Extract SURF points by initializing parameters
CvSURFParams params = cvSURFParams(500, 1);
cvExtractSURF( image, 0, &imageKeypoints, &imageDescriptors, storage, params );
printf("Image Descriptors: %d\n", imageDescriptors->total);

//draw the keypoints on the captured frame
for( i = 0; i < imageKeypoints->total; i++ )

{
CvSURFPoint* r = (CvSURFPoint*)cvGetSeqElem( imageKeypoints, i );
CvPoint center;
int radius;
center.x = cvRound(r->pt.x);
center.y = cvRound(r->pt.y);
radius = cvRound(r->size*1.2/9.*2);
cvCircle( frame, center, radius, red_color[0], 1, 8, 0 );
}

cvShowImage( "Image", frame );
cvWaitKey(30);
}

cvDestroyWindow("Image");
return 0;

}
