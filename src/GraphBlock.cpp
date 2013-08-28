/*
 *  GraphBlock.cpp
 *  GRACE
 *
 *  Created by Wenlei Xie on 9/27/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */


//	GRACE Headers
#include "GRACE/GraphBlock.h"
#include "GRACE/Job.h"

using namespace DBUtl;
using namespace GRACE;

GraphBlock* GraphBlock::blocks = NULL;
int GraphBlock::numBlocks;

void GraphBlock::Initialize(int* vertexID, int size, int BlockID) {
	this->size = size;
	this->vertexID = new int[size];
	this->blockID = BlockID;
	std::copy(vertexID, vertexID + size, this->vertexID);
	std::sort(this->vertexID, this->vertexID + this->size);

	//	Decide whether the vertexID are consecutive
	this->vertexIsConsecutive = true;
	for (int i = 0; i < size; i++) {
		if (this->vertexID[i] != this->vertexID[0] + i) {
			this->vertexIsConsecutive = false;
//			fprintf(stderr, "err vtxID[i]: %d,  vtxID[0]: %d, i: %d\n",  vertexID[i], vertexID[0], i);
			//	The vertices in a block should always be consecutive.
			DBUTL_ASSERT_ALWAYS(false);
			break;
		}
	}

	this->vertexPriority = new float[size];
//	fprintf(stderr, "vertexPriority Initlaized!!\n");

	//	Consecutive, set up inBlockID
	int baseBlockID = this->vertexID[0];
	for (int i = 0; i < size; i++) {
		int vertexID = this->vertexID[i];
		Vertex::vtxList[vertexID].inBlockID = vertexID - baseBlockID;
		DBUTL_ASSERT_ALWAYS(Vertex::vtxList[vertexID].inBlockID == i);
	}
}

void GraphBlock::SetupBlocks() {
	int numBlockEdges = 0;

	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		GraphBlock* block = GraphBlock::blocks + i;

		//	Collect all the incoming block IDs.
		// TODO(guozhang): Use a set instead of a vector, and save the unique operation?
		std::vector<int> tempIncomingBlockID;
		for (int j = 0; j < block->GetBlockSize(); j++) {
			int vertexID = block->GetVertexID(j);

			//	Iterate over their incoming edges;
			Vertex* vertex = &Vertex::vtxList[vertexID];
			for (std::vector<uint32_t>::iterator it = vertex->incomingEids.begin();
					it != vertex->incomingEids.end(); it++) {
				Edge* edge = &Edge::edgeList[*it];
				int blockID = edge->srcVtx->blockID;
				if (blockID != i)
					tempIncomingBlockID.push_back((int)(edge->srcVtx->blockID));
			}
		}

		//	Make a unique list.
		std::sort(tempIncomingBlockID.begin(), tempIncomingBlockID.end());
		tempIncomingBlockID.resize(unique(tempIncomingBlockID.begin(), tempIncomingBlockID.end())
								   - tempIncomingBlockID.begin());

		numBlockEdges += tempIncomingBlockID.size();
		block->numIncomingBlocks = tempIncomingBlockID.size();
		block->incomingBlockID = new int[block->numIncomingBlocks];
		block->incomingBlockSlot = new int[block->numIncomingBlocks];
		std::copy(tempIncomingBlockID.begin(), tempIncomingBlockID.end(),
				  block->incomingBlockID);

		// DEBUG
//		fprintf(stderr, "Num Incoming Blocks: %d\n", block->numIncomingBlocks);

		//	Assign the receiver block index on the edge.
		for (int j = 0; j < block->GetBlockSize(); j++) {
			int vertexID = block->GetVertexID(j);
			//	Iterate over their incoming edges;
			Vertex* vertex = &Vertex::vtxList[vertexID];
			for (std::vector<uint32_t>::iterator it = vertex->incomingEids.begin();
					it != vertex->incomingEids.end(); it++) {
				Edge* edge = &Edge::edgeList[*it];
				int blockID = edge->srcVtx->blockID;
				// TODO(wenleix): Use a hash map to accelerate?
				if (blockID == i) {
					//	Local edge.
					edge->receiverBlockIndex = -1;
				} else {
					edge->receiverBlockIndex = std::lower_bound(block->incomingBlockID,
																block->incomingBlockID + block->numIncomingBlocks,
																blockID) - block->incomingBlockID;
					DBUTL_ASSERT(block->incomingBlockID[edge->receiverBlockIndex] == blockID);
				}
			}
		}
	}

	//	For each vertex, rearrange the outgoing edges.
/*	for (std::vector<Vertex>::iterator it = Vertex::vtxList.begin(); it != Vertex::vtxList.end(); it++) {
		int *eLocal = new int[it->numOutEdges], eLocalCnt = 0;
		int *eRemote = new int[it->numOutEdges], eRemoteCnt = 0;
		for (std::vector<uint32_t>::iterator it2 = it->outgoingEids.begin(); it2 != it->outgoingEids.end(); it2++) {
			Edge* edge = &Edge::edgeList[*it2];
			if (edge->receiverBlockIndex == -1) {
				eLocal[eLocalCnt++] = *it2;
			} else {
				eRemote[eRemoteCnt++] = *it2;
			}
		}
		copy(eLocal, eLocal + eLocalCnt, it->outgoingEids.begin());
		delete [] eLocal;
		copy(eRemote, eRemote + eRemoteCnt, it->outgoingEids.begin() + eLocalCnt);
		delete [] eRemote;
	} */

	fprintf(stderr, "Total # block edges: %d\n", numBlockEdges);
}

bool* GraphBlock::GetReadVertexScheduling(Job *job) {
	return job->taskInfo->inSchedule.schedulingBit[pushSlot ^ 1][blockID];
}

bool* GraphBlock::GetWriteVertexScheduling(Job *job) {
	return job->taskInfo->inSchedule.schedulingBit[pushSlot][blockID];
}



