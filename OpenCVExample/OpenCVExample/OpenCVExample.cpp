#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "pxcsensemanager.h"
#include "pxcprojection.h"
#include <iostream>
#include <stdio.h>

cv::Mat PXCImage2CVMat(PXCImage *pxcImage, PXCImage::PixelFormat format)
{
	PXCImage::ImageData data;
	pxcImage->AcquireAccess(PXCImage::ACCESS_READ, format, &data);
	int width = pxcImage->QueryInfo().width;
	int height = pxcImage->QueryInfo().height;

	if (!format)
		format = pxcImage->QueryInfo().format;

	int type;
	if (format == PXCImage::PIXEL_FORMAT_Y8)
		type = CV_8UC1;
	else if (format == PXCImage::PIXEL_FORMAT_RGB24)
		type = CV_8UC3;
	else if (format == PXCImage::PIXEL_FORMAT_DEPTH_F32)
		type = CV_32FC1;
	cv::Mat ocvImage = cv::Mat(cv::Size(width, height), type, data.planes[0]);

	pxcImage->ReleaseAccess(&data);
	return ocvImage;
}
int main(int argc, char* argv[]) {
	cv::Size frameSize = cv::Size(640, 480);
	float frameRate = 60;

	cv::namedWindow("IR", cv::WINDOW_NORMAL);
	cv::namedWindow("Color", cv::WINDOW_NORMAL);
	cv::namedWindow("Depth", cv::WINDOW_NORMAL);
	cv::Mat frameIR = cv::Mat::zeros(frameSize, CV_8UC1);
	cv::Mat frameColor = cv::Mat::zeros(frameSize, CV_8UC3);
	cv::Mat frameDepth = cv::Mat::zeros(frameSize, CV_8UC1);

	PXCSenseManager *pxcSenseManager = PXCSenseManager::CreateInstance();

	pxcSenseManager->EnableStream(PXCCapture::STREAM_TYPE_IR, frameSize.width, frameSize.height, frameRate);
	pxcSenseManager->EnableStream(PXCCapture::STREAM_TYPE_COLOR, frameSize.width, frameSize.height, frameRate);
	pxcSenseManager->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, frameSize.width, frameSize.height, frameRate);

	pxcSenseManager->Init();

	bool keepRunning = true;
	
	for (int i = 0; i < 2; i++) {
		pxcSenseManager->AcquireFrame();
		PXCCapture::Sample *sample = pxcSenseManager->QuerySample();

		frameIR = PXCImage2CVMat(sample->ir, PXCImage::PIXEL_FORMAT_Y8);
		frameColor = PXCImage2CVMat(sample->color, PXCImage::PIXEL_FORMAT_RGB24);
		PXCImage2CVMat(sample->depth, PXCImage::PIXEL_FORMAT_DEPTH_F32).convertTo(frameDepth, CV_8UC1);

		cv::Rect myROI(10, 10, 100, 100);

		//frameIR = frameIR(myROI);
		//FILE *fp = fopen("depth.txt", "w+");

		// Declare what you need
		cv::FileStorage file("some_name.ext", cv::FileStorage::WRITE);

		// Write to file!
		file << frameDepth;

	    cv::rectangle(frameIR,
			cv::Point(50, 200),
			cv::Point(480, 640),
			cv::Scalar(0, 255, 255),
			1,
			8);

		cv::imshow("IR", frameIR);
		cv::imshow("Color", frameColor);
		cv::imshow("Depth", frameDepth);

		int key = cv::waitKey(1);
		if (key == 27)
			keepRunning = false;
		pxcSenseManager->ReleaseFrame();
	}
	pxcSenseManager->Release();
	return 0;

}