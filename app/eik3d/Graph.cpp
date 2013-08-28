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
#include "GRACE/Profile.h"
#include "GRACE/Scheduler.h"
#include "GRACE/Util.h"

using namespace GRACE;

/* Common STL Headers  */

#include <cstdio>
#include <vector>
#include <cmath>

using namespace std;

//	User cannot change it!!!!!!!!!!!!!!!!!!!!!!!!
vector<Vertex> Vertex::vtxList;
vector<Edge> Edge::edgeList;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void Graph::GenerateParamter(int runNo) {
	//	Fix random seed to get same result.
	MWCRand::Seed(19860726 * (runNo + 1));

	for (int i = 0; i < Vertex::vtxList.size(); i++) {
		Vertex::vtxList[i].data[0].d.value = u_inf;
		Vertex::vtxList[i].data[1].d.value = u_inf;
	}

	for (int i = 0; i < 100; i++) {
		int uuid = MWCRand::GetRand() % Vertex::vtxList.size();
		int vid = VertexBase::uuid2VtxID[uuid];
		Vertex::vtxList[vid].data[1].d.value = 0.0;
	}
}

/*
 * Vertex Construct Function:
 * Read in one line of the vertex file and construct the initial attribute values
 */
int Vertex::construct(void* file) {
	FILE * f = (FILE *) file;
	//	The last committed slot always starts from 0.

	int x, y, z;
	//	Temporal variable
	fscanf(f, "%d%d%d", &x, &y, &z);

	fscanf(f, "%lf", &this->hf);	
	//fscanf(f, "%lf", &vertexData->value);
	data[0].d.value = u_inf;
	data[1].d.value = u_inf;

	if( ferror(f) ) return -1;
	return 1;
}

/*
 * Vertex Show Function:
 * Print out the attribute values of the vertex
 */
void Vertex::show(FILE * file) {
	const VertexData* vertexData = this->GetLastCommittedData();
	if (vertexData->value == u_inf) {
		fprintf(file, "-1");
	} else {
		fprintf(file, "%.6lf", vertexData->value);
	}
	fprintf(file, "\n");
}

// Logic on Initialization
void Vertex::onInitialize(Job* const job) { }

const VertexData* Vertex::GetLastCommittedData() {
	return &data[GraphBlock::blocks[this->blockID].lastCommittedSlot].d;
}

//	Applicaiton Funciton.
double update_3d(double u1, double u2, double u3, double hf) {
    double v  = (u1 + u2 + u3)/3;
    double v2 = (u1*u1 + u2*u2 + u3*u3)/3;
    double d  = v*v-v2+hf*hf/3;
    if (d < 0)
        return -1;
    return v + sqrt(d);
}


double update_2d(double u1, double u2, double hf) {
    double v  = (u1 + u2)/2;
    double v2 = (u1*u1 + u2*u2)/2;
    double d  = v*v-v2+hf*hf/2;
    if (d < 0)
        return -1;
    return v + sqrt(d);
}


void pick_min_two(double& a, double& b, double& c) {
    if (a >= b && a >= c)
        swap(a,c);
    else if (b >= a && b >= c)
        swap(b,c);
    DBUTL_ASSERT(a <= c);
    DBUTL_ASSERT(b <= c);
}

double Vertex::calculateVertexPriority(const VertexData* srcOldData, const VertexData* srcNewData,
                               Edge* incomingEdge) {
	if (srcNewData->value < this->GetLastCommittedData()->value) {
		return srcNewData->value;
	} else {
		return PriorityMax;
	}
}

// The User defined function for 3D Eikonal
bool GRACE::VertexUpdate(Vertex* vertex, GraphBlock* g) {
	// If vertex is at the boundary, return immediately
	if (vertex->incomingEids.size() < 6)
		return false;

	const VertexData* dxVertexData1 = g->GetVertexData(&Edge::edgeList[vertex->incomingEids[0]]);
	const VertexData* dxVertexData2 = g->GetVertexData(&Edge::edgeList[vertex->incomingEids[1]]);
	const VertexData* dyVertexData1 = g->GetVertexData(&Edge::edgeList[vertex->incomingEids[2]]);
	const VertexData* dyVertexData2 = g->GetVertexData(&Edge::edgeList[vertex->incomingEids[3]]);
	const VertexData* dzVertexData1 = g->GetVertexData(&Edge::edgeList[vertex->incomingEids[4]]);
	const VertexData* dzVertexData2 = g->GetVertexData(&Edge::edgeList[vertex->incomingEids[5]]);


    double ud1 = min( dxVertexData1->value, dxVertexData2->value );
    double ud2 = min( dyVertexData1->value, dyVertexData2->value );
    double ud3 = min( dzVertexData1->value, dzVertexData2->value );

	if (ud1 == u_inf && ud2 == u_inf && ud3 == u_inf)
		return false;

	double hf = vertex->hf;
    double newValue = u_inf;

    double ut = update_3d(ud1, ud2, ud3, hf);
    if (ut > ud1 && ut > ud2 && ut > ud3)
        newValue = ut;
    else {
        pick_min_two(ud1, ud2, ud3);
        ut = update_2d(ud1, ud2, hf);
        if (ut > ud1 && ut > ud2)
            newValue = ut;
        else
            newValue = min(ud1,ud2) + hf;
    }

	VertexData* vertexData = g->GetLocalVertexWriteData(vertex);
	if (newValue < vertexData->value) {
		vertexData->value = newValue;
		return true;
	} else {
		return false;
	}
}

/* -------- User must instantiate edge functions above -------- */


