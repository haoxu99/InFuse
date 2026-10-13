#include "Merge_Graph.h"

Merge_Graph::Merge_Graph(const Data& data, vector<pair<int, int>> dependency_edges, vector<pair<int, int>> constraint_edges) {
	this->data = data;

	this->dependency_edges = dependency_edges;
	this->constraint_edges = constraint_edges;

	graph.total_node_num = data.total_node_num;
	graph.in_degree.resize(graph.total_node_num, 0);
	graph.out_degree.resize(graph.total_node_num, 0);
	graph.node_visited.resize(graph.total_node_num, false);
	graph.edge_deleted.resize(graph.total_node_num * graph.total_node_num, false);

}

Merge_Graph::~Merge_Graph() {

}

// Merge patches from bottom to top.

//#if DEFAULT_PRINTING_DIRECTION

//#else
//#endif

//			static_cast<double>(phi_ij) / c2.allDependees.size()) +

//			static_cast<double>(psi_ij) / c2.allDependers.size())

//		do {

//						currentClusters[i].insert(
//							currentClusters[i].end(),
//							currentClusters[j].begin(),
//							currentClusters[j].end()

//	do {

//					clusters[i].insert(
//						clusters[i].end(),
//						clusters[j].begin(),
//						clusters[j].end()

//	 do{

void Merge_Graph::Merge()
{
	std::cout << graph.total_node_num << std::endl;

	clock_t start_time, end_time;
	start_time = clock();

	for (int i = 0; i < dependency_edges.size(); i++) {// Add dependency edges first.
		graph.AddEdge(dependency_edges[i].first, dependency_edges[i].second);
	}

	int d_num = 0;

	for (int u = 0; u < graph.total_node_num; u++) {	//delete edge of two in_degrees or out_degrees.
		for (int i = 0; i < graph.G[u].size(); i++) {
			int v = graph.edges[graph.G[u][i]].GetTo(); d_num++;
			if (graph.in_degree[v] >= 2 || graph.out_degree[u] >= 2) {
				graph.edge_deleted[graph.G[u][i]] = true;
				d_num++;
			}

			bool flag1 = false;
			bool flag2 = false;
			bool flag3 = false;
			int u1, u2;
			for (int j = 0; j < constraint_edges.size(); j++) {
				if (constraint_edges[j].second == u) {
					flag1 = true;
					u1 = constraint_edges[j].first;
				}
				if (constraint_edges[j].second == v) {
					flag2 = true;
					u2 = constraint_edges[j].first;
				}
			}
			if (flag1 && flag2) {
				for (int n = 0; n < dependency_edges.size(); n++) {
					if (dependency_edges[n].first == u1 && dependency_edges[n].second == u2) {
						flag3 = true;
					}
				}
			}

			if ((flag1 ^ flag2 ) || (flag1 && flag2 && !flag3)) {
				graph.edge_deleted[graph.G[u][i]] = true;
				d_num++;
			}

			flag1 = false;
			flag2 = false;
			flag3 = false;
			int u3, u4;
			for (int j = 0; j < constraint_edges.size(); j++) {
				if (constraint_edges[j].first == u) {
					flag1 = true;
					u3 = constraint_edges[j].second;
				}
				if (constraint_edges[j].first == v) {
					flag2 = true;
					u4 = constraint_edges[j].second;
				}
			}
			if (flag1 && flag2) {
				for (int n = 0; n < dependency_edges.size(); n++) {
					if (dependency_edges[n].first == u3 && dependency_edges[n].second == u4) {
						flag3 = true;
					}
				}
			}

			if ((flag1 ^ flag2) || (flag1 && flag2 &&!flag3)) {
				graph.edge_deleted[graph.G[u][i]] = true;
				d_num++;
			}
		}
	}

	// Add constraint edges.
	for (int i = 0; i < constraint_edges.size(); i++) {
		graph.AddEdge(constraint_edges[i].first, constraint_edges[i].second);
	}

	for (int i = 0; i < graph.edges.size(); i++) {
		if (graph.edge_deleted[i]) {
			int u = graph.edges[i].GetFrom();
			int v = graph.edges[i].GetTo();

			graph.in_degree[v]--;
			graph.out_degree[u]--;
		}
	}

	std::cout << "d_num: " << d_num << std::endl;

	for (int i = 0; i < graph.total_node_num; i++) {
		if ((graph.node_visited[i] == false) && (graph.in_degree[i] == 0)) {

			graph.UpdateDegree(i, -1);
			std::vector<int> initial_opp;
			initial_opp.push_back(i);
			graph.node_visited[i] = true;
			DFS(i, initial_opp);
			this->initial_opp_info.push_back(initial_opp);
			count = 0;
			i = 0;
		}
	}

	// Control the merging strength.
	std::vector<std::vector<int>> temp2_opp_info = this->initial_opp_info;
	std::vector<int> temp1_opp_info;
	this->initial_opp_info.clear();

	for (int i = 0; i < temp2_opp_info.size(); i++) {

		int threshold = std::ceil(temp2_opp_info[i].size() / std::round((temp2_opp_info[i].size() - 1) * (1 - sigma) + 1)) - 1;
		int count1 = 0;
		for (int j = 0; j < temp2_opp_info[i].size(); j++) {

			temp1_opp_info.push_back(temp2_opp_info[i][j]);
			if (count1 == threshold) {
				this->initial_opp_info.push_back(temp1_opp_info);
				temp1_opp_info.clear();
				count1 = 0;
				continue;
			}
			count1++;
		}
		if (temp1_opp_info.size() != 0) {
			this->initial_opp_info.push_back(temp1_opp_info);
			temp1_opp_info.clear();
		}
	}

	end_time = clock();
	std::cout << "***initial opp num***: " << this->initial_opp_info.size() << std::endl << std::endl;
	std::cout << "&&&&&&& time of graph clutering: " << double(end_time - start_time) / CLOCKS_PER_SEC << "s &&&&&&&" << std::endl;
	std::cout<<"&&&&&&& time of establish initial opp graph(vertex): " << double(end_time - start_time) / CLOCKS_PER_SEC << "s &&&&&&&" << endl;
#if DEFAULT_PRINTING_DIRECTION
	std::cout << "initial opp num: " << this->initial_opp_info.size() << std::endl;
#else

#endif
	this->OutputMerge(file_name + "_initial_opp" + suffix_txt);
}

void Merge_Graph::DFS(int u, std::vector<int>& initial_opp)
{
	for (int i = 0; i < graph.G[u].size(); i++) {
		int v = graph.edges[graph.G[u][i]].GetTo();
		if (graph.edge_deleted[graph.G[u][i]]) continue;
		if (graph.node_visited[v]) continue;
		if (graph.in_degree[v] != 0) continue;
		bool flag = false;
		for (int j = 0; j < constraint_edges.size(); j++) {
			if (constraint_edges[j].first == u && constraint_edges[j].second == v) {
				flag = true;
			}
		}
		if (flag) continue;

		//judge whether the edge is a contour and segment

		graph.UpdateDegree(v, -1);
		graph.node_visited[v] = true;
		initial_opp.push_back(v);
		count++;
		DFS(v, initial_opp);
		break;

	}
}

void Merge_Graph::OutputMerge(const std::string& file_name)
{
	std::ofstream dstream(file_name.c_str());
	if (!dstream.is_open()) {
		std::cout << "can not open " << file_name << std::endl;
		return;
	}
	dstream << initial_opp_info.size() << std::endl;
	for (int i = 0; i < initial_opp_info.size(); i++) {
		dstream << initial_opp_info[i].size() << std::endl;
		int m, n;
		for (int j = 0; j < initial_opp_info[i].size(); j++) {
			m = data.index[initial_opp_info[i][j]].first;
			n = data.index[initial_opp_info[i][j]].second;
			dstream << data.slice_points[m][n].size() << std::endl;
			for (int k = 0; k < data.slice_points[m][n].size(); k++) {
				dstream << this->data.slice_points[m][n][k].x << " " << this->data.slice_points[m][n][k].y << " " << data.z_value[m][n][k] << std::endl;
			}
		}
	}
	dstream.close();  // Close the stream to flush output and release resources.

}
