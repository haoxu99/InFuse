#pragma once

#ifndef TRA_Graph_H_
#define TRA_Graph_H_

#include "polygon.h"
#include "graph.h"
#include "data.h"

#include     <queue>
#include   <cstring>
#include <algorithm>
#include <limits>
#include <tuple>
#include <unordered_map>
#include<cmath>
#include <float.h>
#include "../slice/clipper.hpp"
#include "../slice/Vector3.h"

#define use_int32
using namespace ClipperLib;
typedef Vector3 Vec3;


class TRA_Graph : public Graph
{
public:
	TRA_Graph(const Data& data, const std::vector<std::vector<int>>& initial_opp_info, std::vector<std::pair<int, int>>& denpendency_edges, std::vector<std::pair<int, int>>& constraint_edges);
	~TRA_Graph();
	void BuildTRAGraph();

	void GeneratePath();
	void TopologicalSort_BeamSearch();
	void generate_dot();
	std::vector<std::vector<int>> medium_tra_info;
private:

	Data data;
	std::vector<std::vector<int>> initial_tra_info;
	std::vector<std::vector<bool>> has_edge;
	std::vector<std::vector<bool>> has_constraint_edge;

	std::vector<std::pair<int, int>> temp_edges;

	std::vector<std::tuple<int, int, double>> weight_edges;

	int cont_normal_dependency_edges;

	bool IsDepend(int i, int j);
	bool IsDepend_Constraint(int i, int j);
	double Constraint_Distance(int i, int j);
	std::vector<std::pair<int, int>> denpendency_edges;
	std::vector<std::pair<int, int>> constraint_edges;

	void InjectionLayerUpdate();// Update injection layer order after reordering patches.

};

#endif
