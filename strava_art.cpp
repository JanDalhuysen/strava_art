/*
 *  strava_art.cpp
 *  Requires: CImg.h in same folder, FMM installed (libfmm.so + headers)
 */
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <vector>

#define cimg_use_png
#include "CImg.h"
using namespace cimg_library;

// --- FMM headers
#include <fmm/fmm.hpp>
#include <fmm/mm/fmm/fmm_algorithm.hpp>
#include <fmm/network.hpp>
#include <fmm/network_graph.hpp>
#include <fmm/ubodt.hpp>

// -----------------------------------------------------------------------------
// 1) Image → skeleton points
// -----------------------------------------------------------------------------
struct Point
{
    double x, y;
}; // image coords
std::vector<Point> skeleton_to_points(const char *fname)
{
    CImg<unsigned char> img(fname);
    img.channel(0); // grayscale
    img.normalize(0, 255);
    img.threshold(128); // binary
    img.dilate(2);
    img.erode(2);   // remove noise
    img.skeleton(); // 1-pixel wide centerline

    std::vector<Point> pts;
    cimg_forXY(img, x, y) if (img(x, y)) pts.push_back({(double)x, (double)y});
    return pts;
}

// -----------------------------------------------------------------------------
// 2) Image space → WGS-84 (simple linear transform)
// -----------------------------------------------------------------------------
std::vector<fmm::Point> geo_place(const std::vector<Point> &pts, double clat, double clon, double km_width = 10.0)
{
    const double km_per_deg_lat = 111.32;
    const double km_per_deg_lon = 111.32 * std::cos(clat * M_PI / 180.0);

    double img_w = 1000.0; // dummy – we normalize anyway
    double img_h = 1000.0;

    std::vector<fmm::Point> wgs;
    for (auto &p : pts)
    {
        double dx = (p.x / img_w - 0.5) * km_width;
        double dy = (p.y / img_h - 0.5) * km_width;
        double lon = clon + dx / km_per_deg_lon;
        double lat = clat + dy / km_per_deg_lat;
        wgs.emplace_back(lon, lat);
    }
    return wgs;
}

// -----------------------------------------------------------------------------
// 3) Map-matching helper
// -----------------------------------------------------------------------------
struct MatchResult
{
    fmm::LineString geom;
    double length_m = 0.0;
    double error = 0.0; // mean dist to original points
};

MatchResult match_trajectory(const std::vector<fmm::Point> &pts, const std::string &ubodt_file,
                             const std::string &network_shp)
{
    // Load network
    fmm::Network network(network_shp, "fid", "u", "v");
    fmm::NetworkGraph graph(network);
    fmm::UBODT ubodt(ubodt_file);

    fmm::FastMapMatch fmm_model(network, graph, ubodt);
    fmm::FastMapMatchConfig config(8, 0.003, 0.0005);

    fmm::Trajectory traj(0, pts);
    auto result = fmm_model.match_traj(traj, config);

    double len = 0.0;
    for (const auto &e : result.cpath)
        len += network.get_edge(e).length;

    double err = 0.0;
    for (size_t i = 0; i < pts.size(); ++i)
        err += result.mgeom.distance(pts[i]);
    if (!pts.empty())
        err /= pts.size();

    return {result.mgeom, len, err};
}

// -----------------------------------------------------------------------------
// 4) Brute-force search a tile (very small demo)
// -----------------------------------------------------------------------------
void search_best(const std::vector<fmm::Point> &art, const std::string &ubodt, const std::string &net,
                 double center_lat, double center_lon, double step_km = 2.0, int grid = 5)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> jitter(-1.0, 1.0);

    MatchResult best;
    best.error = std::numeric_limits<double>::max();

    for (int dx = -grid; dx <= grid; ++dx)
        for (int dy = -grid; dy <= grid; ++dy)
        {
            double try_lat = center_lat + dy * step_km / 111.32;
            double try_lon = center_lon + dx * step_km / (111.32 * std::cos(center_lat * M_PI / 180.0));

            auto wgs = geo_place(art, try_lat, try_lon);
            auto mr = match_trajectory(wgs, ubodt, net);

            // Simple score: short but faithful
            double score = mr.error + 0.0001 * mr.length_m;
            if (score < best.error)
            {
                best = mr;
                std::cout << "new best score " << score << " @ " << try_lat << "," << try_lon << "\n";
            }
        }

    // 5) Dump GPX
    std::ofstream gpx("route.gpx");
    gpx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<gpx version=\"1.1\">\n<trk><trkseg>\n";
    for (const auto &p : best.geom.get_points())
        gpx << "<trkpt lat=\"" << p.y << "\" lon=\"" << p.x << "\"/>\n";
    gpx << "</trkseg></trk></gpx>\n";
    std::cout << "Saved route.gpx (" << best.length_m / 1000.0 << " km)\n";
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cerr << "Usage: ./strava_art image.png center_lat center_lon\n";
        return 1;
    }

    auto pts = skeleton_to_points(argv[1]);
    if (pts.empty())
    {
        std::cerr << "No drawable pixels found.\n";
        return 1;
    }
    std::cout << "Skeleton has " << pts.size() << " points.\n";

    const std::string ubodt = "data/ubodt.txt";
    const std::string net = "data/edges.shp";

    search_best(pts, ubodt, net, std::stod(argv[2]), std::stod(argv[3]));

    return 0;
}
