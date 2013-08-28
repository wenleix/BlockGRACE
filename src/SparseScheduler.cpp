/*
 *  SparseScheduler.cpp
 *  GRACE
 *
 *  Created by Wenlei Xie on 2/1/13.
 *  Copyright 2012 Cornell. All rights reserved.
 */

#include "GRACE/SparseScheduler.h"
#include "GRACE/GraphBlock.h"

using namespace GRACE;

GraphBlock* SparseEagerScheduler::GetNextBlock(Job* job) {
	GraphBlock* block = NULL;

	//	TODO(wenleix): I am using "reservedBlockID" as Index in the global schedule list.
	if  (job->reservedBeginBlockID >= job->reservedEndBlockID) {
		//	Quick quit.
		if (currentScheduleIdx >= globalScheduleList.size())
			return NULL;

		//	Reserve some blocks.
		Lock();
		int beginScheduleIdx = currentScheduleIdx;
		currentScheduleIdx += this->numBlockReserved;
		Unlock();

		if (beginScheduleIdx >= globalScheduleList.size()) {
			return NULL;
		} else {
			job->reservedBeginBlockID = beginScheduleIdx;
			job->reservedEndBlockID = std::min(beginScheduleIdx + numBlockReserved, (int)(globalScheduleList.size()));
		}
	}

	int grabIdx = job->reservedBeginBlockID;
	job->reservedBeginBlockID++;

	int blockID = this->globalScheduleList[grabIdx];
	block = &GraphBlock::blocks[blockID];
	DBUTL_ASSERT_ALWAYS(schedule[blockID]);

	//	Prepare for update.
	schedule[blockID] = false;
	block->PrepareForUpdate();
	return block;
}

void SparseEagerScheduler::FinishBlockUpdate(GraphBlock* gblock, Job *job, bool init) {
	if (this->isVertexScheduler)
		DBUTL_ASSERT_ALWAYS(gblock->size == 1);

	//	Commit Data First
	gblock->Commit();

	if (!isVertexScheduler && !gblock->convergent) {
		int blockID = gblock->blockID;
		//	If it's a block scheduler and the cuurent block is not convergent.
		if (!schedule[blockID] && !scheduleNext[blockID]) {
			scheduleNext[blockID] = true;
			job->localScheduleList.push_back(gblock->blockID);
		}
	}

	//	Scheduling neighbors.
	for (int i = 0; i < gblock->size; i++) {
		int vertexID = gblock->vertexID[i];
		Vertex* vertex = &Vertex::vtxList[vertexID];
		if (vertex->lastCommittedDataChanged()) {
			//	Last committed data changed, notify its outgoing blocks.
			for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin(); it != vertex->outgoingEids.end(); it++) {
				Edge *edge = &Edge::edgeList[*it];
				if (init || edge->receiverBlockIndex != -1) {
					GraphBlock* receiverBlock = GraphBlock::blocks + edge->dstVtx->blockID;
					int blockID = edge->dstVtx->blockID;
					if (!schedule[blockID] && !scheduleNext[blockID]) {
						scheduleNext[blockID] = true;
						job->localScheduleList.push_back(receiverBlock->blockID);
					}
				}
			}
		}
	}
}

void SparseEagerScheduler::PrepareForNextIteration() {
	globalScheduleList.clear();

	//	Iterate over all the local schedule list and deduplicate.
	for (std::vector<Job*>::iterator iter = task->jobs.begin(); iter != task->jobs.end(); iter++) {
		Job* job = *iter;
		for (int i = 0; i < job->localScheduleList.size(); i++) {
			int blockID = job->localScheduleList[i];
			GraphBlock* block = &GraphBlock::blocks[blockID];
			if (scheduleNext[blockID]) {
				scheduleNext[blockID] = false;
				schedule[blockID] = true;
				globalScheduleList.push_back(blockID);
			}
		}
		//	Clean local schedule list.
		job->localScheduleList.clear();
	}

	if (sortList)
		sort(globalScheduleList.begin(), globalScheduleList.end());

	this->currentScheduleIdx = 0;
}

void SparseEagerScheduler::Initialize() {
	this->InitializeBlocks();

	this->schedule = new bool[GraphBlock::numBlocks];
	this->scheduleNext = new bool[GraphBlock::numBlocks];

	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		this->schedule[i] = false;
		this->scheduleNext[i] = false;
	}
	//this->SetRandomPick();
}


