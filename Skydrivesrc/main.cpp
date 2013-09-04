/*
 * main.cpp
 *
 *  Created on: Jun 18, 2013
 *      Author: brianna
 */

#include "main.h"
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include <stdlib.h>
#include <stdio.h>
#include "opencv/highgui.h"
#include "opencv/cv.h"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;

class Symbol {
public:
	Symbol(IplImage* img, int x, int y);
	void print();
	Symbol();
	Symbol(const Symbol& other);
	//	bool operator < (Symbol& other);
	IplImage* img;
	int x;
	int y;
};

Symbol::Symbol(IplImage* img, int x, int y) {
	this->x = x;
	this->y = y;
	this->img = img;
}

Symbol::Symbol(const Symbol &other){
	this->x = other.x;
	this->y = other.y;
	this->img = cvCreateImage(cvGetSize(other.img), other.img->depth, other.img->nChannels);
	cvCopy(other.img,this->img, NULL);
}

//bool operator < (Symbol& first, Symbol& second){
//	return(first.x < second.x);
//}


std::vector<Symbol> symbolList[5];
std::vector<IplImage*> imageList;

Mat prevFrame[5];

//Looks for a specific color on screen and returns thresholded image
IplImage * GetThresholdedImage(IplImage * img)
{
	//Convert the image into an HSV image
	IplImage * imgHSV = cvCreateImage(cvGetSize(img), 8, 3);

	cvCvtColor(img, imgHSV, CV_BGR2HSV);


	//New image that will hold the thresholded image
	IplImage * imgThreshed = cvCreateImage(cvGetSize(img), 8, 1);

	//	cvInRangeS(imgHSV, cvScalar(40, 100, 100), cvScalar(80, 255, 255), imgThreshed);
	//blue
	//cvInRangeS(imgHSV, cvScalar(100, 150, 100), cvScalar(140, 255, 255), imgThreshed);
	//vaguely wood colored
	cvInRangeS(imgHSV, cvScalar(0, 0, 0), cvScalar(360, 60, 50), imgThreshed);

	//Release the temp HSV image and return this thresholded image
	cvReleaseImage(&imgHSV);
	return imgThreshed;
}

//Looks for contours on image and creates a box around it
//Stores values into array and subimage (region of interest)
void DetectAndDrawQuads(IplImage * img, IplImage * original, int frameNumber)
{
	CvSeq * contours;
	CvSeq * result;
	CvMemStorage * storage = cvCreateMemStorage(0);
	IplImage * ret = cvCreateImage(cvGetSize(img), 8, 3);
	IplImage * temp = cvCloneImage(img);

	symbolList[frameNumber].clear();
	int count = 0;
	cvFindContours(temp, storage, &contours, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0));
	while(contours)
	{

		result = cvApproxPoly(contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, cvContourPerimeter(contours) * 0.02, 0);
		if(result->total==4 && fabs(cvContourArea(result, CV_WHOLE_SEQ)) > 20)
		{
			count++;
			CvPoint * pt[4];
			for(int i=0;i < 4; i++)
				pt[i] = (CvPoint * )cvGetSeqElem(result, i);

			cvLine(ret, * pt[0], * pt[1], cvScalar(255));
			cvLine(ret, * pt[1], * pt[2], cvScalar(255));
			cvLine(ret, * pt[2], * pt[3], cvScalar(255));
			cvLine(ret, * pt[3], * pt[0], cvScalar(255));
			int x = 0;
			if (pt[2]->x< pt[0]->x)
			{
				x = pt[2]->x;
			}else{
				x = pt[0]->x;
			}
			int y = pt[0]->y;

			int w = abs(pt[2]->x - pt[0]->x);
			int h = abs(pt[2]->y - pt[0]->y);
			cvResetImageROI(img);
			char *windowName = new char[20];
			sprintf(windowName, "Detected Object %d ", count);
			printf("%s\n", windowName);
			cvDestroyWindow(windowName);
			//printf("x:  %d, y:  %d, w:  %d, h:  %d ", x, y, w, h);

			cvSetImageROI(original, cvRect(x-10,y-10, w+20, h+20));
			IplImage * detectedObject = cvCreateImage(cvGetSize(original),original->depth, original->nChannels);
			cvCopy(original, detectedObject, NULL);
			symbolList[frameNumber].push_back(Symbol(detectedObject, x, y));
			imageList.push_back(detectedObject);
			cvNamedWindow(windowName, CV_WINDOW_AUTOSIZE);
			cvShowImage(windowName, detectedObject);

			cvReleaseImage(&detectedObject);
		}
		contours = contours -> h_next;
	}

	cvReleaseImage(&temp);
	cvReleaseMemStorage(&storage);
}

//compares the positions of the symbols in symbolList across the five frames
//sampled, and if they are the same, creates an array of the same images to be looked at during the match phase later.
//void checkForMatches(){
//	for(int i = 0 ; i < 5 ; i++){
//		sort(symbolList[i].begin(), symbolList[i].end());
//	}
//}

//Takes Mat object and finds its keypoints, then compares against the keypoints in segmentedCapture
//If there are 4 or more matching keypoints, then it reports a match
bool match(Mat object, IplImage* segmentedCapture, int i)
{
	printf("Size check of segmented capture: height: %d, width: %d\n", segmentedCapture->height, segmentedCapture->width);
	printf("attempting to read object now\n");

	bool matchFound = false;
	if( !object.data )
	{
		std::cout<< "Error reading object " << std::endl;
		return -1;
	}
	int minHessian = 500;

	SurfFeatureDetector detector(minHessian);
	//Detect the keypoints using SURF Detector

	std::vector<KeyPoint> kp_object;
	detector.detect( object, kp_object );

	//Calculate descriptors (feature vectors)
	Mat des_object;
	SurfDescriptorExtractor extractor;

	extractor.compute( object, kp_object, des_object );
	printf("Number of descriptors found for initial object: %d\n", (int)kp_object.size());

	FlannBasedMatcher matcher;

	char *windowName = new char[20];
	sprintf(windowName, "Match %d", i);
	destroyWindow(windowName);
	namedWindow(windowName);

	std::vector<Point2f> obj_corners(4);
	obj_corners[0] = cvPoint(0,0);
	obj_corners[1] = cvPoint( object.cols, 0 );
	obj_corners[2] = cvPoint( object.cols, object.rows );
	obj_corners[3] = cvPoint( 0, object.rows );

	Mat des_image, img_matches;
	std::vector<KeyPoint> kp_image;
	std::vector<vector<DMatch > > matches;
	std::vector<DMatch > good_matches;
	std::vector<Point2f> obj;
	std::vector<Point2f> scene;
	std::vector<Point2f> scene_corners(4);
	Mat H;
	Mat image;


	cvResetImageROI(segmentedCapture);
	printf("creating image to store it in");
	//	IplImage *image2 = cvCreateImage(cvSize(segmentedCapture->width, segmentedCapture->height), IPL_DEPTH_8U,1);
	printf("about to convert to gray\n");
	//	cvCvtColor(segmentedCapture, image2, CV_BGR2GRAY);
	//
	//	printf("converted to gray\n");
	Mat matCon(segmentedCapture);
	image = segmentedCapture;
	//	printf("before detection\n");
	detector.detect( image, kp_image );
	//	printf("after detection, number of descriptors for detected object: %d\n", kp_image.size());
	extractor.compute( image, kp_image, des_image );
	//	printf("after computation  of extraction\n");

	if(des_image.empty()){
		printf("key points from capture frame are empty\n");
	} else {

		matcher.knnMatch(des_object, des_image, matches, 2);
		//		matcher.match(des_object, des_image, matches);
		printf("after knnmatch: matches.size() is %d\n", matches.size());
		for(int j = 0; j < min(des_image.rows-1,(int) matches.size()); j++) //THIS LOOP IS SENSITIVE TO SEGFAULTS
		{
			if((matches[j][0].distance < 0.5*(matches[j][1].distance)) && ((int) matches[j].size()<=2 && (int) matches[j].size()>0))
			{
				good_matches.push_back(matches[j][0]);
				//			printf("Outer loop is on: %d, Number of matches is: %d\n", i, (int)good_matches.size());
			}
		}

		//Draw only "good" matches
		drawMatches( object, kp_object, image, kp_image, good_matches, img_matches, Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

		if (good_matches.size() >= 4)
		{
			matchFound = true;
			printf("Found %d matched points for detectedObject %d", good_matches.size(), i );
			for( int i = 0; i < good_matches.size(); i++ )
			{
				//Get the keypoints from the good matches
				obj.push_back( kp_object[ good_matches[i].queryIdx ].pt );
				scene.push_back( kp_image[ good_matches[i].trainIdx ].pt );
			}

			H = findHomography( obj, scene, CV_RANSAC );

			perspectiveTransform( obj_corners, scene_corners, H);

			//Draw lines between the corners (the mapped object in the scene image )
			line( img_matches, scene_corners[0] + Point2f( object.cols, 0), scene_corners[1] + Point2f( object.cols, 0), Scalar(0, 255, 0), 4 );
			line( img_matches, scene_corners[1] + Point2f( object.cols, 0), scene_corners[2] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );
			line( img_matches, scene_corners[2] + Point2f( object.cols, 0), scene_corners[3] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );
			line( img_matches, scene_corners[3] + Point2f( object.cols, 0), scene_corners[0] + Point2f( object.cols, 0), Scalar( 0, 255, 0), 4 );
		}
		imshow( windowName, img_matches );

	}
	return matchFound;
}

int main()
{


	Mat object = imread( "photo14.jpg", CV_LOAD_IMAGE_GRAYSCALE );

	if( !object.data )
	{
		std::cout<< "Error reading object " << std::endl;
		return -1;
	}



	VideoCapture cap(1);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 1020);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 675);


	//namedWindow("Good Matches");
	namedWindow("Capture");

	std::vector<Point2f> obj_corners(4);

	//Get the corners from the object
	obj_corners[0] = cvPoint(0,0);
	obj_corners[1] = cvPoint( object.cols, 0 );
	obj_corners[2] = cvPoint( object.cols, object.rows );
	obj_corners[3] = cvPoint( 0, object.rows );

	char key = 'a';

	while (key != 27)
	{
		int c = cvWaitKey(100);
		Mat frame;
		cap >> frame;
		imshow("Capture", frame);


		if( c == 32){
			printf("spacebar pressed");
			for(int frameNumber = 0; frameNumber < 5; frameNumber++){
				IplImage * imgThresh = GetThresholdedImage(cvCloneImage(&(IplImage)frame));
				DetectAndDrawQuads(imgThresh, cvCloneImage(&(IplImage)frame), frameNumber);
			}
			int i = 1;
			for(std::vector<Symbol>::iterator it = symbolList[0].begin() ; it != symbolList[0].end(); ++it){
				printf("Image %d found at x : %d y : %d , size of image is: %d, looking for matches now. \n", i, it->x, it->y,  it->img->imageSize);
				IplImage *doubleSized = cvCreateImage(cvSize((int) (it->img->width * 12 ), (int) (it->img->height * 12)), it->img->depth, it->img->nChannels);
				//increase size of read image
				cvResize(it->img, doubleSized);
				IplImage *grayScale = cvCreateImage( cvSize(doubleSized->width, doubleSized->height), doubleSized->depth, 1);
				cvCvtColor(doubleSized, grayScale, CV_BGR2GRAY);
				threshold(Mat(grayScale), Mat(grayScale), 120, 255,  THRESH_BINARY);
				//				cvNamedWindow("Original", CV_WINDOW_AUTOSIZE);
				//				cvShowImage("Original", grayScale);
				if(match(object, grayScale, i )){
					printf("\nMatch found at x : %d y : %d. \nHeight of match:  %d\n", it->x, it->y, it->img->height);
					waitKey(3000);
				}
				i++;
			}
			for(int frameNumber = 0; frameNumber < 5; frameNumber++){
				if(frameNumber == 4) prevFrame[frameNumber] = frame;
				else prevFrame[frameNumber] = prevFrame[frameNumber+1];
			}
		}
		if((char)c==27 ) break;
	}
	return 0;
}
