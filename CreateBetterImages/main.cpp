#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "contour_worker.h"


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


// Um unterschiedliche Bilder zu vergleichen!
Mat get_hull_by_thresh(Mat src, double thresh) {
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    findContours(src, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

    // TODO: Hier alle Kontouren aussortieren!
    // 1) die kleiner als Threshold
    // 2) Mittelpunkt der kleineren Fläche in der groesseren? -> kommt irgendwann
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

    Mat hull_img = Mat::zeros(src.size(), src.type());
    for (int i=0; i<contours.size(); i++) {
        drawContours(hull_img, hull, i, 255);
    }

    return hull_img;
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

        // Muss gemacht werden, da beide Bilder eigentlich so vorliegen!
        cvtColor(frame1, frame1, COLOR_BGR2GRAY);
        cvtColor(frame2, frame2, COLOR_BGR2GRAY);

        Mat added = frame1 + frame2;

        // Muss gemacht werden, da durch Konvertiertung zum Video andere Graustufen mit eingebracht wurden!
        threshold(added, added, 127, 255, THRESH_BINARY);

        // Hier Hit-or-Miss Operator um kleine Elemente zu loeschen!
        Mat hom_1x1 = hit_or_miss(added, (Mat_<int>(3, 3) <<
                -1, -1, -1,
                -1,  1, -1,
                -1, -1, -1)
                );
        imshow("Hit-or-Miss (1x1)", hom_1x1);


        // Huellen berechnen lassen
        Mat hull = Mat::zeros(hom_1x1.size(), hom_1x1.type());
        vector<vector<Point>> hulls = get_hulls_by_thresh(hom_1x1, 0);
        for (int i=0; i<hulls.size(); i++) {
            drawContours(hull, hulls, i, 255);
        }
        imshow("Hull (HOM-1x1)", hull);


        // Fuer jede Huelle die naechste berechnen, damit diese zueinander gehoeren koennen
        vector<Moments> momente(hulls.size());
        for (int i=0; i<momente.size(); i++) {
            momente[i] = moments(hulls[i], false);
        }

        for (auto it = momente.begin(); it != momente.end();) {
            int naechster = -1;
            for (auto it2 = momente.begin(); it2 != momente.end();) {
                if (it != it2) {
                    // TODO: hier den Abstand zwischen den Mittelpunkten suchen und wenn kleiner als alter eintragen!
                    //int iterator_1_int = it2-momente.begin();
                    //Point2f mitte()
                }
            }
        }



        waitKey(0);
    }
}


/***********************************************************************************************************************
 *
 ***********************************************************************************************************************/
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