#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <opencv2/opencv.hpp>

#include "contour_worker.h"

using namespace std;
using namespace cv;

#define using_3_frames true


/***********************************************************************************************************************
 *
 *      Aus der FPS koennte man sich herleiten wie lange ein Bild maximal zur Bearbeitung brauchen darf,
 *      damit das Programm noch echtzeitfaehig ist!
 *      Es sind pro Ordner 1000 Bilder und ein Ordner hat die Zeitspanne 1min?
 *
 ***********************************************************************************************************************/


void create_using_added_frames(VideoCapture vid) {
    for (;;) {
        /***************************************************************************************************************
         *
         *      Frames einlesen, aus deren kumulierten Werten Objekte erkannt werden sollen!
         *
         ***************************************************************************************************************/
        Mat frame1;
        vid >> frame1;
        if (frame1.empty()) {
            cout << "1. Frame im letzten Schritt leer!" << endl;
            break;
        }
        cvtColor(frame1, frame1, COLOR_BGR2GRAY);
        //imshow("Frame 1", frame1);

        Mat frame2;
        vid >> frame2;
        if (frame2.empty()) {
            cout << "2. Frame im letzten Schritt leer!" << endl;
            break;
        }
        cvtColor(frame2, frame2, COLOR_BGR2GRAY);
        //imshow("Frame 2", frame2);

        Mat added = frame1 + frame2;

        #if using_3_frames
        Mat frame3;
        vid >> frame3;
        if (frame3.empty()) {
            cout << "3. Frame im letzten Schritt leer!" << endl;
            break;
        }
        cvtColor(frame3, frame3, COLOR_BGR2GRAY);
        //imshow("Frame 3", frame3);

        added += frame3;
        #endif

        // Muss gemacht werden, da durch Konvertiertung zum Video andere Graustufen mit eingebracht wurden!
        threshold(added, added, 127, 255, THRESH_BINARY);
        imshow("Added", added);


        /***************************************************************************************************************
         *
         *      Hit-or-Miss Operator anwenden um kleine Elemente zu loeschen!
         *
         ***************************************************************************************************************/
        auto start = chrono::steady_clock::now();
        Mat hom_1x1 = hit_or_miss(added, (Mat_<int>(3, 3) <<
                -1, -1, -1,
                -1,  1, -1,
                -1, -1, -1)
                );
        auto diff = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()-start).count();
        cout << "Verbrauchte Zeit um Hit-or-Miss zu erzeugen: " << diff << " Milliseconds" << endl;
        imshow("Hit-or-Miss (1x1)", hom_1x1);



        // kommt wieder weg!
        //waitKey(0);
        //continue;



        /***************************************************************************************************************
         *
         *      Huellen das erste mal aus dem Hit-or-Miss-Bild berechnen!
         *
         ***************************************************************************************************************/
        start = chrono::steady_clock::now();
        vector<vector<Point>> hulls = get_hulls_by_thresh(hom_1x1, 0);
        diff = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()-start).count();
        cout << "Verbrauchte Zeit um Hull zu erzeugen: " << diff << " Milliseconds" << endl;
        cout << "Hull-Size (vorher): " << hulls.size() << endl;


        /***************************************************************************************************************
         *
         *      Huellen noch einmal aus dem gefuellten Bild errechnen fuer bessere Ergebnisse!
         *
         ***************************************************************************************************************/
        Mat filled = Mat::zeros(added.size(), added.type());
        for (vector<Point> huelle: hulls) {
            fillConvexPoly(filled, &huelle[0], huelle.size(), Scalar(255.0));
        }
        Mat hull = Mat::zeros(added.size(), added.type());
        start = chrono::steady_clock::now();
        hulls = get_hulls_by_thresh(filled, 0);
        diff = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()-start).count();
        cout << "Verbrauchte Zeit um Hull zu erzeugen: " << diff << " Milliseconds" << endl;
        for (auto huelle : hulls) {
            fillConvexPoly(hull, &huelle[0], huelle.size(), 255);
        }
        imshow("Hull (Gefuellt)", hull);
        cout << "Hull-Size (nachher): " << hulls.size() << endl;


        /***************************************************************************************************************
         *
         *      Alle jetzt noch verbliebenen Huellen zu Objekten (so gut es geht) zusammenfassen!
         *
         ***************************************************************************************************************/
        start = chrono::steady_clock::now();
        // Hier werden alle Indizes von Huellen gespeichert, die zu einem Objekt gehoeren koennten!
        vector<vector<int>> objekte;

        // Fuer jede Huelle die naechste berechnen, damit diese zueinander gehoeren koennen
        vector<Moments> momente(hulls.size());
        for (int i=0; i<momente.size(); i++) {
            momente[i] = moments(hulls[i], false);
        }

        objekte.push_back(vector<int>{0});          // Der erste Index ist schon als Huelle eines Objekts vorhanden
        for (int i=1; i<momente.size(); i++) {
            int mgl_objekt_index = -1;              // Der Index von dem Objekt, zu dem die Huelle am ehesten gehoert
            for (int j=0; j<objekte.size(); j++) {
                for (int k : objekte[j]) {
                    // Ueberpruefen, ob die beiden Objekte sich ueberlappen / nah beieinander liegen:
                    // 1) Extrempunkte
                    Point* huelle = &(hulls[i][0]);

                    // 1.1) kleinstes X
                    Point min_x = *min_element(huelle, huelle + (sizeof(huelle)/ sizeof(*huelle)), [](Point a, Point b) {
                        return a.x < b.x;
                    });

                    if (pointPolygonTest(hulls[k], min_x, false) >= 0) {
                        objekte[j].push_back(i);
                        goto weiter_i;
                    }

                    // 1.2) kleinstes Y
                    Point min_y = *min_element(huelle, huelle + (sizeof(huelle)/ sizeof(*huelle)), [](Point a, Point b) {
                        return a.y < b.y;
                    });

                    if (pointPolygonTest(hulls[k], min_y, false) >= 0) {
                        objekte[j].push_back(i);
                        goto weiter_i;
                    }

                    // 1.3) groesstes X
                    Point max_x = *max_element(huelle, huelle + (sizeof(huelle)/ sizeof(*huelle)), [](Point a, Point b) {
                        return a.x > b.x;
                    });

                    if (pointPolygonTest(hulls[k], max_x, false) >= 0) {
                        objekte[j].push_back(i);
                        goto weiter_i;
                    }

                    // 1.4) groesstes Y
                    Point max_y = *max_element(huelle, huelle + (sizeof(huelle)/ sizeof(*huelle)), [](Point a, Point b) {
                        return a.y > b.y;
                    });

                    if (pointPolygonTest(hulls[k], max_y, false) >= 0) {
                        objekte[j].push_back(i);
                        goto weiter_i;
                    }


                    // 2) Abstand vergleichen, wenn kleiner als irgendein Wert
                    /*Point mid1(momente[i].m10/momente[i].m00 , momente[i].m01/momente[i].m00);
                    vector<double> dist{
                            sqrt(pow(mid1.x - min_x.x, 2) + pow(mid1.y - min_x.y, 2)),      // Mittelpunkt -> min_x
                            sqrt(pow(mid1.x - min_y.x, 2) + pow(mid1.y - min_y.y, 2)),      // Mittelpunkt -> min_y
                            sqrt(pow(mid1.x - max_x.x, 2) + pow(mid1.y - max_x.y, 2)),      // Mittelpunkt -> max_x
                            sqrt(pow(mid1.x - max_x.x, 2) + pow(mid1.y - max_x.y, 2))       // Mittelpunkt -> max_y
                    };*/


                    Point mid1(momente[i].m10/momente[i].m00 , momente[i].m01/momente[i].m00);
                    Point mid2(momente[k].m10/momente[k].m00 , momente[k].m01/momente[k].m00);
                    double thresh = 20;//(sqrt(contourArea(hulls[i], false)) + sqrt(contourArea(hulls[k], false)))/2;
                    if (sqrt(pow(mid1.x - mid2.x, 2) + pow(mid1.y - mid2.y, 2)) < thresh) {
                        objekte[j].push_back(i);
                        goto weiter_i;
                    }
                }
            }

            if (mgl_objekt_index == -1) {
                // Es passt zu keinem Objekt (ergo zu keiner Huelle in dem Objekt-Array)
                objekte.push_back(vector<int>{i});
            }

            weiter_i:;
        }


        // Jetzt noch einmal drueber gehen und alle Arrays mit nur einem Element aufleosen!
        // aber nicht Elemente miteinander verbinden, die beide eine zu grosse Flaeche haben! -> fehlt noch!
        for (auto it=objekte.begin(); it<objekte.end();) {
            if ((*it).size() == 1) {
                int einz = (*it)[0];
                Point mid1(momente[einz].m10/momente[einz].m00, momente[einz].m01/momente[einz].m00);

                double naechste_entfernung = 1000;      // der Wert weil groesser als groesstmoeglichste Entfernung im Bild!
                auto naechstes_objekt = it;
                for (auto it2=objekte.begin(); it2<objekte.end();) {
                    if (!(it == it2 || (*it2).size() < 2)) {
                        for (int index : *it2) {
                            Point mid2(momente[index].m10/momente[index].m00, momente[index].m01/momente[index].m00);
                            double abstand = sqrt(pow(mid1.x - mid2.x, 2) + pow(mid1.y - mid2.y, 2));

                            if (abstand > naechste_entfernung || abstand > 100) continue;

                            naechste_entfernung = abstand;
                            naechstes_objekt = it2;
                        }
                    }
                    it2++;
                }

                if(naechstes_objekt != it) {
                    (*naechstes_objekt).push_back(einz);
                    it = objekte.erase(it);
                } else {
                    it++;
                }
            } else {
                it++;
            }
        }

        diff = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now()-start).count();
        cout << "Verbrauchte Zeit um Objekte zusammenzufuegen: " << diff << " Milliseconds" << endl;

        // eigentlich muesste man hier fuer alle Objekte jedes Hull nehmen und ueberpruefen, ob es neben einem anderen liegt


        // Zu Testzwecken die Objekte anzeigen lassen
        int anz_obj = objekte.size();
        cout << "Anzahl gefundener Objekte: " << anz_obj << endl;

        for (int i=0; i< anz_obj; i++) {
            Mat objekte_img = Mat::zeros(filled.size(), filled.type());

            for (int j=0; j<objekte[i].size(); j++) {
                int index = objekte[i][j];
                fillConvexPoly(objekte_img, &(hulls[index][0]), hulls[index].size(), Scalar(255));
            }

            imshow("Objekte", objekte_img);
            waitKey(0);
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

    // Daten aus addierten Frames extrahieren!
    create_using_added_frames(vid);

    return 0;
}