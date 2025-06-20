#include "matcher.h"
#include <windows.h>
#include <gdiplus.h>
#include <opencv2/opencv.hpp>
#include <string>
using namespace Gdiplus;

// Forward declare Bitmap if needed
// class Bitmap;

// Helper to convert Gdiplus::Bitmap to cv::Mat (grayscale)
cv::Mat BitmapToGrayMat(Gdiplus::Bitmap* bmp) {
    int width = bmp->GetWidth();
    int height = bmp->GetHeight();
    cv::Mat mat(height, width, CV_8UC4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Gdiplus::Color color;
            bmp->GetPixel(x, y, &color);
            mat.at<cv::Vec4b>(y, x) = cv::Vec4b(color.GetBlue(), color.GetGreen(), color.GetRed(), color.GetAlpha());
        }
    }
    cv::Mat gray;
    cv::cvtColor(mat, gray, cv::COLOR_BGRA2GRAY);
    return gray;
}

bool matchImage(Gdiplus::Bitmap* bmp, Gdiplus::Bitmap* templ) {
    if (!bmp || !templ) return false;
    cv::Mat input_gray = BitmapToGrayMat(bmp);
    cv::Mat templ_gray = BitmapToGrayMat(templ);
    if (input_gray.cols < templ_gray.cols || input_gray.rows < templ_gray.rows) return false;
    cv::Mat result;
    cv::matchTemplate(input_gray, templ_gray, result, cv::TM_CCOEFF_NORMED);
    double minVal, maxVal;
    cv::minMaxLoc(result, &minVal, &maxVal);
    return maxVal >= 0.7;
}
