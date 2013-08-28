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
#include "GRACE/Util.h"

/* Common STL Headers  */

#include <cmath>
#include <cstdio>
#include <vector>
#include <map>

const double MaxDist = 1000000000.0;;
const double eps = 1e-8;

namespace GRACE {

	//	Just static functions.
	class Graph {
		public:
			//	Used by workload simulation, generate a new set of parameters for the application.
			//	Ideally, the parameter should be same for the given runNo.
			static void GenerateParamter(int runNo);
	};


	class Edge;

	struct VertexData {
		double dist;
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
			return fabs(data[0].d.dist - data[1].d.dist) > eps;
		}

		double calculateVertexPriority(const VertexData* srcOldData, const VertexData* srcNewData,
		                               Edge* incomingEdge);


		// The two version for Vertex Data.
		// TODO(wenleix): Prevent Cache Coherence?
		struct {
			VertexData d;
#ifdef VERTEXDATA_PADDING
			char padding[64 - sizeof(VertexData)];
#endif
		} data[2];

	private:
		//	A sugar function for internal usage.
		const VertexData* GetLastCommittedData();
	};

	// Graph Edge class
	class Edge : public EdgeBase {
	public:
		static std::vector<Edge> edgeList;

		/* Construct Function */
		int construct(char* str);

		double cost;
	};

	//	TODO(wenleix): A separate class called "VertexHelper"?
	bool VertexUpdate(Vertex* vertex, GraphBlock* g);


} /* namespace GRACE */

#endif /* _GRACE_GRAPH_H_ */
