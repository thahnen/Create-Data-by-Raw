//
// Created by thahnen on 01.04.19.
//

#ifndef CREATEBETTERIMAGES_CONTOUR_WORKER_H
#define CREATEBETTERIMAGES_CONTOUR_WORKER_H

#include <vector>
#include <opencv4/opencv2/opencv.hpp>

using namespace std;
using namespace cv;

/***********************************************************************************************************************
 *
 *      hit_or_miss ( Mat src, Mat kernel) => Mat
 *      =========================================
 *
 *      Fuehrt einen Hit-or-Miss Operator auf das Bild aus und liefert dieses zurueckt
 *      TODO: liegt hier weil ich nicht weiss wohin sonst
 *
 ***********************************************************************************************************************/
Mat hit_or_miss(Mat src, Mat kernel) {
    Mat output;
    morphologyEx(src, output, MORPH_HITMISS, kernel);
    return src - output;

}


/***********************************************************************************************************************
 *
 *      get_hulls_by_thresh ( Mat src, double area_thresh ) => vector<vector<Point>>
 *      ============================================================================
 *
 *      Gibt eine Liste aller Hulls (Huelle um ein Polygon) zurueck:
 *      - deren Flaeche groesser als gewisser Threshold sind
 *      - die nicht in anderen Hulls vollstaendig liegen
 *
 ***********************************************************************************************************************/
vector<vector<Point>> get_hulls_by_thresh(Mat src, double thresh_area) {
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    findContours(src, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    for (auto it = contours.begin(); it != contours.end();) {
        double area = contourArea(*it, false);
        if (area < thresh_area) {
            it = contours.erase(it);
        } else {
            ++it;
        }
    }

    // Alle Hulls bekommen, damit man spaeter damit rechnen kann
    vector<vector<Point>> hull(contours.size());
    for (int i=0; i<contours.size(); i++) {
        convexHull(Mat(contours[i]), hull[i], false);
    }

    // Alle Momente bekommen, damit man spaeter damit rechnen kann
    vector<Moments> momente(hull.size());
    for (int i=0; i<hull.size(); i++) {
        momente[i] = moments(hull[i], true);
    }


    for (auto it = hull.begin(); it != hull.end();) {
        if ((*it).size() < 3) {
            it = hull.erase(it);
        } else {
            ++it;
        }
    }


    // TODO: hier irgendwie einen Fehler werfen, weil kein Hull gefunden wurde!
    if (hull.size() < 1) return hull;



    // Durch alle Hulls (ohne das erste) iterieren, ob Hull in einem anderen liegt
    for (auto it = hull.begin()+1; it != hull.end();) {

        // TODO: das tritt auf, weil der Iterator irgendwie am Ende/ hinter dem Ende ist -> nicht sehr schoen geloest
        if ((*it).size() < 1) break;

        for (auto it2 = hull.begin(); it2 != it;) {
            // Ueberpruefen, ob Flache halb so gross ist wie grosse Flaeche oder kleiner
            if (contourArea(*it, false)*2 < contourArea(*it2, false)) {
                // Annahme: es liegt in dem anderen Hull
                Moments m = moments(*it, false);

                if (pointPolygonTest(*it2, Point2f(m.m10/m.m00, m.m01/m.m00), false) > 0) {
                    // liegt drin, kann geloescht werden!
                    it = hull.erase(it);
                    goto weiter;
                }
            }
            ++it2;
        }
        weiter:
        ++it;
    }

    return hull;
}


#endif //CREATEBETTERIMAGES_CONTOUR_WORKER_H
