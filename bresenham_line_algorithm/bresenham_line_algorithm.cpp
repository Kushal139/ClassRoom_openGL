#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include "EasyBMP.h"

using namespace std;
void drawLine(int x1, int y1, int x2, int y2, BMP& fileName){
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    if(dx >= dy){
        if(x1 > x2){
            swap(x1, x2);
            swap(y1, y2);
        }
        int p = 2 * dy - dx;
        int y = y1;
        for(int x = x1; x <= x2; x++){
            fileName.SetPixel(x, y, RGBApixel{0, 0, 0, 0});
            if(p >= 0){
                p += 2*dy - 2*dx;
                y++;
            }
            else {
                p += 2*dy;
            }
        }
    }
    else {
        if(y1 > y2){
            swap(x1, x2);
            swap(y1, y2);
        }
        int p = 2 * dx - dy;
        int x = x1;
        for(int y = y1; y <= y2; y++){
            fileName.SetPixel(x, y, RGBApixel{0, 0, 0, 0});
            if(p >= 0){
                p += 2*dx - 2*dy;
                x++;
            }
            else {
                p += 2*dx;
            }
        }
    }
}

int main () {
    BMP image;
    image.SetSize(300, 200);
    image.SetBitDepth(24);
    for (int i = 0; i < 300; i++) {
        for (int j = 0; j < 200; j++) {
            image.SetPixel(i, j, RGBApixel{255, 255, 255, 0});
        }
    }
    drawLine(20, 30, 250, 150, image);
    drawLine(2, 0, 150, 198, image);
    drawLine(3, 1, 151, 199, image);
    drawLine(1, 0, 149, 197, image);
    if (image.WriteToFile("line.bmp")) {
        cout << "line.bmp created successfully!" << endl;
    } else {
        cout << "Error writing file!" << endl;
    }
    cout << "Line drawn in line.bmp" << endl;
    return 0;
}