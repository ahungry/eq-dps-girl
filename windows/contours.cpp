#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>

using namespace cv;
using namespace std;

int main() {
    // 1. Load the PNG image
    // Mat image = imread("girl1.png", IMREAD_UNCHANGED); // Load with alpha channel
    Mat image = imread("../bubble200.png", IMREAD_UNCHANGED); // Load with alpha channel

    if (image.empty()) {
        cout << "Could not open or find the image" << endl;
        return -1;
    }

    // 2. Extract the alpha channel (transparency)
    Mat alphaChannel;
    vector<Mat> channels;
    split(image, channels); // Split into B, G, R, A channels

    if (channels.size() == 4) {
        alphaChannel = channels[3]; // Get the alpha channel
    }
    else
    {
         cout << "Image does not have alpha channel" << endl;
         return -1;
    }

    // 3. Find contours using OpenCV (this does the tracing)
    vector<vector<Point>> contours;
    findContours(alphaChannel, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 4. Process the contours (get the points)
    if (contours.size() > 0) {
        // Assuming you want the largest contour (the character outline)
        vector<Point> largestContour = contours[0]; // Or find the largest one

        // 5. Output the points in a format suitable for CreatePolyPolygonRgn
        cout << "POINT points[] = {";
        for (size_t i = 0; i < largestContour.size(); ++i) {
            cout << "{" << largestContour[i].x << ", " << largestContour[i].y << "}";
            if (i < largestContour.size() - 1) {
                cout << ", ";
            }
        }
        cout << "};" << endl;
        cout << "int numPoints = " << largestContour.size() << ";" << endl;

         //Example of calling the windows API
         //HRGN hRgn = CreatePolygonRgn(&largestContour[0], largestContour.size(), ALTERNATE);
    }
    else
    {
        cout << "No contours found" << endl;
        return -1;
    }

    return 0;
}
