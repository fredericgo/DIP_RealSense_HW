#pragma once
#include "pxcsensemanager.h"
#include "pxcprojection.h"
#include "opencv2/opencv.hpp"
#include <opencv2/highgui/highgui.hpp>

class RealSense
{
public:
	RealSense();
	bool StartVideoStream();
	void ShowContinuousImages();
	void PXCColorImg2CVColorImg(PXCImage* srcimg, cv::Mat &mColorImg);
	~RealSense();
private:
	const int IMAGE_WIDTH = 640;
	const int IMAGE_HEIGHT = 480;
	PXCSenseManager* mPXCSenseManager;
};

