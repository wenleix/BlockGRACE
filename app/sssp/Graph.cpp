/*
 *  Graph.cpp
 *  GRACE
 *
 *  User instantiated Graph specification
 */


/* BRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/Graph.h"
#include "GRACE/Job.h"
#include "GRACE/GraphBlock.h"
#include "GRACE/Scheduler.h"

/* Common STL Headers  */

#include <cstdio>
#include <vector>
#include <cmath>

using namespace GRACE;

//	User cannot change it!!!!!!!!!!!!!!!!!!!!!!!!
std::vector<Vertex> Vertex::vtxList;
std::vector<Edge> Edge::edgeList;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void Graph::GenerateParamter(int runNo) {
	//	Fix random seed to get same result.
	MWCRand::Seed(19860726 * (runNo + 1));

	for (int i = 0; i < Vertex::vtxList.size(); i++) {
		Vertex::vtxList[i].data[0].d.dist = MaxDist;
		Vertex::vtxList[i].data[1].d.dist = MaxDist;
	}

	for (int i = 0; i < 3; i++) {
		int uuid = MWCRand::GetRand() % Vertex::vtxList.size();
		int vid = VertexBase::uuid2VtxID[uuid];
		Vertex::vtxList[vid].data[1].d.dist = 0.0;
	}
}


/*
 * Vertex Construct Function:
 * Read in one line of the vertex file and construct the initial attribute values
 */
int Vertex::construct(void* file) {
	FILE * f = (FILE *) file;
	//	The last committed slot always starts from 0.
/*	VertexData *vertexData = &this->data[1].d;
	fscanf(f, "%lf", &vertexData->dist);
	if (vertexData->dist > MaxDist)
		vertexData->dist = MaxDist; */

	data[0].d.dist = MaxDist;
	data[1].d.dist = MaxDist;

	if (ferror(f)) return -1;
	return 1;
}

/*
 * Vertex Show Function:
 * Print out the attribute values of the vertex
 */
void Vertex::show(FILE * file) {
	const VertexData* vertexData = this->GetLastCommittedData();
	fprintf(file, " %.6f\n", vertexData->dist);
}

// Logic on Initialization
void Vertex::onInitialize(Job* const job) {
	//	Nothing to initialize for SSSP.
}

const VertexData* Vertex::GetLastCommittedData() {
	return &data[GraphBlock::blocks[this->blockID].lastCommittedSlot].d;
}

/*
 * Edge Construct Function
 * Read in one line of the edge file and construct the initial attribute values
 */
int Edge::construct(char* str) {
	this->cost = atof(str);
//	this->cost = 1.0;
	return 1;
}

double Vertex::calculateVertexPriority(const VertexData* srcOldData, const VertexData* srcNewData,
                               Edge* incomingEdge) {
	double updatedDist = srcNewData->dist + incomingEdge->cost;
	if (updatedDist < this->GetLastCommittedData()->dist) {
		return updatedDist;
	} else {
		return PriorityMax;
	}
}


bool GRACE::VertexUpdate(Vertex* vertex, GraphBlock* g) {
	double newDist = MaxDist;
	//	Get the incoming vertex
	for (std::vector<uint32_t>::iterator it = vertex->incomingEids.begin();
			it != vertex->incomingEids.end(); it++) {
		Edge* incomingEdge = &Edge::edgeList[*it];
		Vertex* srcVertex = (Vertex*)(incomingEdge->srcVtx);
		const VertexData* srcVertexData = g->GetVertexData(incomingEdge);
		newDist = std::min(newDist, srcVertexData->dist + incomingEdge->cost);
	}

	VertexData* vertexData = g->GetLocalVertexWriteData(vertex);
	if (newDist < vertexData->dist) {
		vertexData->dist = newDist;
		return true;
	} else {
		//	No update
		return false;
	}
}

/* -------- User must instantiate edge functions above -------- */


