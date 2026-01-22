#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cerr << "Usage: gen_circle <num_points>" << endl;
        cerr << "Example: gen_circle 40" << endl;
        return 1;
    }

    int num_points = stoi(argv[1]);
    double center_x = 2.5;
    double center_y = 2.5;
    double radius = 1.5;

    ofstream f("trace.csv");
    if (!f.is_open())
    {
        cerr << "Error: Could not open trace.csv for writing" << endl;
        return 1;
    }

    f << fixed << setprecision(6);

    for (int i = 0; i < num_points; i++)
    {
        double angle = 2.0 * M_PI * i / num_points;
        double x = center_x + radius * cos(angle);
        double y = center_y + radius * sin(angle);
        f << x << "," << y << "\n";
    }

    f.close();
    cout << "Generated " << num_points << " circle points centered at (" << center_x << ", " << center_y
         << ") with radius " << radius << " -> trace.csv" << endl;

    return 0;
}
