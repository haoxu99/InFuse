#include "polygon.h"

Polygon::Polygon(const std::vector<cv::Point2d>& points)
{
	this->points = points;
}

Polygon::~Polygon()
{
}

Circle Polygon::find_max_circle() {
	double GRID_RESOLUTION = 0.01;// Sample a regular grid.
	Circle max_circle;
	// Find the polygon bounding box.
	double min_x = DBL_MAX, max_x = -DBL_MAX, min_y = DBL_MAX, max_y = -DBL_MAX;
	for (int i = 0; i < points.size(); i++) {
		if (points[i].x < min_x) min_x = points[i].x;
		if (points[i].x > max_x) max_x = points[i].x;
		if (points[i].y < min_y) min_y = points[i].y;
		if (points[i].y > max_y) max_y = points[i].y;
	}
	// Evaluate grid points.
	max_circle.radius = 0.0;
	for (double x = min_x; x <= max_x; x += GRID_RESOLUTION) {
		for (double y = min_y; y <= max_y; y += GRID_RESOLUTION) {
			cv::Point2d p;
			p.x = x;
			p.y = y;

			if (is_inside_polygon(p)) {
				double dist = min_distance_to_polygon(p);
				if (dist > max_circle.radius) {
					max_circle.radius = dist;
					max_circle.center = p;
				}
			}
		}
	}
	return max_circle;
}

// Compute the minimum distance from a point to the polygon boundary.
double Polygon::min_distance_to_polygon(cv::Point2d p) {
	double min_dist = DBL_MAX;
	for (int i = 0; i < points.size(); i++) {
		cv::Point2d v1 = points[i];
		cv::Point2d v2 = points[(i + 1) % points.size()];
		// Project the point onto the edge.
		double t = ((p.x - v1.x) * (v2.x - v1.x) + (p.y - v1.y) * (v2.y - v1.y)) /
			((v2.x - v1.x) * (v2.x - v1.x) + (v2.y - v1.y) * (v2.y - v1.y));
		t = fmax(0, fmin(1, t));
		cv::Point2d projection = { v1.x + t * (v2.x - v1.x), v1.y + t * (v2.y - v1.y) };
		double dist = distance(p, projection);
		if (dist < min_dist) {
			min_dist = dist;
		}
	}
	return min_dist;
}

int Polygon::is_inside_polygon(cv::Point2d p) {
	int count = 0;
	for (int i = 0; i < points.size(); i++) {
		cv::Point2d v1 = points[i];
		cv::Point2d v2 = points[(i + 1) % points.size()];
		if ((p.y > v1.y) != (p.y > v2.y)) {
			double intersection = (p.y - v1.y) * (v2.x - v1.x) / (v2.y - v1.y) + v1.x;
			if (p.x < intersection) {
				count++;
			}
		}
	}
	return count % 2; // An odd number of crossings is inside; an even number is outside.
}

double Polygon::distance(cv::Point2d a, cv::Point2d b) {
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

bool Polygon::JudgePointInside(cv::Point2d p)
{
	if (find == 0) {
		minx = MAX_D;
		maxx = MIN_D;
		miny = MAX_D;
		maxy = MIN_D;
		for (int i = 0; i < points.size(); i++) {
			minx = std::min(points[i].x, minx);
			maxx = std::max(points[i].x, maxx);
			miny = std::min(points[i].y, miny);
			maxy = std::max(points[i].y, maxy);
		}
		find = 1;
	}
	if (p.x < minx || p.x > maxx || p.y < miny || p.y > maxy) return false;
	int i, j, c = 0;
	for (i = 0, j = points.size() - 1; i < points.size(); j = i++) {
		if (((points[i].y > p.y) != (points[j].y > p.y)))
			if(p.x < (points[j].x - points[i].x) * (p.y - points[i].y) / (points[j].y - points[i].y) + points[i].x)
				c = !c;
	}
	return c;
}
