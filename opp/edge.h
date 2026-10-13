#pragma once
#ifndef EDGE_H_
#define EDGE_H_

class Edge
{
private:
	int from, to;
	double weight;
public:
	Edge() {};
	Edge(int _from, int _to) : from(_from), to(_to) {};
	Edge(int _from, int _to, double _weight) : from(_from), to(_to), weight(_weight) {};
	inline int GetFrom() { return from; }
	inline int GetTo() { return to; }
	inline double GetWeight() { return weight; }
};

#endif
