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
	int x0 = 180;
	int y0 = 180;
	int W = 270;
	int H = 120;

	cv::namedWindow("IR", cv::WINDOW_NORMAL);
	cv::namedWindow("StdDev", cv::WINDOW_NORMAL);
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
	
	cv::Mat* depthArray = new cv::Mat[640*480];
	cv::Mat MStddev8U_C1(cv::Size(W, H), CV_8UC1);

	for (int i = 0;i < 300;i++) {
	//while (pxcSenseManager->AcquireFrame() >= PXC_STATUS_NO_ERROR);
		pxcSenseManager->AcquireFrame();
		PXCCapture::Sample *sample = pxcSenseManager->QuerySample();

		frameIR = PXCImage2CVMat(sample->ir, PXCImage::PIXEL_FORMAT_Y8);
		frameColor = PXCImage2CVMat(sample->color, PXCImage::PIXEL_FORMAT_RGB24);
		PXCImage2CVMat(sample->depth, PXCImage::PIXEL_FORMAT_DEPTH_F32).convertTo(frameDepth, CV_8UC1);

		cv::Mat croppedDepth;
		croppedDepth = frameDepth(cv::Rect(x0, y0, W, H)).clone();
		
		for (int j = 0; j < croppedDepth.rows; j++) {
			for (int k = 0; k < croppedDepth.cols; k++) {
				if (depthArray[j * W + k].size().height >= 300) continue;//depthArray[j * W + k].pop_back();
				depthArray[j * W + k].push_back<uchar>(static_cast<uchar>(frameDepth.at<uchar>(j, k)));
			}
		}

		cv::Scalar mean, stddev;
		cv::Mat MStddev(cv::Size(W, H), CV_32FC1);
		
		
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
				MStddev8U_C1.at<uchar>(j, k) = static_cast<uchar>(255 * MStddev.at<float>(j,k)/ maxVal);
			}
		}
		cv::imshow("StdDev", MStddev8U_C1);

		// Declare what you need
		
	    cv::rectangle(frameIR,
			cv::Point(180, 180),
			cv::Point(450, 300),
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

	cv::FileStorage fs("test.yml", cv::FileStorage::WRITE);
	fs << MStddev8U_C1;


	//std::cout << depthArray[1];
	
	/*
	cv::Scalar mean, stddev;
	cv::Mat MStddev(cv::Size(640, 480), CV_32FC1);
	for (int j = 0; j < frameDepth.rows; j++) {
		for (int k = 0; k < frameDepth.cols; k++) {
			//std::cout << depthArray[j * 640 + k].size() << std::endl;
			cv::meanStdDev(depthArray[j * 640 + k], mean, stddev);
			MStddev.at<float>(j, k) = stddev.val[0];
			//std::cout << stddev.val[0] << std::endl;
		}
	}
	
	cv::Scalar MeanSdv, Sdv;
	cv::meanStdDev(MStddev, MeanSdv, Sdv);
	std::cout << MeanSdv << "," << Sdv << std::endl;
	*/
	
	cv::waitKey(0);
	pxcSenseManager->Release();
	return 0;

}



void histEq() {
 /// Establish the number of bins
	int histSize = 256;

		  /// Set the ranges ( for B,G,R) )
	float range[] = { 0, 256 } ;
	const float* histRange = { range };

	bool uniform = true; bool accumulate = false;

	cv::Mat hist;

		  /// Compute the histograms:
	//calcHist( &bgr_planes[0], 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate );
		  

		  // Draw the histograms for B, G and R
	int hist_w = 512; int hist_h = 400;
	int bin_w = cvRound( (double) hist_w/histSize );

	cv::Mat histImage( hist_h, hist_w, CV_8UC3, cv::Scalar( 0,0,0) );

		  /// Normalize the result to [ 0, histImage.rows ]
	normalize(hist, hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat() );
		  

		  /// Draw for each channel
	for( int i = 1; i < histSize; i++ )
	{
		line( histImage, cv::Point( bin_w*(i-1), hist_h - cvRound(hist.at<float>(i-1)) ) ,
						 cv::Point( bin_w*(i), hist_h - cvRound(hist.at<float>(i)) ),
						 cv::Scalar( 255, 0, 0), 2, 8, 0  );
	}

		  /// Display
	cv::namedWindow("calcHist Demo", CV_WINDOW_AUTOSIZE );
	imshow("calcHist Demo", histImage );

	cv::waitKey(0);
}