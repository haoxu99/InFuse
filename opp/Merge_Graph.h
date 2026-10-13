#pragma once
#ifndef MERGE_GRAPH_H_
#define MERGE_GRAPH_H_

#include "polygon.h"
#include "graph.h"
#include "data.h"
#include "../slice/clipper.hpp"

#include     <set>
#include  <vector>
#include  <string>
#include "../slice/visual.h"

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <algorithm>
#define use_int32
using namespace ClipperLib;

class Merge_Graph
{
public:
	Merge_Graph(const Data& data,  vector<pair<int, int>> dependency_edges, vector<pair<int, int>> constraint_edges);
	~Merge_Graph();

	void Merge();
	std::vector<std::vector<int>> initial_opp_info;
private:
	Data data;
	Graph graph;

	vector<pair<int, int>> constraint_edges;
	vector<pair<int, int>> dependency_edges;
	void DFS(int u, std::vector<int>& initial_opp);
	void OutputMerge(const std::string& file_name);
	int count;

};

#endif
