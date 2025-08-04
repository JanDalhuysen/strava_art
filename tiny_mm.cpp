#include <bits/stdc++.h>

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

// ---------- read helpers ----------
vector<Edge> read_edges(const string &file)
{
    vector<Edge> out;
    ifstream f(file);
    int id;
    double ax, ay, bx, by;
    while (f >> id >> ax >> ay >> bx >> by)
        out.push_back({id, {ax, ay}, {bx, by}});
    return out;
}
vector<Pt> read_trace(const string &file)
{
    vector<Pt> out;
    ifstream f(file);
    double x, y;
    while (f >> x >> y)
        out.push_back({x, y});
    return out;
}

// ---------- geometry ----------
double sq(double x)
{
    return x * x;
}

// closest distance from p to segment ab
double dist_seg(Pt p, Pt a, Pt b)
{
    double l2 = sq(a.x - b.x) + sq(a.y - b.y);
    if (l2 == 0.0)
        return hypot(p.x - a.x, p.y - a.y);
    double t = max(0.0, min(1.0, ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2));
    Pt proj = {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
    return hypot(p.x - proj.x, p.y - proj.y);
}

// ---------- map-matching ----------
vector<Pt> snap(const vector<Edge> &edges, const vector<Pt> &pts)
{
    vector<Pt> snapped;
    for (const Pt &p : pts)
    {
        double best = 1e9;
        Pt proj;
        for (const Edge &e : edges)
        {
            double d = dist_seg(p, e.a, e.b);
            if (d < best)
            {
                best = d;
                // compute projection on segment
                double l2 = sq(e.a.x - e.b.x) + sq(e.a.y - e.b.y);
                double t = ((p.x - e.a.x) * (e.b.x - e.a.x) + (p.y - e.a.y) * (e.b.y - e.a.y)) / l2;
                t = max(0.0, min(1.0, t));
                proj = {e.a.x + t * (e.b.x - e.a.x), e.a.y + t * (e.b.y - e.a.y)};
            }
        }
        snapped.push_back(proj);
    }
    return snapped;
}

// ---------- demo ----------
int main()
{
    auto edges = read_edges("edges.csv");
    auto trace = read_trace("trace.csv");

    auto matched = snap(edges, trace);

    cout << fixed << setprecision(2);
    cout << "matched trace (lon lat):\n";
    for (auto &p : matched)
        cout << p.x << ' ' << p.y << '\n';
    return 0;
}
