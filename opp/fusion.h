#pragma once
#ifndef FUSION_H_
#define FUSION_H_

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

class Fusion
{
public:
	Fusion(const Data& data_outer, const Data& data_inner);
	~Fusion();

	void BuildGraph(nozzle the_nozzle);
	void Fusion_Graph(nozzle the_nozzle);

	std::vector<Data> data_fusion;
	std::vector<Graph> graph_fusion;
	int total_node_num_fusion;
	vector<pair<int, int>> dependency_edges;

	vector<pair<int, int>> collision_edges;
	vector<pair<int, int>> constraint_edges;

private:
	vector<pair<int, int>> temp_edges;
	vector<pair<int, int>> injection_edges;
	Data data_outer;
	Data data_inner;
	int total_node_num_outer;
	int total_node_num_inner;
	Graph graph_outer;
	Graph graph_inner;
	vector<pair<int, int>> dependency_edges_inner;
	vector<pair<int, int>> dependency_edges_outer;
	bool IsDepend(int i, int j,bool is_outer);
	void Injection_Detect(vector<pair<int, int>>& temp_injection_edges, nozzle the_nozzle);
	void Collision_Detect(vector<pair<int, int>>& temp_collision_edges, nozzle the_nozzle);
	int I[10000] = { 0 };// Layer types: 0 normal, 1 injection, 2 solid, -1 outer model.
	void OutputInjectionLayer();
};

#endif // !LAYER_GRAPG_H_
