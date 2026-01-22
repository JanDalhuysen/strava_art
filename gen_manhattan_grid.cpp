#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Point
{
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y)
    {
    }
};

void generateGrid(std::ostream &out, int width, int height, double spacing = 1.0)
{
    int edge_id = 0;

    // Generate horizontal edges
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width - 1; x++)
        {
            double x1 = x * spacing;
            double y1 = y * spacing;
            double x2 = (x + 1) * spacing;
            double y2 = y * spacing;
            out << edge_id++ << "," << x1 << "," << y1 << "," << x2 << "," << y2 << std::endl;
        }
    }

    // Generate vertical edges
    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height - 1; y++)
        {
            double x1 = x * spacing;
            double y1 = y * spacing;
            double x2 = x * spacing;
            double y2 = (y + 1) * spacing;
            out << edge_id++ << "," << x1 << "," << y1 << "," << x2 << "," << y2 << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    // Generate a Manhattan grid with unit spacing
    // Default: 6x6 grid (which creates a 5x5 block grid with spacing 1.0)
    int width = 6;
    int height = 6;
    double spacing = 1.0;

    if (argc >= 2)
        width = std::stoi(argv[1]);
    if (argc >= 3)
        height = std::stoi(argv[2]);
    if (argc >= 4)
        spacing = std::stod(argv[3]);

    std::ofstream file("edges.csv");
    if (file.is_open())
    {
        generateGrid(file, width, height, spacing);
        file.close();
        std::cout << "Grid " << width << "x" << height << " with spacing " << spacing
                  << " generated and saved to edges.csv" << std::endl;
    }
    else
    {
        std::cerr << "Unable to open file for writing" << std::endl;
        return 1;
    }

    return 0;
}
