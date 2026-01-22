#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

struct Pt
{
    double x, y;
};

struct Edge
{
    int id;
    Pt a, b;
};

struct ProjectionResult
{
    double distance;
    Pt projected_point;
};

// ---------- read helpers ----------
vector<Edge> read_edges(const string &file)
{
    vector<Edge> out;
    ifstream f(file);
    if (!f.is_open())
    {
        throw runtime_error("Could not open edge file: " + file);
    }
    string line;
    while (getline(f, line))
    {
        replace(line.begin(), line.end(), ',', ' ');
        stringstream ss(line);
        int id;
        double ax, ay, bx, by;
        if (ss >> id >> ax >> ay >> bx >> by)
            out.push_back({id, {ax, ay}, {bx, by}});
    }
    return out;
}
vector<Pt> read_trace(const string &file)
{
    vector<Pt> out;
    ifstream f(file);
    if (!f.is_open())
    {
        throw runtime_error("Could not open trace file: " + file);
    }
    string line;
    while (getline(f, line))
    {
        replace(line.begin(), line.end(), ',', ' ');
        stringstream ss(line);
        double x, y;
        if (ss >> x >> y)
            out.push_back({x, y});
    }
    return out;
}

// ---------- geometry ----------
double sq(double x)
{
    return x * x;
}

// Projects point p onto segment ab and returns the distance and projected point.
ProjectionResult project_on_segment(Pt p, Pt a, Pt b)
{
    double l2 = sq(a.x - b.x) + sq(a.y - b.y);
    if (l2 == 0.0)
    {
        double dist = hypot(p.x - a.x, p.y - a.y);
        return {dist, a};
    }
    double t = max(0.0, min(1.0, ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2));
    Pt proj = {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
    double dist = hypot(p.x - proj.x, p.y - proj.y);
    return {dist, proj};
}

// ---------- map-matching ----------
vector<Pt> snap(const vector<Edge> &edges, const vector<Pt> &pts)
{
    vector<Pt> snapped;
    for (const Pt &p : pts)
    {
        double best_dist = 1e9;
        Pt best_proj;
        for (const Edge &e : edges)
        {
            ProjectionResult res = project_on_segment(p, e.a, e.b);
            if (res.distance < best_dist)
            {
                best_dist = res.distance;
                best_proj = res.projected_point;
            }
        }
        snapped.push_back(best_proj);
    }
    return snapped;
}

// ---------- demo ----------
int main()
{
    try
    {
        auto edges = read_edges("edges.csv");
        auto trace = read_trace("trace.csv");

        cout << "Read " << edges.size() << " edges and " << trace.size() << " trace points." << endl;

        if (edges.empty() || trace.empty())
        {
            cerr << "Warning: Edge or trace data is empty. No matching will be performed." << endl;
            return 1;
        }

        auto matched = snap(edges, trace);

        // Print summary to stderr, coordinates to stdout
        cerr << fixed << setprecision(2);
        cerr << "matched trace (lon lat):\n";
        for (auto &p : matched)
            cerr << p.x << ' ' << p.y << '\n';

        // Also save to matched.txt in clean format
        ofstream out("matched.txt");
        out << fixed << setprecision(6);
        for (auto &p : matched)
            out << p.x << ' ' << p.y << '\n';
        out.close();
        cerr << "Saved to matched.txt\n";
    }
    catch (const runtime_error &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
