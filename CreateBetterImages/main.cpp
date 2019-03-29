#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>


using namespace std;
using namespace cv;


void create_using_morph(VideoCapture vid) {
    for (;;) {
        Mat frame;
        vid >> frame;
        if (frame.empty()) {
            cout << "Letzter Frame oder Video leer!" << endl;
            break;
        }

        Mat open1 = frame.clone();
        Mat open2 = frame.clone();
        Mat open3 = frame.clone();

        morphologyEx(open1, open1, MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)));
        morphologyEx(open2, open2, MORPH_OPEN, getStructuringElement(MORPH_CROSS, Size(3, 3)));
        morphologyEx(open3, open3, MORPH_OPEN, getStructuringElement(MORPH_RECT, Size(3, 3)));

        imshow("Frame", frame);
        imshow("Opening (Ellipse)", open1);
        imshow("Opening (Cross)", open2);
        imshow("Opening (Rect)", open3);
        waitKey(0);
    }
}


void get_contours_by_thresh(double thresh) {
    // damit unterschiedliche Bilder zum Vergleich erstellt werden koennen!
}


void create_using_added_frames(VideoCapture vid) {
    for (;;) {
        Mat frame1;
        vid >> frame1;
        if (frame1.empty()) {
            cout << "1. Frame im letzten Schritt leer!" << endl;
            break;
        }
        Mat frame2;
        vid >> frame2;
        if (frame2.empty()) {
            cout << "2. Frame im letzten Schritt leer!" << endl;
            break;
        }

        Mat added = frame1 + frame2;
        cvtColor(added, added, COLOR_BGR2GRAY);

        imshow("Addiert", added);

        // Hier Hit-or-Miss Operator um kleine Elemente zu loeschen!
        Mat kernel = (Mat_<int>(5, 5) <<
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 0, -1, 0, 1,
            1, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        );

        Mat output_img;
        morphologyEx(added, output_img, MORPH_HITMISS, kernel);
        //output_img = added - output_img;
        imshow("Hit-or-Miss", output_img);


        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;

        findContours(added, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

        // TODO: Hier alle Kontouren aussortieren!
        // 1) die kleiner als Threshold
        // -) die in anderer Kontour vollstaendig liegen! -> Idee war gut dauert aber Jahre
        // 2) Mittelpunkt der kleineren Fläche in der größeren?
        double thresh = 5;
        for (auto it = contours.begin(); it != contours.end();) {
            double area = contourArea(*it, false);
            if (area < thresh) {
                it = contours.erase(it);
            } else {
                ++it;
            }
        }

        vector<vector<Point>> hull(contours.size());
        for (int i=0; i<contours.size(); i++) {
            convexHull(Mat(contours[i]), hull[i], false);
        }

        Mat hull_img = Mat::zeros(added.size(), added.type());
        for (int i=0; i<contours.size(); i++) {
            drawContours(hull_img, hull, i, 255);
        }

        imshow("Hull", hull_img);
        waitKey(0);
    }
}


int main() {
    VideoCapture vid("../media/01.mp4");
    if (!vid.isOpened()) {
        cerr << "Video kann nicht geoeffnet werden!" << endl;
        return 1;
    }

    //create_using_morph(vid);
    create_using_added_frames(vid);

    return 0;
}