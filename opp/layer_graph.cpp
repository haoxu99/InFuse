#include "layer_graph.h"

Layer_Graph::Layer_Graph(const Data& data)//
{
	graph.total_node_num = data.total_node_num;
	graph.in_degree.resize(graph.total_node_num, 0);
	graph.out_degree.resize(graph.total_node_num, 0);
	graph.node_visited.resize(graph.total_node_num, false);
	graph.edge_deleted.resize(graph.total_node_num * graph.total_node_num, false);
	this->data = data;
}

Layer_Graph::~Layer_Graph()
{

}

void Layer_Graph::BuildLayerGraph(nozzle the_nozzle)
{
	clock_t start_time, end_time;
	start_time = clock();
	vector<pair<int, int>> temp_collision_edges;

	vector<pair<int, int>> temp_injection_edges;
	vector<pair<int, int>> all_id_layers;
	cv::Point2d dir;

	//	//get (i-1) layer segment's contour

	//	// build graph

	//Update 2024.11.27: use clipper to detect normal dependency edges

	//	//get (i-1) layer segment's contour

	//		// get (i) layer segment's contour

	Path poly;
	Path subj;
	IntPoint pt;

	// Index z values to avoid scanning all layer pairs.
	std::map<int, std::vector<std::pair<int, int>>> z_index; // key = round(z*10)
	for (int i = 0; i < data.slice_points.size(); i++) {
		for (int j = 0; j < data.slice_points[i].size(); j++) {
			int z_key = (int)round(data.z_value[i][j][0] * 10);
			z_index[z_key].push_back(std::make_pair(i, j));
		}
	}

	for (int i = 0; i < data.slice_points.size(); i++) {
		for (int j = 0; j < data.slice_points[i].size(); j++) {
			subj.clear();
			for (int m = 0; m < data.slice_points[i][j].size(); m++) {
				pt.X = data.slice_points[i][j][m].x * 1000;
				pt.Y = data.slice_points[i][j][m].y * 1000;
				subj.push_back(pt);
			}

			// Consider only nodes at the current z plus dh.
			double target_z = data.z_value[i][j][0] + dh;
			int target_key = (int)round(target_z * 10);

			// Use the nearest key to the target z to tolerate rounding error.
			for (int dk = -1; dk <= 1; dk++) {
				auto it = z_index.find(target_key + dk);
				if (it == z_index.end()) continue;

				for (auto& pair_ij : it->second) {
					int ii = pair_ij.first;
					int jj = pair_ij.second;
					if (abs(data.z_value[ii][jj][0] - data.z_value[i][j][0] - dh) >= 0.1)
						continue;

					poly.clear();
					for (int m = 0; m < data.slice_points[ii][jj].size(); m++) {
						pt.X = data.slice_points[ii][jj][m].x * 1000;
						pt.Y = data.slice_points[ii][jj][m].y * 1000;
						poly.push_back(pt);
					}
					Clipper c;
					c.AddPath(subj, ptSubject, true);
					c.AddPath(poly, ptClip, true);
					Paths solution;
					c.Execute(ctIntersection, solution, pftNonZero, pftNonZero);
					if (!solution.empty()) {
						graph.AddEdge(data.index_inv[std::make_pair(i, j)], data.index_inv[std::make_pair(ii, jj)]);
						temp_edges.push_back(make_pair(data.index_inv[std::make_pair(i, j)], data.index_inv[std::make_pair(ii, jj)]));
						all_id_layers.push_back(make_pair(i, ii));
						dependency_edges.push_back(make_pair(data.index_inv[std::make_pair(i, j)], data.index_inv[std::make_pair(ii, jj)]));
						solution.clear();
					}
				}
			}
		}
	}

	cont_normal_dependency_edges = temp_edges.size();

	int count = 0;
	for (int i = 0; i < data.slice_points.size(); i++) {
		for (int j = 0; j < data.slice_points[i].size(); j++) {
			count++;
		}
	}
	cout << "the number of layers: " << count << endl;

	cout << "the number of dependency edges: " << temp_edges.size() << endl;

	//collision detect, add dependency edge
	Collision_Detect(temp_collision_edges, the_nozzle);

	//injection detect, add dependency edge
	Injection_Detect(temp_injection_edges, the_nozzle);

	//add collision and injection edges
	for (int i = 0; i < temp_collision_edges.size(); i++) {
		graph.AddEdge(temp_collision_edges[i].first, temp_collision_edges[i].second);
		all_id_layers.push_back(make_pair(temp_collision_edges[i].first, temp_collision_edges[i].second));
		temp_edges.push_back(temp_collision_edges[i]);
		collision_edges.push_back(temp_collision_edges[i]);
		constraint_edges.push_back(temp_collision_edges[i]);
	}

	for (int i = 0; i < temp_injection_edges.size(); i++) {
		graph.AddEdge(temp_injection_edges[i].first, temp_injection_edges[i].second);
		all_id_layers.push_back(make_pair(temp_injection_edges[i].first, temp_injection_edges[i].second));
		temp_edges.push_back(temp_injection_edges[i]);
		injection_edges.push_back(temp_injection_edges[i]);
		constraint_edges.push_back(temp_injection_edges[i]);
	}

	cout << "the number of all dependency edges: " << temp_edges.size() << endl;

	end_time = clock();
	cout << "&&&&&&& time of establish dependency graph (contain collision detection): " << double(end_time - start_time) / CLOCKS_PER_SEC << "s &&&&&&&" << endl;

	OutputInjectionLayer();// Export injection information.
}

void Layer_Graph::OutputInjectionLayer() {
	// Export injection information.
	std::ofstream file("Injection_Layer.txt");
	file << graph.total_node_num << endl;
	for (int i = 0; i < graph.total_node_num; i++) {
		if (I[i] == 1) {// For injection layers, export the index, type, center, and height.
			Polygon injection_layer_polygons = data.slice_points[data.index[i].first][data.index[i].second];
			cv::Point2d center = injection_layer_polygons.find_max_circle().center;
			file << I[i] << endl;
			file << center.x << " " << center.y << " " << data.z_value[data.index[i].first][data.index[i].second][0] << endl;

		}
		else
		{// For other layers, export the index and type.
			file << I[i] << endl;
		}
	}

	file.close();
}

void Layer_Graph::Collision_Detect(vector<pair<int, int>>& temp_collision_edges, nozzle the_nozzle) {
	vector<pair<int, int>> temp1_collision_edges;
	for (int i = 0; i < data.slice_points.size(); i++) {
		for (int j = 0; j < data.slice_points[i].size(); j++) {
			for (int ii = i + 1; ii < data.slice_points.size(); ii++) {
				double circle_r;
				if ((ii - i) * dh < the_nozzle.nozzle_H_half)
					circle_r = the_nozzle.lowwer_surface_r + (ii - i) * dh * (the_nozzle.upper_surface_r - the_nozzle.lowwer_surface_r) / the_nozzle.nozzle_H_half;
				else
					circle_r = the_nozzle.upper_surface_r;

				for (int jj = 0; jj < data.slice_points[ii].size(); jj++) {
					bool jud_collision = false;
					if ((ii - i) * dh > the_nozzle.nozzle__H_total) //exceed nozzle_H
					{
						jud_collision = true;
						temp1_collision_edges.push_back(make_pair(data.index_inv[std::make_pair(i, j)], data.index_inv[std::make_pair(ii, jj)]));
						continue;
					}
					for (int k = 0; k < data.slice_points[i][j].size(); k += 20) { //step == 2
						for (int kk = 0; kk < data.slice_points[ii][jj].size(); kk += 20) {
							if (pow(data.slice_points[ii][jj][kk].x - data.slice_points[i][j][k].x, 2) + pow(data.slice_points[ii][jj][kk].y - data.slice_points[i][j][k].y, 2) - pow(circle_r, 2) < 0) {
								jud_collision = true;
								break;
							}
						}
						if (jud_collision == true)
							break;
					}
					if (jud_collision == true) {
						temp1_collision_edges.push_back(make_pair(data.index_inv[std::make_pair(i, j)], data.index_inv[std::make_pair(ii, jj)]));
					}
				}
			}
		}
	}

	// Remove redundant collision edges.
	std::vector<std::vector<bool>> dependency_relationship(data.total_node_num,
	    std::vector<bool>(data.total_node_num, false));

	for (int i = 1; i < data.slice_points.size(); i++) {
		for (int j = 0; j < data.slice_points[i].size(); j++) {
			int id_layer = i;
			vector<int> id_current_layer;
			vector<int> temp_id_current_layer;
			id_current_layer.push_back(data.index_inv[std::make_pair(i, j)]);
			while (id_layer > 0) {
				for (int k = 0; k < temp_edges.size(); k++) {
					for (int m = 0; m < id_current_layer.size(); m++) {
						if (temp_edges[k].second == id_current_layer[m]) {
							dependency_relationship[temp_edges[k].first][data.index_inv[std::make_pair(i, j)]] = true;
							temp_id_current_layer.push_back(temp_edges[k].first);
							break;
						}
					}
				}
				id_layer--;
				id_current_layer = temp_id_current_layer;
				temp_id_current_layer.clear();
			}
		}
	}

	for (int i = 0; i < temp1_collision_edges.size(); i++) {
		if (dependency_relationship[temp1_collision_edges[i].first][temp1_collision_edges[i].second] == false) {
			dependency_relationship[temp1_collision_edges[i].first][temp1_collision_edges[i].second] = true;
			for (int j = 0; j < data.total_node_num; j++) {   //update
				if (dependency_relationship[temp1_collision_edges[i].second][j] == true) {
					dependency_relationship[temp1_collision_edges[i].first][j] = true;
				}
			}
			temp_collision_edges.push_back(temp1_collision_edges[i]);
		}
	}

	//print collision edges
	cout << "the number of collision edges: " << temp_collision_edges.size() << endl;
}

void Layer_Graph::Injection_Detect(vector<pair<int, int>>& temp_injection_edges, nozzle the_nozzle) {

	vector<int> injection_layer_id;	//the id of injection layer
	vector<Polygon> injection_layer_polygons;	//the polygons of injection layer

	//get leaf notes id
	for (int i = 0; i < graph.out_degree.size(); i++) {
		if (graph.out_degree[i] == 0) {
			cout << "the leaf node id: " << i << endl;
			injection_layer_id.push_back(i);
			injection_layer_polygons.push_back(data.slice_points[data.index[i].first][data.index[i].second]);
			I[i]++;
		}
	}

	//print update before injection polygons imformations

	// Replace a layer with an injection point when its inscribed circle fits the injection radius.

	//				/*if(I[injection_layer_id[i]] == 3)

		// Replace a layer with an injection point when its inscribed circle fits the injection radius.
	for (int i = 0; i < injection_layer_id.size(); i++) {

		while (injection_layer_polygons[i].find_max_circle().radius <= the_nozzle.injection_size && graph.out_degree[injection_layer_id[i]] <= 1) {

			bool found = false;
			for (int m = 0; m < graph.edges.size(); m++) {
				if (graph.edges[m].GetTo() == injection_layer_id[i]) {
					I[injection_layer_id[i]]++;
					injection_layer_id[i] = graph.edges[m].GetFrom();
					injection_layer_polygons[i] = data.slice_points[data.index[injection_layer_id[i]].first][data.index[injection_layer_id[i]].second];
					I[injection_layer_id[i]]++;
					found = true;
					break;  // Stop after finding one incoming edge.
				}
			}
			if (!found) break;  // Stop when no incoming edges remain.
		}
	}

	//print update after injection polygons imformations

	// Build dependencies within the inner model.
	//add injection dependency edge
	for (int i = 0; i < injection_layer_id.size(); i++) {
		for (int j = 0; j < graph.total_node_num; j++) {
			if ((data.z_value[data.index[injection_layer_id[i]].first][data.index[injection_layer_id[i]].second][0] == data.z_value[data.index[j].first][data.index[j].second][0] && graph.out_degree[j] != 0)
				|| (data.z_value[data.index[injection_layer_id[i]].first][data.index[injection_layer_id[i]].second][0] > data.z_value[data.index[j].first][data.index[j].second][0] && graph.out_degree[j] == 0)
				) {
				if(injection_layer_id[i]!= j) // A layer cannot depend on itself.
					temp_injection_edges.push_back(make_pair(j, injection_layer_id[i]));
			}
		}
	}

	//print injection edges
	cout<<"the number of injection edges: "<<temp_injection_edges.size()<<endl;

}

void Layer_Graph::GetInitialOPP()
{
	clock_t start_time, end_time;
	start_time = clock();

	int d_num = 0;
	for (int u = 0; u < graph.total_node_num; u++) {	//delete edge of two in_degrees or out_degrees.
		for (int i = 0; i < graph.G[u].size(); i++) {
			int v = graph.edges[graph.G[u][i]].GetTo(); d_num++;
			if (graph.in_degree[v] >= 2 || graph.out_degree[u] >= 2) {
				graph.edge_deleted[graph.G[u][i]] = true;
				d_num++;
			}
		}
	}
	std::cout << "d_num: " << d_num << std::endl;
	for (int i = 0; i < graph.edges.size(); i++) {
		if (graph.edge_deleted[i]) {
			int u = graph.edges[i].GetFrom();
			int v = graph.edges[i].GetTo();

			graph.in_degree[v]--;
			graph.out_degree[u]--;
		}
	}
	for (int i = 0; i < graph.total_node_num; i++) {
		if ((graph.node_visited[i] == false) && (graph.in_degree[i] == 0)) {

			graph.UpdateDegree(i, -1);
			std::vector<int> initial_opp;
			initial_opp.push_back(i);
			graph.node_visited[i] = true;
			DFS(i, initial_opp);
			this->initial_opp_info.push_back(initial_opp);
			count = 0;
		}
	}

	end_time = clock();
	std::cout << "***initial opp num***: " << this->initial_opp_info.size() << std::endl << std::endl;
	std::cout << "&&&&&&& time of establish initial opp graph: " << double(end_time - start_time) / CLOCKS_PER_SEC << "s &&&&&&&" << std::endl;

#if DEFAULT_PRINTING_DIRECTION
	std::cout << "initial opp num: " << this->initial_opp_info.size() << std::endl;
#else

#endif
	this->OutputInitialOpp(file_name + "_initial_opp" + suffix_txt);
}

void Layer_Graph::DFS(int u, std::vector<int>& initial_opp)
{
	for (int i = 0; i < graph.G[u].size(); i++) {
		int v = graph.edges[graph.G[u][i]].GetTo();
		if (graph.edge_deleted[graph.G[u][i]]) continue;
		if (graph.node_visited[v]) continue;
		if (graph.in_degree[v] != 0) continue;

		//judge whether the edge is a contour and segment

		graph.UpdateDegree(v, -1);
		graph.node_visited[v] = true;
		initial_opp.push_back(v);
		count++;
		DFS(v, initial_opp);
		break;

	}
}

void Layer_Graph::OutputInitialOpp(const std::string& file_name)
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
