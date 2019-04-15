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



    /*
     *      vlt ggf beim durchiterieren ueberpruefen, wenn element groesser ob ein anderes ist auch
     *      ob das andere jeweils groesser und dann test ob drin liegt und dann zum loeschen merken!
     *      => koennte das "get_hulls_by_thresh2" ersparen?
     */
    // Durch alle Hulls (ohne das erste) vorwaerts iterieren, ob Hull in einem anderen liegt
    for (auto it = hull.begin()+1; it < hull.end();) {

        for (auto davor = hull.begin(); davor < it;) {
            // Ueberpruefen, ob Flaeche so gross ist wie grosse Flaeche oder kleiner
            if (contourArea(*it, false) < contourArea(*davor, false)) {
                // Annahme: es liegt in dem anderen Hull
                Moments m = moments(*it, false);

                if (pointPolygonTest(*davor, Point2f(m.m10/m.m00, m.m01/m.m00), false) > 0) {
                    // liegt drin, kann geloescht werden!
                    it = hull.erase(it);
                    goto weiter;
                }
            } // hier also die zweite Bedingung!
            ++davor;
        }
        weiter:
        ++it;
    }

    return hull;
}


/***********************************************************************************************************************
 *
 *      get_hulls_by_thresh2 ( Mat src, double area_thresh ) => vector<vector<Point>>
 *      =============================================================================
 *
 *      Ruft intern get_hulls_by_thresh auf, iteriert danach nur noch einmal rueckwaerts durch den Vector um
 *      zu ueberpruefen, ob das jeweilige Hull in einem danach liegt?
 *
 *      TODO: irgendwie wird noch nichts geloescht, aber warum?
 *      TODO: ggf andersherum machen:
 *              => 1) hochzaehlen, das zu ueberpruefen vom aktuellen runterzaehlen
 *              == 2) runterzaehlen, das zu ueberpruefen vom aktuellen hochzaehlen
 *
 ***********************************************************************************************************************/
vector<vector<Point>> get_hulls_by_thresh2(Mat src, double thresh_area) {
    vector<vector<Point>> hull = get_hulls_by_thresh(src, thresh_area);

    // Durch alle Hulls (ohne das letzte) rueckwaerts iterieren, ob Hull in einem anderen liegt
    for (auto it = hull.end()-2; it >= hull.begin();) {

        for (auto danach = it+1; danach != hull.end();) {
            // Ueberpruefen, ob Flaeche so gross ist wie grosse Flaeche oder kleiner

            if (contourArea(*it, false) < contourArea(*danach, false)) {

                // Mittelpunkt der aktuellen Flaeche berechnen und gegen das folgende Hull testen
                Moments m = moments(*it, false);

                if (pointPolygonTest(*danach, Point2f(m.m10/m.m00, m.m01/m.m00), false) > 0) {
                    cout << "PPT war erfolgreich -> liegt in anderem" << endl;

                    // liegt drin, kann geloescht werden!
                    it = hull.erase(it)-1; //geht einen weiter nach hinten im Array, deshalb einen weiter nach hinten!
                    goto weiter2;
                }
            }
            ++danach;
        }
        weiter2:
        --it;
    }

    return hull;
}


#endif //CREATEBETTERIMAGES_CONTOUR_WORKER_H
