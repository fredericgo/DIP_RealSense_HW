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
	/* Set up the rectangle box you want to crop here*/
	int x0 = 295;
	int y0 = 255;
	int W = 110;
	int H = 55;

	cv::namedWindow("IR", cv::WINDOW_NORMAL);
	cv::namedWindow("StdDev", cv::WINDOW_NORMAL);
	//cv::namedWindow("Color", cv::WINDOW_NORMAL);
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
	
	cv::Mat* depthArray = new cv::Mat[640*480];
	cv::Mat MStddev8U_C1(cv::Size(W, H), CV_8UC1);
	int i = 0;
	while(true) {
	//while (pxcSenseManager->AcquireFrame() >= PXC_STATUS_NO_ERROR);
		pxcStatus sts = pxcSenseManager->AcquireFrame(true);
		if (sts<PXC_STATUS_NO_ERROR) break;
		if (i > 300) 
			break;
		std::cout << "picture " << i << std::endl;
		i++;
		PXCCapture::Sample *sample = pxcSenseManager->QuerySample();

		frameIR = PXCImage2CVMat(sample->ir, PXCImage::PIXEL_FORMAT_Y8);
		//frameColor = PXCImage2CVMat(sample->color, PXCImage::PIXEL_FORMAT_RGB24);
		frameDepth = PXCImage2CVMat(sample->depth, PXCImage::PIXEL_FORMAT_DEPTH_F32);//.convertTo(frameDepth, CV_8UC1);

		cv::Mat croppedDepth;
		croppedDepth = frameDepth(cv::Rect(x0, y0, W, H)).clone();
		
		//std::cout << "depth: " << croppedDepth.at<float>(30, 30) << std::endl;

		for (int j = 0; j < croppedDepth.rows; j++) {
			for (int k = 0; k < croppedDepth.cols; k++) {
				if (depthArray[j * W + k].size().height >= 300) continue;//depthArray[j * W + k].pop_back();
				depthArray[j * W + k].push_back<float>(croppedDepth.at<float>(j, k));
			}
		}

		cv::Scalar mean, stddev;
		cv::Mat MStddev(cv::Size(W, H), CV_32FC1);
		
		//std::cout << depthArray[0] << std::endl;
		for (int j = 0; j < croppedDepth.rows; j++) {
			for (int k = 0; k < croppedDepth.cols; k++) {
				//std::cout << depthArray[j * 640 + k].size() << std::endl;
				cv::meanStdDev(depthArray[j * W + k], mean, stddev);
				MStddev.at<float>(j, k) = stddev.val[0];
				//std::cout << stddev.val[0] << std::endl;
			}
		}

		cv::Scalar MeanSdv, Sdv;
		cv::meanStdDev(MStddev, MeanSdv, Sdv);
		std::cout << MeanSdv.val[0] << "," << Sdv.val[0] << std::endl;

		double minVal;
		double maxVal;
		cv::Point minLoc;
		cv::Point maxLoc;

		minMaxLoc(MStddev, &minVal, &maxVal, &minLoc, &maxLoc);
		std::cout << "max: " << maxVal << std::endl;
		

		for (int j = 0; j < croppedDepth.rows; j++) {
			for (int k = 0; k < croppedDepth.cols; k++) {
				MStddev8U_C1.at<uchar>(j, k) = static_cast<uchar>(255 * MStddev.at<float>(j,k)/ 2.55);
			}
		}
		cv::imshow("StdDev", MStddev8U_C1);
		
	    cv::rectangle(frameIR,
			cv::Point(x0, y0),
			cv::Point(x0+W,y0+H),
			cv::Scalar(0, 255, 255),
			1,
			8);
		

		cv::imshow("IR", frameIR);
		//cv::imshow("Color", frameColor);
		cv::imshow("Depth", croppedDepth);

		int key = cv::waitKey(1);
		if (key == 27)
			keepRunning = false;
		pxcSenseManager->ReleaseFrame();
	}

	std::cout << "writing file." << std::endl;
	cv::FileStorage fs("80.yml", cv::FileStorage::WRITE);
	fs << "img" << MStddev8U_C1;

	//cv::waitKey(0);
	pxcSenseManager->Release();
	return 0;

}