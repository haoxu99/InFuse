#include "fusion.h"
#include <stdexcept>

Fusion::Fusion(const Data& data_outer, const Data& data_inner) {
	this->data_outer = data_outer;
	this->data_inner = data_inner;
	this->total_node_num_outer = data_outer.total_node_num;
	this->total_node_num_inner = data_inner.total_node_num;
	this->total_node_num_fusion = this->total_node_num_outer + this->total_node_num_inner;
	if (this->total_node_num_fusion > maxn) throw std::runtime_error("Fused model exceeds the 10000-node graph capacity.");

	this->graph_outer.total_node_num = this->total_node_num_outer;
	this->graph_outer.in_degree.resize(this->total_node_num_outer, 0);
	this->graph_outer.out_degree.resize(this->total_node_num_outer, 0);
	this->graph_outer.node_visited.resize(this->total_node_num_outer, false);
	this->graph_outer.edge_deleted.resize(this->total_node_num_outer * this->total_node_num_outer, false);

	this->graph_inner.total_node_num = this->total_node_num_inner;
	this->graph_inner.in_degree.resize(this->total_node_num_inner, 0);
	this->graph_inner.out_degree.resize(this->total_node_num_inner, 0);
	this->graph_inner.node_visited.resize(this->total_node_num_inner, false);
	this->graph_inner.edge_deleted.resize(this->total_node_num_inner * this->total_node_num_inner, false);

	this->data_fusion.resize(1);
	this->graph_fusion.resize(1);
	this->data_fusion[0].total_node_num = this->total_node_num_fusion;

	this->graph_fusion[0].total_node_num = this->total_node_num_fusion;
	this->graph_fusion[0].in_degree.resize(this->total_node_num_fusion, 0);
	this->graph_fusion[0].out_degree.resize(this->total_node_num_fusion, 0);
	this->graph_fusion[0].node_visited.resize(this->total_node_num_fusion, false);
	this->graph_fusion[0].edge_deleted.resize(this->total_node_num_fusion * this->total_node_num_fusion, false);
}

Fusion::~Fusion() {

}

void Fusion::BuildGraph(nozzle the_nozzle)
{
	clock_t start_time, end_time;
	start_time = clock();
	// Build the outer graph.
	for (int i = 0; i < this->total_node_num_outer; i++) {
		for (int j = 0; j < this->total_node_num_outer; j++) {
			if (i == j) continue;
			if (IsDepend(i, j, true)) {
				this->graph_outer.AddEdge(i, j);
				dependency_edges_outer.push_back(make_pair(i, j));
			}
		}
	}
	// Build the inner graph.
	for (int i = 0; i < this->total_node_num_inner; i++) {
		for (int j = 0; j < this->total_node_num_inner; j++) {
			if (i == j) continue;
			if (IsDepend(i, j, false)) {
				this->graph_inner.AddEdge(i, j);
				dependency_edges_inner.push_back(make_pair(i, j));
			}
		}
	}
	vector<pair<int, int>> temp_injection_edges;

	Injection_Detect(temp_injection_edges, the_nozzle);
	for (int i = 0; i < temp_injection_edges.size(); i++) {
		this->graph_inner.AddEdge(temp_injection_edges[i].first, temp_injection_edges[i].second);
		injection_edges.push_back(temp_injection_edges[i]);
	}

	std::cout << "number of edges in outer:" << graph_outer.edges.size() << std::endl;
	std::cout << "number of edges in inner:" << graph_inner.edges.size() << std::endl;

	std::cout << "number of nodes in outer:" << graph_outer.total_node_num << std::endl;
	std::cout << "number of nodes in inner:" << graph_inner.total_node_num << std::endl;
	end_time = clock();
	std::cout << "&&&&&&& time of establish initial opp graph: " << double(end_time - start_time) / CLOCKS_PER_SEC << "s &&&&&&&" << std::endl;
}

void Fusion::Injection_Detect(vector<pair<int, int>>& temp_injection_edges, nozzle the_nozzle) {

	vector<int> injection_layer_id;	//the id of injection layer
	vector<Polygon> injection_layer_polygons;	//the polygons of injection layer

	//get leaf notes id
	for (int i = 0; i < graph_inner.out_degree.size(); i++) {
		if (graph_inner.out_degree[i] == 0) {
			injection_layer_id.push_back(i);
			injection_layer_polygons.push_back(data_inner.slice_points[data_inner.index[i].first][data_inner.index[i].second]);
			I[i]++;
		}
	}

	//print update before injection polygons imformations

	// Replace a layer with an injection point when its inscribed circle fits the injection radius.
	for (int i = 0; i < injection_layer_id.size(); i++) {

		while (injection_layer_polygons[i].find_max_circle().radius <= the_nozzle.injection_size && graph_inner.out_degree[injection_layer_id[i]] <= 1) {
			bool moved_to_predecessor = false;
			for (int m = 0; m < graph_inner.edges.size(); m++) {
				if (graph_inner.edges[m].GetTo() == injection_layer_id[i]) {
					I[injection_layer_id[i]]++;
					injection_layer_id[i] = graph_inner.edges[m].GetFrom();
					injection_layer_polygons[i] = data_inner.slice_points[data_inner.index[injection_layer_id[i]].first][data_inner.index[injection_layer_id[i]].second];
					I[injection_layer_id[i]]++;
					moved_to_predecessor = true;
				}
			}
			if (!moved_to_predecessor) break;
		}
	}

	//print update after injection polygons imformations

	// Build dependencies within the inner model.
	//add injection dependency edge
	for (int i = 0; i < injection_layer_id.size(); i++) {
		for (int j = 0; j < total_node_num_inner; j++) {
			if ((data_inner.z_value[data_inner.index[injection_layer_id[i]].first][data_inner.index[injection_layer_id[i]].second][0] == data_inner.z_value[data_inner.index[j].first][data_inner.index[j].second][0] && graph_inner.out_degree[j] != 0)
				|| (data_inner.z_value[data_inner.index[injection_layer_id[i]].first][data_inner.index[injection_layer_id[i]].second][0] > data_inner.z_value[data_inner.index[j].first][data_inner.index[j].second][0] && graph_inner.out_degree[j] == 0)
				) {
				if (injection_layer_id[i] != j) // A layer cannot depend on itself.
					temp_injection_edges.push_back(make_pair(j, injection_layer_id[i]));
			}
		}
	}

	//print injection edges
	cout << "the number of injection edges: " << temp_injection_edges.size() << endl;

}

bool Fusion::IsDepend(int i, int j,bool is_outer)
{
	std::vector<Point2d> i_layers;
	std::vector<Point2d> j_layers;
	double H;
	if (is_outer) {
		i_layers = data_outer.slice_points[data_outer.index[i].first][data_outer.index[i].second];
		j_layers = data_outer.slice_points[data_outer.index[j].first][data_outer.index[j].second];
		H = data_outer.z_value[data_outer.index[j].first][data_outer.index[j].second][0] - data_outer.z_value[data_outer.index[i].first][data_outer.index[i].second][0];
	}
	else{
		i_layers = data_inner.slice_points[data_inner.index[i].first][data_inner.index[i].second];
		j_layers = data_inner.slice_points[data_inner.index[j].first][data_inner.index[j].second];
		H = data_inner.z_value[data_inner.index[j].first][data_inner.index[j].second][0] - data_inner.z_value[data_inner.index[i].first][data_inner.index[i].second][0];
	}

	if (abs(H - dh) < 0.1) {
		IntPoint pt;
		Path subj;
		for (int k = 0; k < i_layers.size(); k++) {
			pt = IntPoint(i_layers[k].x * 1000, i_layers[k].y * 1000);
			subj.push_back(pt);
		}

		Path clip;
		for (int k = 0; k < j_layers.size(); k++) {
			pt = IntPoint(j_layers[k].x * 1000, j_layers[k].y * 1000);
			clip.push_back(pt);
		}

		Clipper c;
		c.AddPath(subj, ptSubject, true);
		c.AddPath(clip, ptClip, true);
		Paths solution;
		c.Execute(ctIntersection, solution, pftNonZero, pftNonZero);
		if (!solution.empty()) return true;
	}

	return false;
}

void Fusion::Fusion_Graph(nozzle the_nozzle)
{
	clock_t start_time, end_time;
	start_time = clock();

	// Merge graph nodes.

	// Merge graph edges.

	// Retain the layer types before and after merging.
	std::vector<int> index_outer(data_outer.total_node_num);
	std::vector<int> index_inner(data_inner.total_node_num);

	// Merge graph data; outer contours are treated as continuous paths.
	int num = 0;
	std::vector<int> I_buffer(data_inner.total_node_num, 0);
	for(int i = 0; i < data_inner.total_node_num; i++) {
		I_buffer[i] = I[i];// Temporarily store layer types.
		I[i] = 0;
	}
	for (int i = 0; i < data_outer.slice_points.size(); i++) {
		data_fusion[0].slice_points.push_back(data_outer.slice_points[i]);
		data_fusion[0].z_value.push_back(data_outer.z_value[i]);
		data_fusion[0].is_contour.push_back(data_outer.is_contour[i]);
		for (int j = 0; j < data_outer.slice_points[i].size(); j++) {
			index_outer[data_outer.index_inv[std::make_pair(i, j)]] = num;
			num++;
		}
		for (int ii = 0; ii < data_inner.slice_points.size(); ii++) {
			if (data_outer.z_value[i][0][0] == data_inner.z_value[ii][0][0]) {
				for (int jj = 0; jj < data_inner.slice_points[ii].size(); jj++) {
					data_fusion[0].slice_points[i].push_back(data_inner.slice_points[ii][jj]);
					data_fusion[0].z_value[i].push_back(data_inner.z_value[ii][jj]);
					data_fusion[0].is_contour[i].push_back(data_inner.is_contour[ii][jj]);
					index_inner[data_inner.index_inv[std::make_pair(ii, jj)]] = num;
					num++;
				}
			}
		}
	}

	// Update layer types.
	for (int i = 0; i < data_inner.total_node_num; i++) {
		I[index_inner[i]] = I_buffer[i];
	}
	for (int i = 0; i < data_outer.total_node_num; i++) {
		I[index_outer[i]] = -1;// Mark outer layers with type -1.
	}
	OutputInjectionLayer();// Export injection information.

	std::cout << "the number of fusion points: " << num << std::endl;
	// Update node data.
	num = 0;
	for (int i = 0; i < data_fusion[0].slice_points.size(); i++) {
		for (int j = 0; j < data_fusion[0].slice_points[i].size(); j++) {
			data_fusion[0].index[num] = std::make_pair(i, j);
			data_fusion[0].index_inv[std::make_pair(i, j)] = num;
			num++;
		}
	}

	// Merge graph edges.
	for (int i = 0; i < graph_outer.edges.size(); i++) {
		graph_fusion[0].AddEdge(index_outer[graph_outer.edges[i].GetFrom()], index_outer[graph_outer.edges[i].GetTo()]);
		temp_edges.push_back(make_pair(index_outer[graph_outer.edges[i].GetFrom()], index_outer[graph_outer.edges[i].GetTo()]));
	}
	for (int i = 0; i < graph_inner.edges.size(); i++) {
		graph_fusion[0].AddEdge(index_inner[graph_inner.edges[i].GetFrom()], index_inner[graph_inner.edges[i].GetTo()]);
		temp_edges.push_back(make_pair(index_inner[graph_inner.edges[i].GetFrom()], index_inner[graph_inner.edges[i].GetTo()]));
	}

	// Add collision edges.
	vector<pair<int, int>> temp_collision_edges;
	Collision_Detect(temp_collision_edges, the_nozzle);
	for (int i = 0; i < temp_collision_edges.size(); i++) {
		graph_fusion[0].AddEdge(temp_collision_edges[i].first, temp_collision_edges[i].second);
		temp_edges.push_back(temp_collision_edges[i]);
		collision_edges.push_back(temp_collision_edges[i]);
		constraint_edges.push_back(temp_collision_edges[i]);

	}

	// Update edge data.
	for (int i = 0; i < dependency_edges_outer.size(); i++) {
		dependency_edges.push_back(make_pair(index_outer[dependency_edges_outer[i].first], index_outer[dependency_edges_outer[i].second]));
	}
	for (int i = 0; i < dependency_edges_inner.size(); i++) {
		dependency_edges.push_back(make_pair(index_inner[dependency_edges_inner[i].first], index_inner[dependency_edges_inner[i].second]));
	}
	for (int i = 0; i < injection_edges.size(); i++) {
		injection_edges[i].first = index_inner[injection_edges[i].first];
		injection_edges[i].second = index_inner[injection_edges[i].second];
		constraint_edges.push_back(injection_edges[i]);
	}

	// Detect collisions.

}

void Fusion::Collision_Detect(vector<pair<int, int>>& temp_collision_edges, nozzle the_nozzle) {
	vector<pair<int, int>> temp1_collision_edges;
	for (int i = 0; i < data_fusion[0].slice_points.size(); i++) {
		for (int j = 0; j < data_fusion[0].slice_points[i].size(); j++) {
			for (int ii = i + 1; ii < data_fusion[0].slice_points.size(); ii++) {
				double circle_r;
				if ((ii - i) * dh < the_nozzle.nozzle_H_half)
					circle_r = the_nozzle.lowwer_surface_r + (ii - i) * dh * (the_nozzle.upper_surface_r - the_nozzle.lowwer_surface_r) / the_nozzle.nozzle_H_half;
				else
					circle_r = the_nozzle.upper_surface_r;

				for (int jj = 0; jj < data_fusion[0].slice_points[ii].size(); jj++) {
					bool jud_collision = false;
					if ((ii - i) * dh > the_nozzle.nozzle__H_total) //exceed nozzle_H
					{
						jud_collision = true;
						temp1_collision_edges.push_back(make_pair(data_fusion[0].index_inv[std::make_pair(i, j)], data_fusion[0].index_inv[std::make_pair(ii, jj)]));
						continue;
					}
					for (int k = 0; k < data_fusion[0].slice_points[i][j].size(); k += 10) { //step == 2
						for (int kk = 0; kk < data_fusion[0].slice_points[ii][jj].size(); kk += 10) {
							if (pow(data_fusion[0].slice_points[ii][jj][kk].x - data_fusion[0].slice_points[i][j][k].x, 2) + pow(data_fusion[0].slice_points[ii][jj][kk].y - data_fusion[0].slice_points[i][j][k].y, 2) - pow(circle_r, 2) < 0) {
								jud_collision = true;
								break;
							}
						}
						if (jud_collision == true)
							break;
					}
					if (jud_collision == true) {
						temp1_collision_edges.push_back(make_pair(data_fusion[0].index_inv[std::make_pair(i, j)], data_fusion[0].index_inv[std::make_pair(ii, jj)]));
					}
				}
			}
		}
	}

	// Remove redundant collision edges.
	std::vector<std::vector<bool>> dependency_relationship(data_fusion[0].total_node_num,
	    std::vector<bool>(data_fusion[0].total_node_num, false));

	for (int i = 1; i < data_fusion[0].slice_points.size(); i++) {
		for (int j = 0; j < data_fusion[0].slice_points[i].size(); j++) {
			int id_layer = i;
			vector<int> id_current_layer;
			vector<int> temp_id_current_layer;
			id_current_layer.push_back(data_fusion[0].index_inv[std::make_pair(i, j)]);
			while (id_layer > 0) {
				for (int k = 0; k < temp_edges.size(); k++) {
					for (int m = 0; m < id_current_layer.size(); m++) {
						if (temp_edges[k].second == id_current_layer[m]) {
							dependency_relationship[temp_edges[k].first][data_fusion[0].index_inv[std::make_pair(i, j)]] = true;
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
			for (int j = 0; j < data_fusion[0].total_node_num; j++) {   //update
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

void Fusion::OutputInjectionLayer() {
	// Export injection information.
	std::ofstream file("Injection_Layer.txt");
	file <<graph_fusion[0].total_node_num << endl;
	for (int i = 0; i < graph_fusion[0].total_node_num; i++) {
		if (I[i] == 1) {// For injection layers, export the index, type, center, and height.
			Polygon injection_layer_polygons = data_fusion[0].slice_points[data_fusion[0].index[i].first][data_fusion[0].index[i].second];
			cv::Point2d center = injection_layer_polygons.find_max_circle().center;
			file << I[i] << endl;
			file << center.x << " " << center.y << " " << data_fusion[0].z_value[data_fusion[0].index[i].first][data_fusion[0].index[i].second][0] << endl;

		}
		else
		{// For other layers, export the index and type.
			file << I[i] << endl;
		}
	}

	file.close();
}
