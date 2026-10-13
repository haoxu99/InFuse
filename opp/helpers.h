#pragma once
#ifndef HELPERS_H_
#define HELPERS_H_
#include <cmath>
#include <string>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <opencv2/core/core.hpp>

struct nozzle {
    double upper_surface_r;
    double lowwer_surface_r;
    double nozzle__H_total;
    double nozzle_H_half;
    double injection_size;
};
struct Circle {
    cv::Point2d center;
    double radius;
};
const int maxn = 10000;
const double eps = 1e-10;
const double MAX_D = 1e18;
const double MIN_D = -1e18;
const double dh = 0.2;                       // Layer spacing, in millimeters.
const double Extrusion_width = 0.4;          // Extrusion width, in millimeters.
const double PathOverlap = 0.1;              // Path overlap ratio.
const int times = 1;                         // Number of contour offsets.
const std::pair<double, double> material_ratio = std::make_pair(25, 10);
const bool GapOptimization_support = false;
const double sigma = 1;                      // Graph merging strength, in [0, 1].
const int beam_width = 500;
const double weight_factor = 0.5;            // 0 uses jump count; 1 uses travel distance.
extern std::string file_name;
const std::string suffix_txt = ".txt";

// Retain the original integer-point distance semantics used by the slicer.
inline static double Distance2D(cv::Point p1, cv::Point p2) {
    return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}
#endif
