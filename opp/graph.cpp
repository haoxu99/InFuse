#include "graph.h"

Graph::Graph()
{
}

Graph::~Graph()
{

}

void Graph::AddEdge(int from, int to)
{
	this->edges.push_back(Edge(from, to));
	this->out_degree[from]++;
	this->in_degree[to]++;
	int m = edges.size();
	G[from].push_back(m - 1);
}

void Graph::AddWeightEdge(int from, int to, double weight) {

	this->edges.push_back(Edge(from, to, weight));
	this->out_degree[from]++;
	this->in_degree[to]++;
	int m = edges.size();
	G[from].push_back(m - 1);
}

void Graph::UpdateDegree(int node_id, int change)
{
	for (int i = 0; i < G[node_id].size(); i++) {
		if (edge_deleted[G[node_id][i]]) continue;
		int to = this->edges[G[node_id][i]].GetTo();
		this->in_degree[to] += change;
		if (this->in_degree[to] < 0)
			this->in_degree[to] = 0;
	}
	this->out_degree[node_id] += change * G[node_id].size();
}
