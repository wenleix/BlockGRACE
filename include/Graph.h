/*
 *  Graph.h
 *  GRACE
 *
 *  User instantiated Graph specification
 */

#ifndef _GRACE_GRAPH_H_
#define _GRACE_GRAPH_H_

/* BRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/Task.h"
#include "GRACE/GraphBase.h" 

/* Common STL Headers  */

#include <cmath>
#include <cstdio>
#include <vector>
#include <map>

typedef double REAL;

namespace GRACE {
const REAL eps = 1e-3;

class Edge;

struct VertexData {
	REAL rank;
};

//	Just static functions.
class Graph {
public:
	//	Used by workload simulation, generate a new set of parameters for the application.
	//	Ideally, the parameter should be same for the given runNo.
	static void GenerateParamter(int runNo);
};

// Graph Edge class
class Edge : public EdgeBase {
public:
	REAL weight;

	static std::vector<Edge> edgeList;

	/* Construct Function */
	int construct(char *str);
};

// Graph Vertex class
class Vertex : public VertexBase {

public:
	static std::vector<Vertex> vtxList;

	Vertex() : VertexBase() {}

	/* Construct Function */
	int construct(void* file);

	/* Show Function */
	void show(FILE * file);

	/* Logic on Initialization */
	void onInitialize(Job* const job);

	bool lastCommittedDataChanged() {
		//	TODO(wenleix): Not correct when the vertex is not updated.
		return fabs(data[0].d.rank - data[1].d.rank) > eps;
	}

	double calculateVertexPriority(const VertexData* srcOldData, const VertexData* srcNewData,
				                   Edge* incomingEdge) {
		return fabs(srcNewData->rank - srcOldData->rank) * incomingEdge->weight;
	}


	// The two version for Vertex Data.
	struct {
		VertexData d;
#ifdef VERTEXDATA_PADDING
		char padding[64 - sizeof(VertexData)];
#endif
	} data[2];

	//	The preference value.
	REAL pref;

private:
	//	A sugar function for internal usage.
	const VertexData* GetLastCommittedData();
};

//	TODO(wenleix): A separate class called "VertexHelper"?
bool VertexUpdate(Vertex* vertex, GraphBlock* g);


} /* namespace GRACE */

#endif /* _GRACE_GRAPH_H_ */
