#pragma once
#ifndef LAYER_GRAPG_H_
#define LAYER_GRAPG_H_

#include "polygon.h"
#include "graph.h"
#include "data.h"
#include "../slice/clipper.hpp"

#include     <set>
#include  <vector>
#include  <string>
#include "../slice/visual.h"

#define use_int32
using namespace ClipperLib;

class Layer_Graph
{
public:
	Layer_Graph(const Data& data);
	~Layer_Graph();
	void BuildLayerGraph(nozzle the_nozzle);

	void GetInitialOPP();
	Data data;
	Graph graph;
	std::vector<std::vector<int>> initial_opp_info;
	std::vector<std::vector<int>> medium_opp_info;
	vector<pair<int, int>> temp_edges;
	int cont_normal_dependency_edges;

	std::vector<pair<int, int>> dependency_edges;
	std::vector<pair<int, int>> injection_edges;
	std::vector<pair<int, int>> collision_edges;
	std::vector<pair<int, int>> constraint_edges;
	std::vector<Circle> circles;
	void Injection_Detect(vector<pair<int, int>>& temp_injection_edges, nozzle the_nozzle);
	void Collision_Detect(vector<pair<int, int>>& temp_collision_edges, nozzle the_nozzle);
private:
	void DFS(int u, std::vector<int>& initial_opp);
	void OutputInitialOpp(const std::string& file_name);
	void OutputInjectionLayer();
	int count = 0;
	int I[10000] = { 0 };// Layer types: 0 normal, 1 injection, 2 solid.
};

#endif // !LAYER_GRAPG_H_
