#include "MHI.h"



MHI::MHI()
{
}

MHI::MHI(double MHI_DURATION_input, double MAX_TIME_DELTA_input, double MIN_TIME_DELTA_input, int N_input) {
	this->MHI_DURATION = MHI_DURATION_input;
	this->MAX_TIME_DELTA = MAX_TIME_DELTA_input;
	this->MIN_TIME_DELTA = MIN_TIME_DELTA_input;
	this->N = N_input;
}

void MHI::update_mhi(IplImage* img, int diff_threshold)
{
	//IplImage* dst = img;
	this->dst = img;
	double timestamp = (double)clock() / CLOCKS_PER_SEC;
	CvSize size = cvSize(img->width, img->height);
	int i, idx1 = last, idx2;
	IplImage* silh;
	CvSeq* seq;
	CvRect comp_rect;
	double count;
	double angle;
	CvPoint center;
	double magnitude;
	CvScalar color;

	if (!mhi || mhi->width != size.width || mhi->height != size.height) {
		if (buf == 0) {
			buf = (IplImage**)malloc(N * sizeof(buf[0]));
			memset(buf, 0, N * sizeof(buf[0]));
		}

		for (i = 0; i < N; i++) {
			cvReleaseImage(&buf[i]);
			buf[i] = cvCreateImage(size, IPL_DEPTH_8U, 1);
			cvZero(buf[i]);
		}
		cvReleaseImage(&mhi);
		cvReleaseImage(&orient);
		cvReleaseImage(&segmask);
		cvReleaseImage(&mask);

		mhi = cvCreateImage(size, IPL_DEPTH_32F, 1);
		cvZero(mhi);
		orient = cvCreateImage(size, IPL_DEPTH_32F, 1);
		segmask = cvCreateImage(size, IPL_DEPTH_32F, 1);
		mask = cvCreateImage(size, IPL_DEPTH_8U, 1);
	}

	cvCvtColor(img, buf[last], CV_BGR2GRAY);

	idx2 = (last + 1) % N;
	last = idx2;

	silh = buf[idx2];
	cvAbsDiff(buf[idx1], buf[idx2], silh);

	cvThreshold(silh, silh, diff_threshold, 1, CV_THRESH_BINARY);
	//imshow("idx1", Mat(buf[idx1]));
	//imshow("idx2", Mat(buf[idx2]));
	//imshow("silh", Mat(silh));
	cvUpdateMotionHistory(silh, mhi, timestamp, MHI_DURATION);
	cvCvtScale(mhi, mask, 255. / MHI_DURATION,
		(MHI_DURATION - timestamp)*255. / MHI_DURATION);
	cvZero(dst);
	cvCvtPlaneToPix(mask, 0, 0, 0, dst);

	cvCalcMotionGradient(mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3);

	if (!storage)
		storage = cvCreateMemStorage(0);
	else
		cvClearMemStorage(storage);

	seq = cvSegmentMotion(mhi, segmask, storage, timestamp, MAX_TIME_DELTA);

	for (i = -1; i < seq->total; i++) {

		if (i < 0) {
			comp_rect = cvRect(0, 0, size.width, size.height);
			color = CV_RGB(255, 255, 255);
			magnitude = 100;
		}
		else {
			comp_rect = ((CvConnectedComp*)cvGetSeqElem(seq, i))->rect;
			if (comp_rect.width + comp_rect.height < 100)
				continue;
			color = CV_RGB(255, 0, 0);
			magnitude = 30;
		}

		cvSetImageROI(silh, comp_rect);
		cvSetImageROI(mhi, comp_rect);
		cvSetImageROI(orient, comp_rect);
		cvSetImageROI(mask, comp_rect);

		angle = cvCalcGlobalOrientation(orient, mask, mhi, timestamp, MHI_DURATION);
		angle = 360.0 - angle;
		count = cvNorm(silh, 0, CV_L1, 0);

		cvResetImageROI(mhi);
		cvResetImageROI(orient);
		cvResetImageROI(mask);
		cvResetImageROI(silh);

		if (count < comp_rect.width*comp_rect.height * 0.05)
			continue;

		center = cvPoint((comp_rect.x + comp_rect.width / 2),
			(comp_rect.y + comp_rect.height / 2));

	}
}

cv::Mat MHI::frameMat() {
	return cv::Mat(this->dst);
}

cv::Mat MHI::blur() {
	cv::Mat mhifilter = cv::Mat(this->dst);
	erode(mhifilter, mhifilter, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2)));
	dilate(mhifilter, mhifilter, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2)));

	return mhifilter;
}

MHI::~MHI()
{
}
