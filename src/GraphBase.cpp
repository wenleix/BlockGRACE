/*
 *  GraphBase.cpp
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/20/12.
 *  Updated by Wenlei Xie on 5/9/12
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/Job.h"
#include "GRACE/TaskBase.h"
#include "GRACE/Scheduler.h"
#include "GRACE/Graph.h"
#include "GRACE/GraphBlock.h"
#include "GRACE/Profile.h"

/* Cornell Utility Headers  */

#include <DBUtl/Log.h>

/* Common STL Headers */

#include <vector>

#include <sys/time.h>

using namespace DBUtl;
using namespace GRACE;

/* ----------- VertexBase ---------------- */

std::vector<int> VertexBase::uuid2VtxID;

/* Comparators */
bool vIDComp(const Vertex& v1, const Vertex& v2) {
	return (v1.vertexID < v2.vertexID);
}

void VertexBase::sortByID() {
	timeval start, end;
	gettimeofday(&start, NULL);

	sort(Vertex::vtxList.begin(), Vertex::vtxList.end(), vIDComp);

	gettimeofday(&end, NULL);
	fprintf(stdout, "Vertex Sort Finished! Time elapsed %.2f seconds.\n",
			(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);
}


/* Initialization of Edge */
void EdgeBase::init(uint16_t srcPID, uint32_t srcVID, uint16_t dstPID, uint32_t dstVID) {
	srcVtx = &Vertex::vtxList[srcVID];
	dstVtx = &Vertex::vtxList[dstVID];

	DBUTL_ASSERT_ALWAYS(srcVtx->vertexID == srcVID);
	DBUTL_ASSERT_ALWAYS(dstVtx->vertexID == dstVID);

	return;
}

