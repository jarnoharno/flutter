// From lkpyramid.cpp

#include "registration.h"
#include <opencv2/opencv.hpp>
#include <cstring>

using namespace cv;

static void
get_rt_matrix(const CvPoint2D32f* a, const CvPoint2D32f* b,
	int count, CvMat* M)
{

	double sa[16], sb[4], m[4], *om = M->data.db;
	CvMat A = cvMat(4, 4, CV_64F, sa), B = cvMat(4, 1, CV_64F, sb);
	CvMat MM = cvMat(4, 1, CV_64F, m);

	int i;

	memset(sa, 0, sizeof(sa));
	memset(sb, 0, sizeof(sb));

	for (i = 0; i < count; i++) {
		sa[0] += a[i].x*a[i].x + a[i].y*a[i].y;
		sa[1] += 0;
		sa[2] += a[i].x;
		sa[3] += a[i].y;

		sa[4] += 0;
		sa[5] += a[i].x*a[i].x + a[i].y*a[i].y;
		sa[6] += -a[i].y;
		sa[7] += a[i].x;

		sa[8] += a[i].x;
		sa[9] += -a[i].y;
		sa[10] += 1;
		sa[11] += 0;

		sa[12] += a[i].y;
		sa[13] += a[i].x;
		sa[14] += 0;
		sa[15] += 1;

		sb[0] += a[i].x*b[i].x + a[i].y*b[i].y;
		sb[1] += a[i].x*b[i].y - a[i].y*b[i].x;
		sb[2] += b[i].x;
		sb[3] += b[i].y;
	}

	cvSolve(&A, &B, &MM, CV_SVD);

	om[0] = om[4] = m[0];
	om[1] = -m[1];
	om[3] = m[1];
	om[2] = m[2];
	om[5] = m[3];
}

static int estimate_rigid_transform_detail(const CvArr* matA, const CvArr* matB,
	CvMat* matM)
{
	const int COUNT = 15;
	const int WIDTH = 160, HEIGHT = 120;
	const int RANSAC_MAX_ITERS = 500;
	const int RANSAC_SIZE0 = 3;
	const double RANSAC_GOOD_RATIO = 0.5;

	cv::Ptr<CvMat> sA, sB;
	cv::AutoBuffer<CvPoint2D32f> pA, pB;
	cv::AutoBuffer<int> good_idx;
	cv::AutoBuffer<char> status;
	cv::Ptr<CvMat> gray;

	CvMat stubA, *A = cvGetMat(matA, &stubA);
	CvMat stubB, *B = cvGetMat(matB, &stubB);
	CvSize sz0, sz1;
	int cn, equal_sizes;
	int i, j, k, k1;
	int count_x, count_y, count = 0;
	double scale = 1;
	CvRNG rng = cvRNG(-1);
	double m[6]= {0};
	CvMat M = cvMat(2, 3, CV_64F, m);
	int good_count = 0;
	CvRect brect;

	if (!CV_IS_MAT(matM)) {
		CV_Error(matM ? CV_StsBadArg : CV_StsNullPtr, "Output parameter M is not a valid matrix");
	}

	if (!CV_ARE_SIZES_EQ(A, B)) {
		CV_Error(CV_StsUnmatchedSizes, "Both input images must have the same size");
	}

	if (!CV_ARE_TYPES_EQ(A, B)) {
		CV_Error(CV_StsUnmatchedFormats, "Both input images must have the same data type");
	}

	if (CV_MAT_TYPE(A->type) == CV_8UC1 || CV_MAT_TYPE(A->type) == CV_8UC3) {
		cn = CV_MAT_CN(A->type);
		sz0 = cvGetSize(A);
		sz1 = cvSize(WIDTH, HEIGHT);

		scale = MAX((double)sz1.width/sz0.width, (double)sz1.height/sz0.height);
		scale = MIN(scale, 1.);
		sz1.width = cvRound(sz0.width * scale);
		sz1.height = cvRound(sz0.height * scale);

		equal_sizes = sz1.width == sz0.width && sz1.height == sz0.height;

		if (!equal_sizes || cn != 1) {
			sA = cvCreateMat(sz1.height, sz1.width, CV_8UC1);
			sB = cvCreateMat(sz1.height, sz1.width, CV_8UC1);

			if (cn != 1) {
				gray = cvCreateMat(sz0.height, sz0.width, CV_8UC1);
				cvCvtColor(A, gray, CV_BGR2GRAY);
				cvResize(gray, sA, CV_INTER_AREA);
				cvCvtColor(B, gray, CV_BGR2GRAY);
				cvResize(gray, sB, CV_INTER_AREA);
				gray.release();
			} else {
				cvResize(A, sA, CV_INTER_AREA);
				cvResize(B, sB, CV_INTER_AREA);
			}

			A = sA;
			B = sB;
		}

		count_y = COUNT;
		count_x = cvRound((double)COUNT*sz1.width/sz1.height);
		count = count_x * count_y;

		pA.allocate(count);
		pB.allocate(count);
		status.allocate(count);

		for (i = 0, k = 0; i < count_y; i++)
			for (j = 0; j < count_x; j++, k++) {
				pA[k].x = (j+0.5f)*sz1.width/count_x;
				pA[k].y = (i+0.5f)*sz1.height/count_y;
			}

		// find the corresponding points in B
		cvCalcOpticalFlowPyrLK(A, B, 0, 0, pA, pB, count, cvSize(10,10), 3,
		                       status, 0, cvTermCriteria(CV_TERMCRIT_ITER,40,0.1), 0);

		// repack the remained points
		for (i = 0, k = 0; i < count; i++)
			if (status[i]) {
				if (i > k) {
					pA[k] = pA[i];
					pB[k] = pB[i];
				}
				k++;
			}

		count = k;
	} else if (CV_MAT_TYPE(A->type) == CV_32FC2 || CV_MAT_TYPE(A->type) == CV_32SC2) {
		count = A->cols*A->rows;
		CvMat _pA, _pB;
		pA.allocate(count);
		pB.allocate(count);
		_pA = cvMat(A->rows, A->cols, CV_32FC2, pA);
		_pB = cvMat(B->rows, B->cols, CV_32FC2, pB);
		cvConvert(A, &_pA);
		cvConvert(B, &_pB);
	} else {
		CV_Error(CV_StsUnsupportedFormat, "Both input images must have either 8uC1 or 8uC3 type");
	}

	good_idx.allocate(count);

	if (count < RANSAC_SIZE0) {
		return 0;
	}

	CvMat _pB = cvMat(1, count, CV_32FC2, pB);
	brect = cvBoundingRect(&_pB, 1);

	// RANSAC stuff:
	// 1. find the consensus
	for (k = 0; k < RANSAC_MAX_ITERS; k++) {
		int idx[RANSAC_SIZE0];
		CvPoint2D32f a[3];
		CvPoint2D32f b[3];

		memset(a, 0, sizeof(a));
		memset(b, 0, sizeof(b));

		// choose random 3 non-complanar points from A & B
		for (i = 0; i < RANSAC_SIZE0; i++) {
			for (k1 = 0; k1 < RANSAC_MAX_ITERS; k1++) {
				idx[i] = cvRandInt(&rng) % count;

				for (j = 0; j < i; j++) {
					if (idx[j] == idx[i]) {
						break;
					}
					// check that the points are not very close one each other
					if (fabs(pA[idx[i]].x - pA[idx[j]].x) +
					                fabs(pA[idx[i]].y - pA[idx[j]].y) < FLT_EPSILON) {
						break;
					}
					if (fabs(pB[idx[i]].x - pB[idx[j]].x) +
					                fabs(pB[idx[i]].y - pB[idx[j]].y) < FLT_EPSILON) {
						break;
					}
				}

				if (j < i) {
					continue;
				}

				if (i+1 == RANSAC_SIZE0) {
					// additional check for non-complanar vectors
					a[0] = pA[idx[0]];
					a[1] = pA[idx[1]];
					a[2] = pA[idx[2]];

					b[0] = pB[idx[0]];
					b[1] = pB[idx[1]];
					b[2] = pB[idx[2]];

					double dax1 = a[1].x - a[0].x, day1 = a[1].y - a[0].y;
					double dax2 = a[2].x - a[0].x, day2 = a[2].y - a[0].y;
					double dbx1 = b[1].x - b[0].x, dby1 = b[1].y - b[0].y;
					double dbx2 = b[2].x - b[0].x, dby2 = b[2].y - b[0].y;
					const double eps = 0.01;

					if (fabs(dax1*day2 - day1*dax2) < eps*sqrt(dax1*dax1+day1*day1)*sqrt(dax2*dax2+day2*day2) ||
					                fabs(dbx1*dby2 - dby1*dbx2) < eps*sqrt(dbx1*dbx1+dby1*dby1)*sqrt(dbx2*dbx2+dby2*dby2)) {
						continue;
					}
				}
				break;
			}

			if (k1 >= RANSAC_MAX_ITERS) {
				break;
			}
		}

		if (i < RANSAC_SIZE0) {
			continue;
		}

		// estimate the transformation using 3 points
		get_rt_matrix(a, b, 3, &M);

		for (i = 0, good_count = 0; i < count; i++) {
			if (fabs(m[0]*pA[i].x + m[1]*pA[i].y + m[2] - pB[i].x) +
			                fabs(m[3]*pA[i].x + m[4]*pA[i].y + m[5] - pB[i].y) < MAX(brect.width,brect.height)*0.05) {
				good_idx[good_count++] = i;
			}
		}

		if (good_count >= count*RANSAC_GOOD_RATIO) {
			break;
		}
	}

	if (k >= RANSAC_MAX_ITERS) {
		return 0;
	}

	if (good_count < count) {
		for (i = 0; i < good_count; i++) {
			j = good_idx[i];
			pA[i] = pA[j];
			pB[i] = pB[j];
		}
	}

	get_rt_matrix(pA, pB, good_count, &M);
	m[2] /= scale;
	m[5] /= scale;
	cvConvert(&M, matM);

	return 1;
}

cv::Mat flutter::estimate_rigid_transform(cv::InputArray src1, cv::InputArray src2)
{
	Mat M(2, 3, CV_64F), A = src1.getMat(), B = src2.getMat();
	CvMat matA = A, matB = B, matM = M;
	int err = estimate_rigid_transform_detail(&matA, &matB, &matM);
	if (err == 1) {
		return M;
	} else {
		return Mat();
	}
}
