#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <cmath>

using namespace cv;
using namespace std;

#define DIFF_THRESH 230

enum {
    MAX_WORDS = 26,          // Number of letters
    SAMPLE_RATE = 1,         // Frame sampling rate (every x loops)
    NUM_LAST_LETTERS = 5,    // Number of letters to store
    RESET_THRESH = 25000000, // Minimum sum pixel data to automatically reset
    THRESH = 200,            // Threshold to eliminate background noise
    KEY_ESC = 27,            // 'esc' key code
};

// Global variables
Ptr<BackgroundSubtractor> pMOG2; // MOG2 Background subtractor
vector<Point> letters[MAX_WORDS];

// Function declarations
void processVideo();

int main(int argc, char* argv[]) {
cout<<"hello";
    // Create Background Subtractor objects (MOG2 approach)
    pMOG2 = new BackgroundSubtractorMOG2();

    // Preload letter images
    for (int i = 0; i < MAX_WORDS; i++) {
        char buf[13 * sizeof(char)];
        sprintf(buf, "images3/%c.png", (char)('a' + i));
        Mat im = imread(buf, 1);
        if (im.data) {
            Mat bwim;
            cvtColor(im, bwim, CV_RGB2GRAY);
            Mat threshold_output;
            vector<Vec4i> hierarchy;
            vector<vector<Point> > contours;
            // Detect edges using Threshold
            threshold( bwim, threshold_output, THRESH, 255, THRESH_BINARY );
            findContours(threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
            letters[i] = contours[0];
        }
    }
    processVideo();
    return EXIT_SUCCESS;
}
void processVideo() {
    // Create the capture object
    VideoCapture capture = VideoCapture(0);
    if (!capture.isOpened()) {
        // Error in opening the video input
        cerr << "Cannot Open Webcam... " << endl;
        exit(EXIT_FAILURE);
    }
cout<<"hello";
    Mat frame;           // current frame
    Mat fgMaskMOG2;      // fg mask fg mask generated by MOG2 method
    int keyboard = 0;    // last key pressed
    int frames = 0;      // number of frames since last sample
    int letterCount = 0; // number of letters captured since last display
    char lastLetters[NUM_LAST_LETTERS] = {0};
    Mat letterText = Mat::zeros(200, 200, CV_8UC3);
    int counts[MAX_WORDS+1] = {0};
    // Read input data
    while ((char)keyboard != KEY_ESC) {
        // Read the current frame
        if (!capture.read(frame)) {
            cerr << "Unable to read next frame." << endl;
            cerr << "Exiting..." << endl;
            exit(EXIT_FAILURE);
        }
        // Crop Frame to smaller region
        cv::Rect myROI(250, 100, 300, 300);
        Mat cropFrame = frame(myROI);
        // Update the background model
        pMOG2->operator()(cropFrame, fgMaskMOG2);
        // Generate Convex Hull
        Mat threshold_output;
        vector<Vec4i> hierarchy;
        vector<vector<Point> > contours;
        // Detect edges using Threshold
        threshold( fgMaskMOG2, threshold_output, THRESH, 255, THRESH_BINARY );
        // Find contours
        findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
        // Find largest contour
        Mat drawing = Mat::zeros(cropFrame.size(), CV_8UC3);
        double largest_area = 0;
        int maxIndex = 0;
        for (int j = 0; j < contours.size(); j++) {
        if(contourArea(contours[j])>5000)
        {
                maxIndex = j;  // Store the index of largest contour
        }
        }
        Scalar color = Scalar(0, 0, 255);
        drawContours(drawing, contours, maxIndex, Scalar(255, 255, 255), CV_FILLED); // fill white
        imshow("draw",drawing);
        Mat contourImg = Mat::zeros(cropFrame.size(), CV_8UC3);
        drawContours( contourImg, contours, maxIndex, Scalar(0, 0, 255), 2, 8, hierarchy, 0, Point(0, 0) );
        waitKey(30);
        // Reset if too much noise
        Scalar sums = sum(drawing);
        int s = sums[0] + sums[1] + sums[2] + sums[3];
        if (s >= RESET_THRESH) {
            pMOG2 = new BackgroundSubtractorMOG2();
            continue;
        }
        if (contours.size() > 0 && frames++ > SAMPLE_RATE && contours[maxIndex].size() >= 5) {
            RotatedRect testRect = fitEllipse(contours[maxIndex]);
            frames = 0;
            double lowestDiff = 1000000;
            char best = 'a';
            for (int i = 0; i < MAX_WORDS; i++) {
                if (letters[i].size() == 0) continue;
                if(i>=MAX_WORDS) break;
                 double diff = matchShapes(letters[i], contours[maxIndex],CV_CONTOURS_MATCH_I3, 0);
                 diff += matchShapes(letters[i], contours[maxIndex],CV_CONTOURS_MATCH_I1, 0); 
                if (diff < lowestDiff) {
                    lowestDiff = diff;
                    best = 'a' + i;
                }
            }
            letterText = Mat::zeros(200, 200, CV_8UC3);
            int flag=0;
            letterCount %= NUM_LAST_LETTERS;
            lastLetters[letterCount++] = best;
            if(letterCount==NUM_LAST_LETTERS-1&&flag==0)
                flag=1;
            letterText = Mat::zeros(200, 200, CV_8UC3);
            for (int i = 0; i < NUM_LAST_LETTERS&&flag==1; i++)
                counts[lastLetters[i] - 'a']++;
            int maxCount = 0;
            char maxChar ;
            for (int i = 0; i < MAX_WORDS; i++) {
                if (counts[i] > maxCount) {
                    maxCount = counts[i];
                    maxChar = 'a'+i;
                }
            }
            for (int i = 0; i < MAX_WORDS; i++) 
                counts[i]=0;
                char buf[2 * sizeof(char)];
                sprintf(buf, "%c", maxChar);
                putText(letterText, buf, Point(10, 75), CV_FONT_NORMAL, 3, Scalar(255, 255, 255), 1, 1);
        }
        imshow("Letter", letterText);
        // Get the input from the keyboard
        keyboard = waitKey(1);
        // Save image as keyboard input
        if (keyboard >= 'a' && keyboard <= 'z') {
            cout << "Wrote letter '" << (char)keyboard << '\'' << endl;
            // save in memory
            letters[keyboard - 'a'] = contours[maxIndex];
            // write to file
            char buf[13 * sizeof(char)];
            sprintf(buf, "images3/%c.png", (char)keyboard);
            imwrite(buf, drawing);
        }
        // Manual reset
        if (keyboard == ' ')
            pMOG2 = new BackgroundSubtractorMOG2();
    }
    // Delete capture object
    capture.release();
}
