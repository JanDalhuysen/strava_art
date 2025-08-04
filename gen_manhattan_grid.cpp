#include <iostream>
#include <vector>
#include <fstream>

struct Point {
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

void generateGrid(std::ostream& out, int width, int height, double spacing = 1.0) {
    // Generate horizontal edges
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width - 1; x++) {
            double x1 = x * spacing;
            double y1 = y * spacing;
            double x2 = (x + 1) * spacing;
            double y2 = y * spacing;
            out << x1 << "," << y1 << "," << x2 << "," << y2 << std::endl;
        }
    }
    
    // Generate vertical edges
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height - 1; y++) {
            double x1 = x * spacing;
            double y1 = y * spacing;
            double x2 = x * spacing;
            double y2 = (y + 1) * spacing;
            out << x1 << "," << y1 << "," << x2 << "," << y2 << std::endl;
        }
    }
}

int main() {
    // Generate a 3x3 grid with unit spacing
    std::ofstream file("edges.csv");
    if (file.is_open()) {
        generateGrid(file, 3, 3, 1.0);
        file.close();
        std::cout << "Grid generated and saved to edges.csv" << std::endl;
    } else {
        std::cerr << "Unable to open file for writing" << std::endl;
        return 1;
    }
    
    return 0;
}
