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

    // Alle Konturen mit kleinerer Flaeche wegschmeissen (bisher als bestes 0)
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
 *      Geht anders als get_hulls_by_thresh an die Sache heran, indem direkt versucht wird mehr auszumisten,
 *      damit nachher nicht zu viel uebrig ist und ggf nochmal nach einem Hull gesucht werden muss!
 *
 *      1) alle Objekte, die vollstaendig in anderen liegen fallen raus
 *      2) alle Objekte, die sich mit anderen ueberlappen, werden zu einem zusammengefasst
 *      3) alle Objekte, die nicht die Kriterien erfuellen fallen komplett raus
 *
 ***********************************************************************************************************************/
vector<vector<Point>> get_hulls_by_thresh2(Mat src, double thresh_area) {
    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;

    findContours(src, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

    // Alle Konturen mit kleinerer Flaeche wegschmeissen (bisher als bestes 0)
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

    // Hier werden alle Huellen aussortiert, die weniger als 3 Eckpunkte haben (koennen ergo keine Flaeche aufspannen)
    for (auto it = hull.begin(); it != hull.end();) {
        if ((*it).size() < 3) {
            it = hull.erase(it);
        } else {
            ++it;
        }
    }

    // Alle Momente bekommen, damit man spaeter damit rechnen kann
    vector<Moments> momente(hull.size());
    for (int i=0; i<hull.size(); i++) {
        momente[i] = moments(hull[i], true);
    }


    // TODO: hier irgendwie einen Fehler werfen, weil kein Hull gefunden wurde!
    if (hull.size() < 1) return hull;


    // TODO: ggf im gleichen Abwasch ueberpruefen, ob Flachen uberlappen (zum Grossteil), dann werden die zusammengefasst!
    // TODO: am besten, indem man ueberprueft, ob die Punkte ineinander liegen?
    // Fuer jede Huelle ueberpruefen, ob sie in einer folgenden Huelle liegt oder umgekehrt!
    for (auto it=hull.begin(); it<hull.end()-1;) {
        for (auto it2=it+1; it2<hull.end();) {
            // Ueberpruefen, ob Huellen-Flaeche kleiner ist als die der folgenden Huelle (-> als Mass dass sie darin liegt)
            double area1 = contourArea(*it, false);
            double area2 = contourArea(*it2, false);

            // TODO: Damit Proportion immer im Intervall (0, 1] := 0 < x <= 1 bleibt wird Variable benutzt!
            bool folgendes_kleiner = area1 >= area2;
            double proportion = folgendes_kleiner ? area2/area1 : area1/area2;

            if (!folgendes_kleiner) {
                // Flaeche kleiner (/gleich) als der der folgenden Huelle
                // TODO: => Proportion winzig (kp < 0.25) nur ueberpruefen, ob Mittelpunkt in grosser Flaeche liegt
                // TODO: => Proportion klein (kp > 0.25) noch, ob Mittelpunkt weit genug vom Rand entfernt
                // TODO: => Proportion gross (kp > 0.75) noch, ob Extrempunkte in der Flaeche liegen
                // TODO: => Proportion gleich (kp > 0.95) fuer die restlichen Punkte ueberpruefen, ob in Flaeche liegen

                // Ueberpruefen, ob die Huelle auch wirklich in der anderen liegt
                // TODO: 1) Mittelpunkt ?
                Moments m = moments(*it, false);
                Point2f middle(m.m10/m.m00, m.m01/m.m00);

                bool inside = false;    // Variable die sich aendert, wenn komplett drinnen oder eben nicht festgestellt wird!
                if (proportion > 0) {
                    // groesser 0 damit am Rand nicht beachtet wird
                    if (pointPolygonTest(*it2, middle, false) > 0) {
                        inside = true;
                    }
                }

                // TODO: 2) Mittelpunkt weitgenug vom Rand entfernt (d.h. nah genug am anderen Mittelpunkt dran)
                if (inside && proportion > 0.25) {
                    bool first_done = false;
                    double distance = pointPolygonTest(*it2, middle, true);
                    // 1. Annahme kleineres Objekt -> Quadrat
                    double radius = sqrt(area1);
                    first_done = radius <= distance;            // erster Test erfolgreich ?

                    // 2. Annahme kleineres Objekt -> Kreis
                    radius = sqrt(area1/M_PI);
                    bool second_done = radius <= distance;      // zweiter Test erfolgreich ?
                    if (!(first_done && second_done)) {

                        // 3. Annahme kleineres Objekt -> Dreieck (gleichseitig)
                        radius = (2*sqrt(3*area1) / pow(3, 5/4));
                        if (!(radius <= distance && (first_done || second_done))) {
                            inside = false;                     // keine zwei Tests erfolgreich
                        }
                    }

                }

                Point* huelle = &((*it)[0]);                // das hier sollte ein C-Array anstatt des vector sein!
                Point min_x, min_y, max_x, max_y;           // hier gespeichert, damit man die im 4. Schritt noch verwenden kann!

                // TODO: 3) Extrempunkte in anderer Flaeche?
                if (inside && proportion > 0.75) {
                    // 1. kleinstes X
                    min_x = *min_element(huelle, huelle + (sizeof(huelle)/ sizeof(*huelle)), [](Point a, Point b) {
                        return a.x < b.x;
                    });

                    if (pointPolygonTest(*it2, min_x, false) < 0) {
                        // liegt nicht drin!
                        inside = false;
                    } else {

                        // 2. kleinstes Y
                        min_y = *min_element(huelle, huelle + (sizeof(huelle)/ sizeof(*huelle)), [](Point a, Point b) {
                            return a.y < b.y;
                        });

                        if (pointPolygonTest(*it2, min_y, false) < 0) {
                            // liegt nicht drin!
                            inside = false;
                        } else {

                            // 3. groesstes X
                            max_x = *max_element(huelle, huelle + (sizeof(huelle)/ sizeof(*huelle)), [](Point a, Point b) {
                                return a.x > b.x;
                            });

                            if (pointPolygonTest(*it2, max_x, false) < 0) {
                                // liegt nicht drin!
                                inside = false;
                            } else {
                                // 4. groesstes Y
                                max_y = *max_element(huelle, huelle + (sizeof(huelle)/ sizeof(*huelle)), [](Point a, Point b) {
                                    return a.y > b.y;
                                });

                                if (pointPolygonTest(*it2, max_y, false) < 0) {
                                    // liegt nicht drin!
                                    inside = false;
                                }
                            }
                        }
                    }
                }

                // TODO: 4) Restliche Punkte ?
                if (inside && proportion > 0.95) {
                    int anz_huellen_punkte = (*it).size();
                    for (Point punkt : *it) {
                        // TODO: durch alle Punkte iterieren, die nicht die Maxima / Minima sind!
                        if (!(punkt == min_x || punkt == min_y || punkt == max_x || punkt == max_y)) {
                            if (pointPolygonTest(*it2, punkt, false) < 0) {
                                // liegt nicht drin
                                inside = false;
                            }
                        }
                    }
                }
            } else {
                // Flaeche groesser als die der folgenden Huelle
                // TODO: wie oben nur andersherum? ggf damit zusammenfassen
            }

            // TODO: Ueberpruefen, ob es drin liegt, weil wenn ja, dann kann das jeweilige Objekt geloescht werden!
        }
    }

    return hull;
}


#endif //CREATEBETTERIMAGES_CONTOUR_WORKER_H
