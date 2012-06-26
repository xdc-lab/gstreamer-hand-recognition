//============================================================================
// Name        : opencv_handdetect.cpp
// Author      : andol li
// Version     : 0.10
// Copyright   : 2012
// Description : using haartraining results to detect the hand gesture of FIST in video stream.
//				 this is the base for the gstreamer plugin dev for real-time media operation using natural hand gestures.
//============================================================================

#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>

using namespace cv;
using namespace std;

const double scale = 1.1;
BackgroundSubtractorMOG2 bg_model;

//define the core function
void detectAndDraw(Mat& img, CascadeClassifier& cascade, double scale);

//define the path to cascade file
string cascadeName =
	"fist.xml"; /*ROBUST-fist detection haartraining file*/
//	"./handFistHAAR.xml"; /*under dev-hand fist*/
//	"./handgesturehaar.xml"; /*under dev-hand contract palm*/
//	"./handgesturehaar2.xml"; /*under dev-hand open palm*/
//	"./haartraining3.xml"; /*under dev-black&white hand open palm*/
//	"./pos_palm2_haartraining.xml"; /*under dev-the 2nd hand open palm*/

int main()
{
//	define images
	Mat image, fgmask, fgimg, ssrc;
//	define cascade classifier for later detection
	CascadeClassifier cascade, nestedCascade;
//		load cascade file, and check if successed
		if( ! cascade.load(cascadeName))
		{
			cerr << "ERROR: could not load cascade file. \n please check the file name and directory." << endl;
			return -1;
		}

//	define video stream capture device
	VideoCapture cap(0);
//	request a single image from camera
	cap >> image;

//	define key value for event listenning, start main loop
	int c = 0;
	while( c != 27)
	{
//		request a image
		cap >> image;
//		resize the image for better performance, image -> ssrc
		if(image.rows > 240 || image.cols > 320)
		resize(image, ssrc, Size(320,240), 0.5, 0.5, 1);

		/*under dev-test code-dynamic foreground extraction*/
//		bg_model(ssrc, fgmask, -1);
//		dilate(fgmask, fgmask, Mat(), Point(-1, -1), 2, 0, 1);
//		blur(fgmask, fgmask, Size(8,8), Point(0,0), 1);
//		fgimg = Scalar::all(0);
//		ssrc.copyTo(fgimg, fgmask); imshow("000", fgimg);
		/**/

//		detect hands from image
		if(! ssrc.empty())
		{
			detectAndDraw( ssrc, cascade, scale);
		}

//		key event listening
		c = waitKey(10);
	}

//
	return 0;
}

void detectAndDraw(Mat& img, CascadeClassifier& cascade, double scale)
{
	int i = 0;
	double t = 0;
	vector<Rect> hands;

	Mat gray, smallImg(cvRound (img.rows / scale), cvRound(img.cols / scale), CV_8UC1);

	cvtColor(img, gray, CV_BGR2GRAY);
	resize(gray, smallImg, smallImg.size(), 0, 0, INTER_LINEAR);
	equalizeHist(smallImg, smallImg);

	t = (double) getTickCount();

//	detect hands
	cascade.detectMultiScale( smallImg,
		hands,
		scale, /*change this to adjust detection performance-accuracy of hand sizes*/
		3,
		0,
		Size(40, 30) );

	t = (double) getTickCount() - t;
	printf("detect time: %g ms\n", t / ((double)getTickFrequency() ) * 1000. );

//	loop to get hands, draw circles on the hands
	for( vector<Rect>::const_iterator r = hands.begin(); r != hands.end(); r++, i++)
	{
		Mat smallImgROI;
		vector<Rect> nestedObjects;
		Point center;
		Scalar color = CV_RGB(200, 0, 0);
		int radius;
		center.x = cvRound((r->x + r->width * 0.5) * scale);
		center.y = cvRound((r->y + r->height*0.5)*scale);
		radius = cvRound((r->width + r->height)*0.25*scale);
		circle( img, center, radius, color, 1, 8, 0 );
		smallImgROI = smallImg(*r);
	}
	imshow("result", img);
}
