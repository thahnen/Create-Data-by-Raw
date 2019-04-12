//
// Created by thahnen on 12.04.19.
//

#ifndef CREATEBETTERIMAGES_OBJECT_H
#define CREATEBETTERIMAGES_OBJECT_H

#include <vector>
#include <opencv4/opencv2/opencv.hpp>

using namespace std;
using namespace cv;


/***********************************************************************************************************************
 *
 *      class Object:
 *      ============
 *
 *      - vector<Point> outerHull,
 *      - Point center,
 *
 *      Beinhaltet ein im Bild gefundenes Objekt, dass sich ggf. aus mehreren Huellen zusammensetzt.
 *      Die aeussere Huelle umhuellt alle innenliegenden Huellen, gebildet anhand der Extrempunkte derer.
 *      Der Mittelpunkt ist der Mittelpunkt der Flaeche, die sich aus den Mittelpunkten der Huellen ergibt.
 *
 ***********************************************************************************************************************/
class Object {
public:
    Object(vector<Point> outerHull, Point center) {
        outerHull = outerHull;
        center = center;
    };

    Object(vector<vector<Point>> hulls) {
        // berechnet alles aus den angegebenen hulls!
        // die aeussere Huelle aus den Extrempunkten der einzelnen Huellen
        // den Mittelpunkt aus eben der berechneten aeusseren Huelle
    }
private:
    vector<Point> outerHull;
    Point center;
};


#endif //CREATEBETTERIMAGES_OBJECT_H
