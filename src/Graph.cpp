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
#include "GRACE/Util.h"

/* Common STL Headers  */

#include <cstdio>
#include <vector>
#include <cmath>

using namespace GRACE;

//	User cannot change it!!!!!!!!!!!!!!!!!!!!!!!!
std::vector<Vertex> Vertex::vtxList;
std::vector<Edge> Edge::edgeList;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

const REAL DAMP_FACTOR = 0.85;
const REAL initValue = 0.0;

void Graph::GenerateParamter(int runNo) {
	//	Fix random seed to get same result.
	MWCRand::Seed(19860726 * (runNo + 1));

	for (int i = 0; i < Vertex::vtxList.size(); i++) {
		Vertex::vtxList[i].pref = 0.0;
		Vertex::vtxList[i].data[0].d.rank = 0.0;
		Vertex::vtxList[i].data[1].d.rank = 0.0;
	}

	//	Randomly choose 10000 topics.
	int numTopics = 10000;
	for (int i = 0; i < numTopics; i++) {
		int uuid = MWCRand::GetRand() % Vertex::vtxList.size();
		int vid = VertexBase::uuid2VtxID[uuid];
	//	fprintf(stdout, "vid = %d\n", vid);
		Vertex::vtxList[vid].pref += (double)Vertex::vtxList.size() / numTopics * (1.0 - DAMP_FACTOR);
		Vertex::vtxList[vid].data[1].d.rank = Vertex::vtxList[vid].pref;
	} 
}

/*
 * Vertex Construct Function:
 * Read in one line of the vertex file and construct the initial attribute values
 */
int Vertex::construct(void* file) {
	FILE * f = (FILE *) file;
	//	The last committed slot always starts from 0.
	//fscanf(f, "%lf", &this->pref);

	this->pref = (1.0 - DAMP_FACTOR);

	this->data[0].d.rank = 0.0f;
	this->data[1].d.rank = this->pref;

	if (ferror(f)) return -1;
	return 1;
}

/*
 * Vertex Show Function:
 * Print out the attribute values of the vertex
 */
void Vertex::show(FILE * file) {
	const VertexData* vertexData = this->GetLastCommittedData();
	fprintf(file, " %.8lf", vertexData->rank);
	fprintf(file, "\n");
}

// Logic on Initialization
void Vertex::onInitialize(Job* const job) { 
	for (std::vector<uint32_t>::iterator it = this->outgoingEids.begin();
			it != this->outgoingEids.end(); it++) {
		Edge* outEdge = &Edge::edgeList[*it];
		outEdge->weight = 1.0 / this->outgoingEids.size();
	}
}

const VertexData* Vertex::GetLastCommittedData() {
	return &data[GraphBlock::blocks[this->blockID].lastCommittedSlot].d;
}

/*
 * Edge Construct Function
 * Read in one line of the edge file and construct the initial attribute values
 */
int Edge::construct(char* str) {
	return 1;
}

bool GRACE::VertexUpdate(Vertex* vertex, GraphBlock* g) {
	double sum = 0.0f;

	//	Get the incoming vertex
	for (std::vector<uint32_t>::iterator it = vertex->incomingEids.begin();
			it != vertex->incomingEids.end(); it++) {
		Edge* incomingEdge = &Edge::edgeList[*it];
		Vertex* srcVertex = (Vertex*)(incomingEdge->srcVtx);
		const VertexData* srcVertexData = g->GetVertexData(incomingEdge);
		sum += srcVertexData->rank * incomingEdge->weight;
	}

	VertexData* vertexData = g->GetLocalVertexWriteData(vertex);
	double newRank = DAMP_FACTOR * sum + vertex->pref;
	bool updated = fabs(newRank - vertexData->rank) > eps;
	vertexData->rank = newRank;
	return updated;
}

/* -------- User must instantiate edge functions above -------- */


