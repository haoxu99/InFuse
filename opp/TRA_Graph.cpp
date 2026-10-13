#include <filesystem>
#include "TRA_Graph.h"
#include <stdexcept>

//Initialization
TRA_Graph::TRA_Graph(const Data& data, const std::vector<std::vector<int>>& initial_opp_info, std::vector<std::pair<int, int>>& denpendency_edges, std::vector<std::pair<int, int>>& constraint_edges) {

	this->data = data;
	this->initial_tra_info = initial_opp_info;
	this->total_node_num = initial_opp_info.size();
	this->in_degree.resize(this->total_node_num, 0);
	this->out_degree.resize(this->total_node_num, 0);
	this->node_visited.resize(this->total_node_num, false);
	this->edge_deleted.resize(this->total_node_num * this->total_node_num, false);

	has_edge.assign(total_node_num, std::vector<bool>(total_node_num, false));
	has_constraint_edge.assign(total_node_num, std::vector<bool>(total_node_num, false));

	this->denpendency_edges = denpendency_edges;
	this->constraint_edges = constraint_edges;

}

TRA_Graph::~TRA_Graph() {

}

void TRA_Graph::BuildTRAGraph()
{
	clock_t start_time, end_time;
	start_time = clock();

	for (int i = 0; i < this->total_node_num; i++) {
		for (int j = 0; j < this->total_node_num; j++) {
			if (i == j) continue;
			if (IsDepend(i, j)) {
				this->has_edge[i][j] = true;

				this->AddWeightEdge(i, j, dh);	//normal depend is layer height
				continue;
			}
			if (IsDepend_Constraint(i, j)) {
				this->has_constraint_edge[i][j] = true;

				this->AddWeightEdge(i, j, Constraint_Distance(i, j));	//Constraint depend is approximate line distance
			}
		}
	}
	std::cout << "number of edges in G_init:" << edges.size() << std::endl;
	end_time = clock();

}

//Calculate distrance of the constraint depend
double TRA_Graph::Constraint_Distance(int i, int j) {

	int id_top_layer_from = initial_tra_info[i][initial_tra_info[i].size() - 1];
	int id_bottom_layer_to = initial_tra_info[j][0];

	std::pair<int, int > index_top_layer_from = data.index[initial_tra_info[i][initial_tra_info[i].size() - 1]];
	std::pair<int, int > index_bottom_layer_to = data.index[initial_tra_info[j][0]];

	double z_value_top_layer_from = data.z_value[index_top_layer_from.first][index_top_layer_from.second][0];
	double z_value_bottom_layer_to = data.z_value[index_bottom_layer_to.first][index_bottom_layer_to.second][0];

	cv::Point2d i_value_top_layer_from = data.slice_points[index_top_layer_from.first][index_top_layer_from.second][0];
	cv::Point2d j_value_bottom_layer_to = data.slice_points[index_bottom_layer_to.first][index_bottom_layer_to.second][0];

	return  sqrt(pow(i_value_top_layer_from.x - j_value_bottom_layer_to.x, 2) + pow(i_value_top_layer_from.y - j_value_bottom_layer_to.y, 2) + pow(z_value_top_layer_from - z_value_bottom_layer_to, 2));

}

bool TRA_Graph::IsDepend(int i, int j)
{

	std::vector<int> i_tra_layers = initial_tra_info[i];
	std::vector<int> j_tra_layers = initial_tra_info[j];
	for (int m = 0; m < i_tra_layers.size(); m++) {
		for (int n = 0; n < j_tra_layers.size(); n++) {
			for (int k = 0; k < denpendency_edges.size(); k++) {
				if (denpendency_edges[k].first == i_tra_layers[m] && denpendency_edges[k].second == j_tra_layers[n]) {
					return true;
				}
			}

		}
	}

	return false;

}

void TRA_Graph::generate_dot() {
	std::vector<std::vector<int>> adj_matrix(this->total_node_num, std::vector<int>(this->total_node_num, 0));
	for (int i = 0; i < this->edges.size(); i++) {
		adj_matrix[this->edges[i].GetFrom()][this->edges[i].GetTo()] = 1;
	}

	std::ofstream file("G.dot");

	file<<"digraph G {"<<std::endl;
	file<<" rankdir=BT;"<<std::endl;
	// Start of the DOT file

	// Traverse the adjacency matrix to generate the graph
	for (int i = 0; i < total_node_num; i++) {
		for (int j = 0; j < total_node_num; j++) {
			if (adj_matrix[i][j] == 1) {
				// If there's an edge from node i to node j
				if (has_edge[i][j]) {
					file << "  " << i << " -> " << j << "[color = black];" << std::endl;
				}
				else if (has_constraint_edge[i][j]) {
					file << "  " << i << " -> " << j << "[color = red];" << std::endl;
				}

			}
		}
	}

	// End of the DOT file
	file<<"}"<<std::endl;
	file.close();

}

bool TRA_Graph::IsDepend_Constraint(int i, int j)
{
	// Update constraints after graph merging.
	for (int m = 0; m < initial_tra_info[i].size(); m++) {
		for (int n = 0; n < initial_tra_info[j].size(); n++) {
			for (int k = 0; k < constraint_edges.size(); k++) {
				if (constraint_edges[k].first == initial_tra_info[i][m] && constraint_edges[k].second == initial_tra_info[j][n]) {
					return true;
				}
			}
		}

	}

	return false;
}

///////Beam search algorithm used shortest path///////////

void TRA_Graph::TopologicalSort_BeamSearch() {
	if (total_node_num == 0) throw std::runtime_error("No patches available for travel planning.");

	clock_t start_time, end_time;
	start_time = clock();

	struct Node {
		std::vector<int> index;
		double score = 0;
		std::vector<int>Node_indegree;

	};

	std::vector<Node> queue;
	std::vector<Node> Seq_buffer;// Keep at most beam_width candidate nodes.
	Node bestSeq;

	bool excced = false;

	// Compute initial travel distance and turning angle.
	double jump_distance_init = 0;
	double jump_count_init = 0;
	for (int i = 1; i < initial_tra_info.size(); i++) {
		if (!has_edge[i - 1][i]) {
			jump_distance_init += Constraint_Distance(i - 1, i);
			jump_count_init++;
		}
		else {
			jump_distance_init += dh;
		}
	}
	std::cout << "Initial jump distance: " << jump_distance_init << std::endl;
	std::cout << "Initial jump count: " << jump_count_init << std::endl;

	//initialize graph
	Node TopoGraph;
	TopoGraph.Node_indegree = in_degree;

	Seq_buffer.push_back(TopoGraph);

	//beam search
	while (true) {

		if (Seq_buffer.empty()) throw std::runtime_error("No topological order exists for the patch constraints.");
		if (Seq_buffer[0].index.size() == total_node_num) break;
		//Beam Search
		printf("Beam search progress: %.2f%%\r", (float)Seq_buffer[0].index.size() / total_node_num * 100);
		for (int i = 0; i < Seq_buffer.size(); i++) {
			for (int j = 0; j < total_node_num; j++) {

				if (Seq_buffer[i].Node_indegree[j] == 0) {
					Node newNode = Seq_buffer[i];
					double distance_jump = 0;
					double count_jump = 0;
					//calculate score
					if (newNode.index.size() >= 1) {

						for (int k = 1; k < newNode.index.size(); k++) {
							if (!has_edge[newNode.index[k - 1]][newNode.index[k]]) {
								distance_jump += Constraint_Distance(newNode.index[k - 1], newNode.index[k]);
								count_jump += 1;
							}
							else {
								distance_jump += dh;
							}
						}

						if (!has_edge[newNode.index[newNode.index.size() - 1]][j]) {
							distance_jump += Constraint_Distance(newNode.index[newNode.index.size() - 1], j);
							count_jump += 1;
						}
						else {
							distance_jump += dh;
						}
						newNode.score = weight_factor * (jump_distance_init > eps ? distance_jump / jump_distance_init : 0.0)
						    + (1 - weight_factor) * (jump_count_init > eps ? count_jump / jump_count_init : 0.0);

						//	//discance

						//else {
						//	//continuity

					}
					newNode.index.push_back(j);
					newNode.Node_indegree[j] = -1;

					for (int k = 0; k < G[j].size(); k++) {
						newNode.Node_indegree[edges[G[j][k]].GetTo()]--;
					}
					queue.push_back(newNode);
				}
			}
		}
		Seq_buffer.clear();
		if (queue.size() > beam_width) {
			excced = true;

			for (int i = 0; i < beam_width; i++) {
				double score = DBL_MAX;
				int index = 0;
				for (int j = 0; j < queue.size(); j++) {
					if (queue[j].score <= score) {
						score = queue[j].score;
						index = j;
					}
				}
				Seq_buffer.push_back(queue[index]);
				queue.erase(queue.begin() + index);
			}

		}
		else {
			Seq_buffer = queue;
		}
		queue.clear();

	}
	if (excced) {
		std::cout<<"Exceeds beam search width!"<<std::endl;
	}

	double score = DBL_MAX;
	int index = 0;
	for (int i = 0; i < Seq_buffer.size(); i++) {
		if (Seq_buffer[i].score <= score) {
			score = Seq_buffer[i].score;
			index = i;
		}
	}
	bestSeq = Seq_buffer[index];
	std::cout << std::endl;
	std::cout << "Best score: " << bestSeq.score << std::endl;

	//output distance and continuity
	int count = 0;
	double distance = 0;
	for (int i = 1; i < bestSeq.index.size(); i++) {
		if (!has_edge[bestSeq.index[i - 1]][bestSeq.index[i]]) {
			count++;
			distance += Constraint_Distance(bestSeq.index[i - 1], bestSeq.index[i]);
		}
		else {
			distance += dh;
		}

	}
	std::cout << "Number of discontinuous: " << count << std::endl;
	std::cout << "Distance: " << distance << std::endl;

	medium_tra_info.push_back(initial_tra_info[bestSeq.index[0]]);// Keep blocks separate.
	for (int i = 1; i < bestSeq.index.size(); i++) {
		if (!has_edge[bestSeq.index[i - 1]][bestSeq.index[i]]) {
			medium_tra_info.push_back(initial_tra_info[bestSeq.index[i]]);

		}
		else {
			for(int j = 0; j < initial_tra_info[bestSeq.index[i]].size(); j++)
				medium_tra_info[medium_tra_info.size() - 1].push_back(initial_tra_info[bestSeq.index[i]][j]);
		}
	}

	InjectionLayerUpdate();// Update the injection order after reordering patches.
	end_time = clock();
	std::cout << "&&&&&&& time of Beam Search: " << double(end_time - start_time) / CLOCKS_PER_SEC << "s &&&&&&&" << std::endl;
}

void TRA_Graph::InjectionLayerUpdate() {
	// Represent an injection layer.
	struct InjectionLayer {
		int type;
		Vec3 center;
	};
	std::vector<InjectionLayer> injection_layers;
	// Initialize injection layer data.
	// Read initial layer types: 0 normal, 1 injection, 2 solid.
	std::ifstream ifile;
	ifile.open("Injection_Layer.txt");
	if (!ifile.is_open()) {
		std::cout << "can not open " << file_name << std::endl;
	}
	int layer_num;
	ifile >> layer_num;	// Read the number of layers.

	for (int i = 0; i < layer_num; i++) {	// Read each layer.
		InjectionLayer buffer;	// Temporarily store layer information.
		ifile >> buffer.type;	// Read the layer type.
		if(buffer.type == 1)	// Read the center for injection layers.
			ifile >> buffer.center.m_x >> buffer.center.m_y >> buffer.center.m_z;

		injection_layers.push_back(buffer);
	}
	ifile.close();

	// Apply the reordered patch sequence.
	std::vector<InjectionLayer> injection_order;
	for (int i = 0; i < medium_tra_info.size(); i++) {
		for (int j = 0; j < medium_tra_info[i].size(); j++) {
			injection_order.push_back(injection_layers[medium_tra_info[i][j]]);
		}
	}

	// Write the injection order.
	std::ofstream ofile;
	ofile.open("Injection_Order.txt");
	if (!ofile.is_open()) {
		std::cout << "can not open " << file_name << std::endl;
	}
	ofile << layer_num << std::endl;
	for (int i = 0; i < injection_order.size(); i++) {
		ofile << injection_order[i].type << std::endl;
		if (injection_order[i].type == 1)
			ofile << injection_order[i].center.m_x << " " << injection_order[i].center.m_y << " " << injection_order[i].center.m_z << std::endl;
	}
	ofile.close();
}

///////TRAvel Algorithm/////////

//	else {

void TRA_Graph::GeneratePath()
{

	//output flat blocks
	int number_block = 0;

	std::string derectory_block = "../Blocks_files";
	std::filesystem::create_directories(derectory_block.c_str());

	for (int i = 0; i < medium_tra_info.size(); i++) {
		std::string sub_file_name = "_block" + std::to_string(number_block);
		std::ofstream dstream("../Blocks_files/" + file_name + sub_file_name + ".obj");
		std::ofstream dstream_txt("../Blocks_files/" + file_name + sub_file_name + ".txt");
		int sum_layer = 0;
		for (int j = 0; j < medium_tra_info[i].size(); j++)
			sum_layer += 1;
		dstream_txt << sum_layer << std::endl;
		for (int j = 0; j < medium_tra_info[i].size(); j++) {
			int m, n;

			m = data.index[medium_tra_info[i][j]].first;
			n = data.index[medium_tra_info[i][j]].second;
			dstream_txt << data.slice_points[m][n].size() << std::endl;
			for (int l = 0; l < data.slice_points[m][n].size(); l++) {
				dstream << "v " << this->data.slice_points[m][n][l].x << " " << this->data.slice_points[m][n][l].y << " " << data.z_value[m][n][l] << std::endl;
				dstream_txt << data.slice_points[m][n][l].x << " " << data.slice_points[m][n][l].y << " " << data.z_value[m][n][l] << " " << dh << std::endl;

				///////for thickness constrant/////////

			}

		}
		dstream.close();
		number_block++;
	}

}
