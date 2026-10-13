#pragma once
#ifndef POLYGON_H
#define POLYGON_H

#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "helpers.h"

class Polygon
{
public:
	Polygon(const std::vector<cv::Point2d>& points);
	~Polygon();

	bool JudgePointInside(cv::Point2d p);
	Circle find_max_circle();
private:
	std::vector<cv::Point2d> points;
	double minx, maxx, miny, maxy;
	int find = 0;
	double min_distance_to_polygon(cv::Point2d p);
	int is_inside_polygon(cv::Point2d p);
	double distance(cv::Point2d a, cv::Point2d b);
};

#endif // !POLYGON_H
