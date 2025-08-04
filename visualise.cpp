#include <bits/stdc++.h>

using namespace std;

struct Pt
{
    double x, y;
};

// ---------- tiny CSV reader ----------
vector<Pt> read_pts(const string &file, bool raw = true)
{
    vector<Pt> v;
    ifstream f(file);
    double a, b, c, d;
    string line;
    while (getline(f, line))
    {
        stringstream ss(line);
        if (raw)
        {
            ss >> a >> b;
            v.push_back({a, b});
        }
        else
        {
            ss >> a >> b >> c >> d;
            v.push_back({a, b});
            v.push_back({c, d});
        }
    }
    return v;
}

// ---------- SVG helpers ----------
void svg_line(ofstream &o, const Pt &a, const Pt &b, const string &stroke, double width)
{
    o << "<line x1=\"" << a.x << "\" y1=\"" << -a.y << "\" x2=\"" << b.x << "\" y2=\"" << -b.y << "\" stroke=\""
      << stroke << "\" stroke-width=\"" << width << "\" />\n";
}
void svg_circle(ofstream &o, const Pt &p, const string &fill, double r)
{
    o << "<circle cx=\"" << p.x << "\" cy=\"" << -p.y << "\" r=\"" << r << "\" fill=\"" << fill << "\" />\n";
}

// ---------- main ----------
int main()
{
    // read data
    vector<Pt> edges;
    {
        auto e = read_pts("edges.csv", false);
        edges.swap(e);
    }
    vector<Pt> trace = read_pts("trace.csv");
    vector<Pt> matched;
    {
        ifstream f("matched.txt");
        double x, y;
        while (f >> x >> y)
            matched.push_back({x, y});
    }

    // bounding box (for viewBox)
    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
    auto grow = [&](const Pt &p) {
        minX = min(minX, p.x);
        maxX = max(maxX, p.x);
        minY = min(minY, p.y);
        maxY = max(maxY, p.y);
    };
    for (auto &p : edges)
        grow(p);
    for (auto &p : trace)
        grow(p);
    for (auto &p : matched)
        grow(p);

    double pad = 0.1 * max(maxX - minX, maxY - minY);
    minX -= pad;
    maxX += pad;
    minY -= pad;
    maxY += pad;

    // write SVG
    ofstream svg("map.svg");
    svg << fixed << setprecision(3);
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "viewBox=\"" << minX << " " << -maxY << " " << (maxX - minX) << " " << (maxY - minY) << "\">\n";

    // edges (black)
    for (size_t i = 0; i < edges.size(); i += 2)
        svg_line(svg, edges[i], edges[i + 1], "black", 0.03);

    // trace (red)
    for (size_t i = 1; i < trace.size(); ++i)
        svg_line(svg, trace[i - 1], trace[i], "red", 0.05);

    // matched points (blue)
    for (const Pt &p : matched)
        svg_circle(svg, p, "blue", 0.08);

    svg << "</svg>\n";
    cout << "Generated map.svg  (open in browser)\n";
}
