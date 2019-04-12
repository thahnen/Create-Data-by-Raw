#include <iostream>
#include <vector>
#include <chrono>
#include <opencv2/opencv.hpp>

#include "contour_worker.h"

using namespace std;
using namespace cv;

#define using_3_frames true


/***********************************************************************************************************************
 *
 *      Aus der FPS koennte man sich herleiten wie lange in Bild maximal zur Bearbeitung benutzt werden darf,
 *      damit das Programm noch echtzeitfaehig ist!
 *      Es sind pro Ordner 1000 Bilder und ein Ordner hat die Zeitspanne 1min?
 *
 ***********************************************************************************************************************/


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
        cvtColor(frame1, frame1, COLOR_BGR2GRAY);

        Mat frame2;
        vid >> frame2;
        if (frame2.empty()) {
            cout << "2. Frame im letzten Schritt leer!" << endl;
            break;
        }
        cvtColor(frame2, frame2, COLOR_BGR2GRAY);

        #if using_3_frames
        Mat frame3;
        vid >> frame3;
        if (frame3.empty()) {
            cout << "3. Frame im letzten Schritt leer!" << endl;
            break;
        }
        cvtColor(frame3, frame3, COLOR_BGR2GRAY);
        Mat added = frame1 + frame2 + frame3;
        #else
        Mat added = frame1 + frame2;
        #endif

        // Muss gemacht werden, da durch Konvertiertung zum Video andere Graustufen mit eingebracht wurden!
        threshold(added, added, 127, 255, THRESH_BINARY);
        //imshow("Added", added);

        // Hier Hit-or-Miss Operator um kleine Elemente zu loeschen!
        auto start = chrono::steady_clock::now();
        Mat hom_1x1 = hit_or_miss(added, (Mat_<int>(3, 3) <<
                -1, -1, -1,
                -1,  1, -1,
                -1, -1, -1)
                );
        auto diff = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()-start).count();
        cout << "Verbrauchte Zeit um Hit-or-Miss zu erzeugen: " << diff << " Milliseconds" << endl;
        imshow("Hit-or-Miss (1x1)", hom_1x1);


        // Huellen berechnen lassen
        Mat hull = Mat::zeros(hom_1x1.size(), hom_1x1.type());
        start = chrono::steady_clock::now();
        vector<vector<Point>> hulls = get_hulls_by_thresh(hom_1x1, 0);
        diff = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()-start).count();
        cout << "Verbrauchte Zeit um Hull zu erzeugen: " << diff << " Milliseconds" << endl;
        for (int i=0; i<hulls.size(); i++) {
            drawContours(hull, hulls, i, 255);
        }
        imshow("Hull (HOM-1x1)", hull);
        cout << "Hull-Size: " << hulls.size() << endl;


        // mal fuellen
        Mat filled = hull.clone();
        for (vector<Point> huelle: hulls) {
            fillConvexPoly(filled, &huelle[0], huelle.size(), Scalar(255.0));
        }
        imshow("Gefuellte Huellen", filled);
        morphologyEx(filled, filled, MORPH_CLOSE, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)));
        imshow("Gefuellte Huellen (Closing)", filled);
        waitKey(0);


        continue;


        // Fuer jede Huelle die naechste berechnen, damit diese zueinander gehoeren koennen
        vector<Moments> momente(hulls.size());
        for (int i=0; i<momente.size(); i++) {
            momente[i] = moments(hulls[i], false);
        }

        for (auto it = momente.begin(); it != momente.end();) {
            auto naechster = it;
            for (auto it2 = momente.begin(); it2 != momente.end();) {
                auto naechster = it2;
                double kleinster_abstand = 0;
                if (it != it2) {
                    // Abstand vergleichen mit allen anderen Elementen
                }
            }
        }
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