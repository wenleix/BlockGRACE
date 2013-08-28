/*
 *  Scheduler.cpp
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/17/12.
 *  Updated by Wenlei Xie on 5/9/12.
 *  Copyright 2012 Cornell. All rights reserved.
 */

/* GRACE Headers */

#include "GRACE/Scheduler.h"
#include "GRACE/GraphBase.h"
#include "GRACE/TaskBase.h"
#include "GRACE/Graph.h"
#include "GRACE/GraphBlock.h"
#include "GRACE/Job.h"

#include <vector>

using namespace GRACE;

GraphBlock* AbstractBlockScheduler::GrabBlock(Job* job) {
	while (true) {
		while (job->reservedBeginBlockID < job->reservedEndBlockID) {
			int grabBlockID = randomPick 
							  ? scheduleIndex[job->reservedBeginBlockID]
							  : job->reservedBeginBlockID;
			job->reservedBeginBlockID++;

			if (schedule[grabBlockID]) {
				GraphBlock* block = GraphBlock::blocks + grabBlockID;
				return block;
			}
		}

		//	Quick quit.
		if (currentBlockID >= GraphBlock::numBlocks)
			return NULL;

		//	Reserve some blocks.
		Lock();
		int beginBlockID = currentBlockID;
		currentBlockID += this->numBlockReserved;
		Unlock();

		if (beginBlockID >= GraphBlock::numBlocks) {
			return NULL;
		} else {
			job->reservedBeginBlockID = beginBlockID;
			job->reservedEndBlockID = std::min(beginBlockID + numBlockReserved, GraphBlock::numBlocks);
		}
	}

	DBUTL_ASSERT_ALWAYS(false);
	return NULL;
}


 void AbstractBlockScheduler::SetRandomPick() {
	randomPick = true;

	//	Initialize the scheduling sequence
	scheduleIndex = new int[GraphBlock::numBlocks];
	for (int i = 0; i < GraphBlock::numBlocks; i++)
		scheduleIndex[i] = i;
	srand(860726);	//	Fix seed to get same result.
	std::random_shuffle(scheduleIndex, scheduleIndex + GraphBlock::numBlocks);
	fprintf(stderr, "RandomPick Enabled!\n");
	fprintf(stderr, "First ten: ");
	for (int i = 0; i < 10 && i < GraphBlock::numBlocks; i++)
		fprintf(stderr, "%d ", scheduleIndex[i]);
	fprintf(stderr, "\n");
}

void AbstractBlockScheduler::InitializeBlocks() {
	//	Do the Block Initialization depends on whether it's a vertex scheduler or block scheduler.
	if (this->isVertexScheduler) {
		GraphBlock::InitializeVerticesAsBlocks();
	} else {
		GraphBlock::InitalizeGraphBlocks();
	}
}

AbstractBlockScheduler::~AbstractBlockScheduler() {
	DestroyLock();
	//fprintf(stderr, "Start deallocte memory in ~AbstractBlockScheduler\n");
	if (scheduleIndex != NULL)
		delete [] scheduleIndex;
	if (schedule != NULL)
		delete [] schedule;

	if (blockPriorLock != NULL) {
		for (int i = 0; i < GraphBlock::numBlocks; i++)
			SpinLockDestroy(&blockPriorLock[i]);
		delete [] blockPriorLock;
	}

	//fprintf(stderr, "FInish deallocte memory in ~AbstractBlockScheduler\n");
}




//	============== StaticScheduler. ==============

GraphBlock* StaticScheduler::GetNextBlock(Job* job) {
	GraphBlock* block = this->GrabBlock(job);
	if (block != NULL) {
		block->PrepareForUpdate();
	}
	return block;
}

void StaticScheduler::FinishBlockUpdate(GraphBlock* gblock, Job* job, bool init) {
	// Change the block status.
	gblock->Commit();

	//	Notify neighbors.
	if (!init && job->taskInfo->dynamicSweep) {
		for (int i = 0; i < gblock->size; i++) {
			int vertexID = gblock->vertexID[i];
			Vertex* vertex = &Vertex::vtxList[vertexID];
			if (vertex->lastCommittedDataChanged()) {
				//	Last committed data changed, notify its outgoing blocks.
				for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin(); it != vertex->outgoingEids.end(); it++) {
					Edge *edge = &Edge::edgeList[*it];
					if (edge->receiverBlockIndex != -1) {
						GraphBlock* receiverBlock = GraphBlock::blocks + edge->dstVtx->blockID;

						bool *inSchedule = receiverBlock->GetWriteVertexScheduling(job);
						inSchedule[edge->dstVtx->inBlockID] = true;
					}
				}
			}
		}
	}

}

void StaticScheduler::Initialize() {
	this->InitializeBlocks();

	schedule = new bool[GraphBlock::numBlocks];
	for (int i = 0; i < GraphBlock::numBlocks; i++)
		schedule[i] = true;

//	this->SetRandomPick();
}




//	============== EagerScheduler. ==============

GraphBlock* EagerScheduler::GetNextBlock(Job* job) {
	GraphBlock* block = this->GrabBlock(job);
	if (block != NULL) {
		schedule[block->blockID] = false;
		block->PrepareForUpdate();
	}
	return block;
}

void EagerScheduler::FinishBlockUpdate(GraphBlock* gblock, Job* job, bool init) {
	//	Sanity Check
	if (this->isVertexScheduler)
		DBUTL_ASSERT_ALWAYS(gblock->size == 1);

	//	Commit Data First
	gblock->Commit();

	if (!init && !isVertexScheduler && !gblock->convergent) {
		//	If it's a block scheduler and the current block is not convergent.
		if (!schedule[gblock->blockID]) {
			scheduleNext[gblock->blockID] = true;
		}
	}

	//	Notify neighbors.
	for (int i = 0; i < gblock->size; i++) {
		int vertexID = gblock->vertexID[i];
		Vertex* vertex = &Vertex::vtxList[vertexID];
		if (vertex->lastCommittedDataChanged()) {
			//	Last committed data changed, notify its outgoing blocks.
			for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin(); it != vertex->outgoingEids.end(); it++) {
				Edge *edge = &Edge::edgeList[*it];
				if (init || edge->receiverBlockIndex != -1) {
					GraphBlock* receiverBlock = GraphBlock::blocks + edge->dstVtx->blockID;

					//	COMMENT: The following procedure is the actual scheduling policy for Eager (assumed to be defined as OnNbrChange)

					if (!schedule[receiverBlock->blockID]) {
						scheduleNext[receiverBlock->blockID] = true;
					}

					//	!!!	TODO(wenlex): Refactor
					if (!init && job->taskInfo->dynamicSweep) {
						bool *inSchedule = receiverBlock->GetWriteVertexScheduling(job);
						inSchedule[edge->dstVtx->inBlockID] = true;
					}

					//	END COMMENT.

				}
			}
		}
	}
}

void EagerScheduler::Initialize() {
	this->InitializeBlocks();

	schedule = new bool[GraphBlock::numBlocks];
	scheduleNext = new bool[GraphBlock::numBlocks];
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		schedule[i] = false;
		scheduleNext[i] = false;
	}

	//this->SetRandomPick();
}

void EagerScheduler::ResetForNextRun() {
	//	Reset all the scheduling bit.
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		schedule[i] = false;
		scheduleNext[i] = false;
	}
}


//	============== PriorScheduler. ==============

GraphBlock* PriorScheduler::GetNextBlock(Job* job) {
	GraphBlock* block = this->GrabBlock(job);
	if (block != NULL) {
		//	Reset priority.
		if (job->taskInfo->avoidInternalPriority) {
			this->ResetPriority(block->blockID);
			//block->priority = PriorityMax;
		} else {
			this->LockPrior(block->blockID);
			p[block->blockID].prior = PriorityMax;
			for (int i = 0; i < block->size; i++)
				this->InitializePriorAggr(&block->vertexPriority[i]);
			this->UnlockPrior(block->blockID);
		}

		block->PrepareForUpdate();

	}
	return block;
}

void PriorScheduler::PrepareForNextIteration() {
	//	Sample and determine the threshold.
	int size = GraphBlock::numBlocks;
	if (SampleSize >= size) {
		for (int i = 0; i < size; i++) {
//			samplePrior[i] = GraphBlock::blocks[i].priority;
			samplePrior[i] = p[i].prior;
		}
		std::sort(samplePrior, samplePrior + size);
		//	Min Priority Queue
		rankThreshold = samplePrior[std::min((int)(size * ratio), size - 1)];
	} else {
		//	Sample
		for (int i = 0; i < SampleSize; i++) {
			int sampleIndex = rand() % size;
//			samplePrior[i] = GraphBlock::blocks[sampleIndex].priority;
			samplePrior[i] = p[sampleIndex].prior;
		}
		std::sort(samplePrior, samplePrior + SampleSize);
		//	Min Priority Queue
		rankThreshold = samplePrior[std::min((int)(SampleSize * ratio), SampleSize - 1)];
	}
	//fprintf(stderr, "rankThreshold = %f\n", rankThreshold);
	task->scheduleVertices();

	//	Reset VBlock ID.
	this->Reset();
}

void PriorScheduler::FinishBlockUpdate(GraphBlock* gblock, Job* job, bool init) {
	//	Commit data first.
	gblock->Commit();

	if (job->taskInfo->avoidInternalPriority) {
		//	Notify its neighbors.
		for (int i = 0; i < gblock->size; i++) {
			int vertexID = gblock->vertexID[i];
			Vertex* vertex = &Vertex::vtxList[vertexID];
			if (vertex->lastCommittedDataChanged()) {
				//	Last committed data changed, update its outgoing blocks' priority.
				for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin(); it != vertex->outgoingEids.end(); it++) {
					Edge *edge = &Edge::edgeList[*it];
					if (init || edge->receiverBlockIndex != -1) {
						Vertex* dstVertex = (Vertex*)(edge->dstVtx);
						GraphBlock* receiverBlock = GraphBlock::blocks + edge->dstVtx->blockID;


						//	COMMENT: The following procedure is the actual scheduling policy for Prior (assumed to be defined as OnNbrChange)

						double priority = dstVertex->calculateVertexPriority(gblock->GetLocalVertexWriteData(vertex),
																			 gblock->GetLocalVertexLastCommittedData(vertex),
																			 edge);

						//	Update receiver block's priority.
						this->UpdatePriorityMin(receiverBlock->blockID, priority);
	//					receiverBlock->UpdatePriorityMin(priority);

						//	!!!	TODO(wenleix): Refactor
						if (!init && job->taskInfo->dynamicSweep) {
							bool *inSchedule = receiverBlock->GetWriteVertexScheduling(job);
							inSchedule[edge->dstVtx->inBlockID] = true;
						}

						//	END COMMENT.

					}
				}
			}
		}
	} else {
		for (int i = 0; i < gblock->size; i++) {
			int vertexID = gblock->vertexID[i];
			Vertex* vertex = &Vertex::vtxList[vertexID];
			if (vertex->lastCommittedDataChanged()) {
				//	Last committed data changed, update its outgoing blocks' priority.
				for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin(); it != vertex->outgoingEids.end(); it++) {
					Edge *edge = &Edge::edgeList[*it];
					if (init || edge->receiverBlockIndex != -1) {
						Vertex* dstVertex = (Vertex*)(edge->dstVtx);
						GraphBlock* receiverBlock = GraphBlock::blocks + edge->dstVtx->blockID;


						//	COMMENT: The following procedure is the actual scheduling policy for Prior (assumed to be defined as OnNbrChange)

						double imp = dstVertex->calculateVertexPriority(gblock->GetLocalVertexWriteData(vertex),
																	    gblock->GetLocalVertexLastCommittedData(vertex),
																	    edge);

						int bid = receiverBlock->blockID;
						this->LockPrior(bid);

//	Version 1: Function call
						float *vtxPrior = &receiverBlock->vertexPriority[dstVertex->inBlockID];

						float oldPrior = *vtxPrior;
						this->UpdateVertexPrior(vtxPrior, imp);
						this->IncUpdatePriorAggr(bid, oldPrior, *vtxPrior);  

//	Version 2: Incrementally update wo/ function call
/*						float *vtxPrior = &receiverBlock->vertexPriority[dstVertex->inBlockID];
						float oldPrior = *vtxPrior;
						
						//DBUTL_ASSERT_ALWAYS(p[bid].prior <= oldPrior);
						if (imp < *vtxPrior)
							*vtxPrior = imp;
						if (*vtxPrior < p[bid].prior)
							p[bid].prior = *vtxPrior;    */

//	Version 3: Directly update the block priority
	/*					if (imp < p[bid].prior) {
							p[bid].prior = imp;
						}   */

						this->UnlockPrior(bid); 

						//	END COMMENT.

						//	!!!	TODO(wenleix): Refactor
						if (!init && job->taskInfo->dynamicSweep) {
							bool *inSchedule = receiverBlock->GetWriteVertexScheduling(job);
							inSchedule[edge->dstVtx->inBlockID] = true;
						}

					}
				}
			}
		}
	}
}

void PriorScheduler::BarrierSetScheduleBit(int blockID) const {
/*	GraphBlock* b = &GraphBlock::blocks[blockID];
	schedule[blockID] = (b->priority <= rankThreshold && b->priority != PriorityMax); */
//	schedule[blockID] = (blockPriority[blockID] <= rankThreshold && blockPriority[blockID] != PriorityMax);
	schedule[blockID] = (p[blockID].prior <= rankThreshold && p[blockID].prior != PriorityMax);
}


void PriorScheduler::Initialize() {
	//fprintf(stderr, "Size of PriorStruct %lu\n", sizeof(PriorStruct));
	this->InitializeBlocks();

	schedule = new bool[GraphBlock::numBlocks];

	p = new PriorStruct[GraphBlock::numBlocks];
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		schedule[i] = false;
		p[i].prior = PriorityMax;
		SpinLockInit(&p[i].lock);

		GraphBlock *block = GraphBlock::blocks + i;
		for (int j = 0; j < block->size; j++)
			this->InitializePriorAggr(&block->vertexPriority[j]);
	}

	this->blockPriorLock = new SpinLock[GraphBlock::numBlocks];
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		SpinLockInit(&blockPriorLock[i]);
	}


/*
	blockPriority = new float[GraphBlock::numBlocks];
	priorityLock = new SpinLock[GraphBlock::numBlocks];
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		schedule[i] = false;
		blockPriority[i] = PriorityMax;
		SpinLockInit(&priorityLock[i]);
	}
	*/

	//this->SetRandomPick();
}

void PriorScheduler::ResetForNextRun() {
	//	Reset all the scheduling bit.
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		schedule[i] = false;
		p[i].prior = PriorityMax;
	}
}

PriorScheduler::~PriorScheduler() {
	//delete [] blockPriority;
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		SpinLockDestroy(&p[i].lock);
	}
	delete [] p;
}



//	============== MaxSum-PriorScheduler. ==============

GraphBlock* MaxSumPriorScheduler::GetNextBlock(Job* job) {
	GraphBlock* block = this->GrabBlock(job);
	if (block != NULL) {
		//	Reset priority.

		if (job->taskInfo->avoidInternalPriority) {
			this->ResetPriority(block->blockID);
	//		block->ResetPriority(0.0);
		} else {
			this->LockPrior(block->blockID);
			p[block->blockID].prior = 0.0;
			for (int i = 0; i < block->size; i++)
				this->InitializePriorAggr(&block->vertexPriority[i]);
			this->UnlockPrior(block->blockID);
		}

		block->PrepareForUpdate();
	}
	return block;
}

void MaxSumPriorScheduler::PrepareForNextIteration() {
	//	Sample and determine the threshold.
	int size = GraphBlock::numBlocks;
	if (SampleSize >= size) {
		for (int i = 0; i < size; i++) {
//			samplePrior[i] = GraphBlock::blocks[i].priority;
//			samplePrior[i] = blockPriority[i];
			samplePrior[i] = p[i].prior;
		}

		std::sort(samplePrior, samplePrior + size);
		//	Max Priority Queue, use 1.0 - ratio
		rankThreshold = samplePrior[std::min((int)(size * (1.0 - ratio)), size - 1)];
	} else {
		//	Sample
		for (int i = 0; i < SampleSize; i++) {
			int sampleIndex = rand() % size;
//			samplePrior[i] = GraphBlock::blocks[sampleIndex].priority;
//			samplePrior[i] = blockPriority[sampleIndex];
			samplePrior[i] = p[sampleIndex].prior;
		}
		std::sort(samplePrior, samplePrior + SampleSize);
		//	Max Priority Queue, use 1.0 - ratio
		rankThreshold = samplePrior[std::min((int)(SampleSize * (1.0 - ratio)), SampleSize - 1)];
	}
	//fprintf(stderr, "rankThreshold = %.2f\n", rankThreshold);
	task->scheduleVertices();

	//	Reset VBlock ID.
	this->Reset();
}

void MaxSumPriorScheduler::FinishBlockUpdate(GraphBlock* gblock, Job* job, bool init) {
	//	Commit data first.
	gblock->Commit();

	if (job->taskInfo->avoidInternalPriority) {

		//	Notify its neighbors.
		for (int i = 0; i < gblock->size; i++) {
			int vertexID = gblock->vertexID[i];
			Vertex* vertex = &Vertex::vtxList[vertexID];
			if (vertex->lastCommittedDataChanged()) {
				//	Last committed data changed, update its outgoing blocks' priority.
				for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin(); it != vertex->outgoingEids.end(); it++) {
					Edge *edge = &Edge::edgeList[*it];
					if (init || edge->receiverBlockIndex != -1) {
						Vertex* dstVertex = (Vertex*)(edge->dstVtx);
						GraphBlock* receiverBlock = GraphBlock::blocks + edge->dstVtx->blockID;
						double priority = dstVertex->calculateVertexPriority(gblock->GetLocalVertexWriteData(vertex),
																			 gblock->GetLocalVertexLastCommittedData(vertex),
																			 edge);

						//	Update receiver block's priority.
						this->UpdatePrioritySum(receiverBlock->blockID, priority);
	//					receiverBlock->UpdatePrioritySum(priority);

						//	!!!	TODO(wenleix): Refactor
						if (!init && job->taskInfo->dynamicSweep) {
							//fprintf(stderr, "Trigger block %d, inBlockID %d\n", edge->dstVtx->blockID, edge->dstVtx->inBlockID);
							bool *inSchedule = receiverBlock->GetWriteVertexScheduling(job);
							inSchedule[edge->dstVtx->inBlockID] = true;
						}

					}
				}
			}
		}
	} else {

//		fprintf(stderr, "start to maxsumprior::Finish update block %d!!\n", gblock->blockID);

		//	Notify its neighbors.
		for (int i = 0; i < gblock->size; i++) {
			int vertexID = gblock->vertexID[i];

//			fprintf(stderr, "Vertex ID %d\n!", vertexID);

			Vertex* vertex = &Vertex::vtxList[vertexID];
			if (vertex->lastCommittedDataChanged()) {
				//	Last committed data changed, update its outgoing blocks' priority.
				for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin(); it != vertex->outgoingEids.end(); it++) {
					Edge *edge = &Edge::edgeList[*it];
					if (init || edge->receiverBlockIndex != -1) {
						Vertex* dstVertex = (Vertex*)(edge->dstVtx);
						GraphBlock* receiverBlock = GraphBlock::blocks + edge->dstVtx->blockID;
						double imp = dstVertex->calculateVertexPriority(gblock->GetLocalVertexWriteData(vertex),
																		gblock->GetLocalVertexLastCommittedData(vertex),
																		edge);

//						fprintf(stderr, "Lock Prior and update for block %d!\n", receiverBlock->blockID);




						int bid = receiverBlock->blockID;
						this->LockPrior(bid);

						float *vtxPrior = &receiverBlock->vertexPriority[dstVertex->inBlockID];

						float oldPrior = *vtxPrior;
						this->UpdateVertexPrior(vtxPrior, imp);
						this->IncUpdatePriorAggr(bid, oldPrior, *vtxPrior);  

						this->UnlockPrior(bid);


						/*this->LockPrior(receiverBlock->blockID);

						float oldPrior = receiverBlock->vertexPriority[dstVertex->inBlockID];
						this->UpdateVertexPrior(&receiverBlock->vertexPriority[dstVertex->inBlockID],
											    imp);
						this->IncUpdatePriorAggr(
								receiverBlock->blockID,
								oldPrior,
								receiverBlock->vertexPriority[dstVertex->inBlockID]);

						this->UnlockPrior(receiverBlock->blockID); */
//						fprintf(stderr, "Finish lock prior and update!\n"); 

						//	!!!	TODO(wenleix): Refactor
						if (!init && job->taskInfo->dynamicSweep) {
							bool *inSchedule = receiverBlock->GetWriteVertexScheduling(job);
							inSchedule[edge->dstVtx->inBlockID] = true;
						}

					}
				}
			}
		}

//		fprintf(stderr, "Done!!!\n");

	}
}

void MaxSumPriorScheduler::BarrierSetScheduleBit(int blockID) const {
	GraphBlock* b = &GraphBlock::blocks[blockID];
//	schedule[blockID] = (b->priority >= rankThreshold && b->priority != 0);
//	schedule[blockID] = (blockPriority[blockID] >= rankThreshold && blockPriority[blockID] != 0);
	schedule[blockID] = p[blockID].prior >= rankThreshold && p[blockID].prior != 0;
}

void MaxSumPriorScheduler::Initialize() {
	this->InitializeBlocks();

	schedule = new bool[GraphBlock::numBlocks];
//	blockPriority = new float[GraphBlock::numBlocks];
//	priorityLock = new SpinLock[GraphBlock::numBlocks];
	p = new PriorStruct[GraphBlock::numBlocks];

	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		schedule[i] = false;
		p[i].prior = 0;
		SpinLockInit(&p[i].lock);


		GraphBlock *block = GraphBlock::blocks + i;
		for (int j = 0; j < block->size; j++)
			this->InitializePriorAggr(&block->vertexPriority[j]);
	}

	this->blockPriorLock = new SpinLock[GraphBlock::numBlocks];
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		SpinLockInit(&blockPriorLock[i]);
	}

/*	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		schedule[i] = false;
		blockPriority[i] = 0;
		SpinLockInit(&priorityLock[i]);
	}
*/

	//this->SetRandomPick();
}

void MaxSumPriorScheduler::ResetForNextRun() {
	//	Reset all the scheduling bit.
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		schedule[i] = false;
		p[i].prior = 0;
	}
}

MaxSumPriorScheduler::~MaxSumPriorScheduler() {
	for (int i = 0; i < GraphBlock::numBlocks; i++) {
		SpinLockDestroy(&p[i].lock);
	}
	delete [] p;
}


