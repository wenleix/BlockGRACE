/*
 *  Driver.cpp
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/17/12.
 *  Updated by Wenlei Xie on 5/9/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/Graph.h"
#include "GRACE/Driver.h"
#include "GRACE/Job.h"
#include "GRACE/Profile.h"
#include "GRACE/GraphBlock.h"
#include "GRACE/Scheduler.h"

/* Cornell Utility Headers  */

#include <DBUtl/Log.h>

/* Common STL Headers */

#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <time.h>
#include <pthread.h>
#include "ptimer.h"

using namespace DBUtl;
using namespace GRACE;

Task * GRACE::task;

//	Check if the task can stop...
bool GRACE::canStop() {
	//	If no vertex is updated in the last iteration, stop.
	uint32_t updateCounter = 0;
	for(int i = 0; i < task->numThreads; i++) {
		updateCounter += task->jobs[i]->vertexUpdateCounter;
	}

	task->vertexUpdateCounter = updateCounter;
	if (task->stop == TaskBase::Convergence) {
		bool halt = true;
		for (int i = 0; i < task->numThreads; i++) {
			if (!task->jobs[i]->allVoteHalt) {
				halt = false;
				break;
			}
		}
		return halt;
	} else /* stop == Ticks */{
		return (task->curEpoch * task->numEpcTicks > task->numTotalTicks);
	}
}

// Initialize computation ...
void GRACE::prepareToStart(void * params) {
	Job * job =(Job *)params;

   std::vector<Vertex>::iterator it;

   for (it = Vertex::vtxList.begin(); it != Vertex::vtxList.end(); ++it) {
	   it->onInitialize(job);
   }

   for (int i = 0; i < GraphBlock::numBlocks; i++)
	   job->taskInfo->blockScheduler->FinishBlockUpdate(&GraphBlock::blocks[i], job, true /* init */);
}

//	Worker Thread
void* GRACE::workerThread(void * params) {
	Job * job =(Job *)params;

	while (true) {
		// Phase 1: prepare for the next iteration.

		while (true) {
			// Wait for master for commands in onPrepare
			pthread_mutex_lock( &(job->mutex) );
			while ( !job->signalWoker )
				pthread_cond_wait( &(job->cond_worker), &(job->mutex));

			// Check if the master decide to finish prepare phase
			if ( job->ready ) break;

			switch (job->taskInfo->opt) {
			case TaskBase::NoCmd:
				DBUTL_ASSERT_ALWAYS(false);
				break;
			case TaskBase::Schedule:
				//fprintf(stderr, "Start schedule! Current Epoch %d\n", job->taskInfo->curEpoch);

				int numBlocksPerThread = GraphBlock::numBlocks / job->taskInfo->numThreads;

				int blockBeginID = job->threadID * numBlocksPerThread;
				int blockEndID = (job->threadID == job->taskInfo->numThreads - 1)
										 ? GraphBlock::numBlocks
										 : (job->threadID + 1) * numBlocksPerThread;

				//fprintf(stderr, "begin: %d, end: %d\n", blockBeginID, blockEndID);

				for (int i = blockBeginID; i < blockEndID; i++) {
					//	TODO(wenleix): Here we assume the numEpcTicks is 1.
//					GraphBlock::blocks[i].scheduled = (job->taskInfo->curEpoch == 1 ||
//													   job->taskInfo->blockScheduler->schedulePredicate(&GraphBlock::blocks[i]));
					job->taskInfo->blockScheduler->BarrierSetScheduleBit(i);
				}
				//	To get rid of compiler warning.
				break;
			}

			job->signalMaster = true;
			job->signalWoker = false;
			pthread_cond_signal( &job->cond_master );
			pthread_mutex_unlock( &job->mutex );
		}

		// Phase 2: finish work for this epoch

		// Check if master calls for halt
		if ( job->finish ) {
			break;
		}

		// Execute until the end of the tick
		advanceToTick(job);

		job->signalMaster = true;
		job->signalWoker = false;
		pthread_cond_signal( &job->cond_master );
		pthread_mutex_unlock( &job->mutex );
	}

    return NULL;

}

//  Return: True if the block data get update
//	False if the block data doesn't update
bool GRACE::BlockUpdate(GraphBlock* block, Job* job) {
	job->blockProcedCnt++;

	bool getUpdated = false;
	block->convergent = false;


	//	Dynamic Sweep
	if (job->taskInfo->dynamicSweep) {
//		fprintf(stderr, "DynamicSweep!\n");
		int blockID = block->blockID;

		bool *scheduling = block->GetReadVertexScheduling(job);
//		bool *isBoundary = job->taskInfo->inSchedule.boundaryVertex[blockID];

		for (int inner = 0; ; inner++) {
			bool updated = false;
			for (int i = 0; i < block->GetBlockSize(); i++) {
//				if (inner == 0 || scheduling[i]) {
				//				if ((inner == 0 && (isBoundary[i] || job->taskInfo->curEpoch == 1)) || scheduling[i]) {
				if ((inner == 0 && block->firstTimeUpdate) || scheduling[i]) {
					int vid = block->GetVertexID(i);
					Vertex* vertex = &Vertex::vtxList[vid];
					if (VertexUpdate(vertex, block)) {
						updated = true;
						//	Trigger outgoing vertices;
						for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin();
								it != vertex->outgoingEids.end(); it++) {
							Vertex* dstVertex = (Vertex*)(Edge::edgeList[*it].dstVtx);
							//	TODO(wenleix): We can reduce scheduling overhead by separating the local vertices and the ghost vertices.
							if (dstVertex->blockID == blockID) {
								//	Schedule it.
								int inBlockID = dstVertex->inBlockID;
								scheduling[inBlockID] = true;
							}
						}
					}

					scheduling[i] = false;
					job->vertexUpdateCounter++;
					job->numEdgeVisited += vertex->numInEdges;
				}
			}

			getUpdated |= updated;

			//	Terminate at block convergence
			if (!updated) {
				block->convergent = true;
				return getUpdated;
			}
		}
	}


	bool enableAlternateSweep = job->taskInfo->alternatingSweep;
	bool earlyTermination = job->taskInfo->earlyTermination;
	int numInnerIteration = job->taskInfo->numInnerIteration;

//	fprintf(stderr, "!!!============BlockUpdate!!\n");

	for (int inner = 0; inner < numInnerIteration; inner++) {
		bool updated = false;
		int blockID = block->blockID;
		AbstractBlockScheduler *scheduler = job->taskInfo->blockScheduler;


		//	Internal Priority tracking is enabled only for the last inner sweep.
		bool enableInteralPriority = (!job->taskInfo->avoidInternalPriority) &&
				(inner == numInnerIteration - 1);

		if ((!enableAlternateSweep) || inner % 2 == 0) {
			VertexData old_vdata;

			for (int i = 0; i < block->GetBlockSize(); i++) {
				int vid = block->GetVertexID(i);
				//	Debug!
				//fprintf(stdout, "Update Vertex %d\n", vid);
				Vertex* vertex = &Vertex::vtxList[vid];
				if (enableInteralPriority) {
					//	Copy the old data
					old_vdata = *(block->GetLocalVertexWriteData(vertex));
				}

				bool vtxUpdated = VertexUpdate(vertex, block);
				updated |= vtxUpdated;

				if (vtxUpdated && enableInteralPriority) {
					VertexData *newData = block->GetLocalVertexWriteData(vertex);
					//	Trigger the outgoing vertices
					for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin();
						 it != vertex->outgoingEids.end(); it++) {
						Edge* e = &Edge::edgeList[*it];
						if (e->receiverBlockIndex == -1) {
							//	Same block
							Vertex* dstVtx = (Vertex*)(e->dstVtx);
							if (dstVtx->inBlockID < vertex->inBlockID) {
								float imp = dstVtx->calculateVertexPriority(&old_vdata, newData, e);

								//	Need to lock it;
								if (dstVtx->isBoundaryVertex) {
									scheduler->LockPrior(blockID);
									float oldPrior = block->vertexPriority[dstVtx->inBlockID];
									scheduler->UpdateVertexPrior(
											&block->vertexPriority[dstVtx->inBlockID], imp);
									scheduler->IncUpdatePriorAggr(blockID, oldPrior,
																  block->vertexPriority[dstVtx->inBlockID]);
									scheduler->UnlockPrior(blockID);
								} else {
									//	Internal Vertex, no need to lock it.
									scheduler->UpdateVertexPrior(
											&block->vertexPriority[dstVtx->inBlockID], imp);
								}
							}
						}
					}
				}

				job->vertexUpdateCounter++;
				job->numEdgeVisited += vertex->numInEdges;
			}
		} else {
			for (int i = block->GetBlockSize() - 1; i >= 0; i--) {
				int vid = block->GetVertexID(i);
				Vertex* vertex = &Vertex::vtxList[vid];
				updated |= VertexUpdate(vertex, block);
				job->vertexUpdateCounter++;
				job->numEdgeVisited += vertex->numInEdges;
			}
		}
		getUpdated |= updated;

		//	Terminate at block convergence
		if (!updated) {
			block->convergent = true;
			if (earlyTermination) {
				job->blockInnerProcedCnt += inner + 1;
				return getUpdated;
			}
		}
	}

	if (!job->taskInfo->avoidInternalPriority) {
		int blockID = block->blockID;
		AbstractBlockScheduler *scheduler = job->taskInfo->blockScheduler;

		for (int i = 0; i < block->GetBlockSize(); i++) {
			int vid = block->GetVertexID(i);
			Vertex* vertex = &Vertex::vtxList[vid];
			if (!vertex->isBoundaryVertex) {
				scheduler->LockPrior(blockID);
				scheduler->PriorAggrAdd(blockID, block->vertexPriority[i]);
				scheduler->UnlockPrior(blockID);
			}
		}

	}

	job->blockInnerProcedCnt += numInnerIteration;
	return getUpdated;
}

/*
 * Advance to tick t.
 * Main flow:
 * 	while (more work) {
 * 		Grab a block.
 * 		Do the work on the block.
 * 		Finish block.
 * 	}
 */
void GRACE::advanceToTick(Job* job) {
//	fprintf(stderr, "Current Epoch: %u\n", job->taskInfo->curEpoch);

    while (true) {
    	GraphBlock* block = job->taskInfo->blockScheduler->GetNextBlock(job);
    	if (block == NULL)
    		break;

		//	Debug!
/*		fprintf(stderr, "Update Block %d\n", block->blockID);
		fprintf(stderr, "First three VertexID : ");
		for (int i = 0; i < 3 && i < block->GetBlockSize(); i++)
			fprintf(stderr, "%d ", block->GetVertexID(i));
		fprintf(stderr, "\n");  */

		job->allVoteHalt &= (!BlockUpdate(block, job));
		block->firstTimeUpdate = false;
		job->taskInfo->blockScheduler->FinishBlockUpdate(block, job);
    }
}

/*
 * Master Thread: driver loop
 *
 * Synchronize barriers every `numEpcTicks' ticks until the fix point is reached ...
 */
void GRACE::run(bool multiRun, bool firstInMultiRun) {
	//	Initialize the scheduler.
	if (multiRun && !firstInMultiRun) {
		//	Nothing to do.
	} else {
		task->blockScheduler->Initialize();

		for (int i = 0; i < GraphBlock::numBlocks; i++) {
			for (int j = 0; j < GraphBlock::blocks[i].size; j++) {
				int vertexID = GraphBlock::blocks[i].GetVertexID(j);
				Vertex *vertex = &Vertex::vtxList[vertexID];
				vertex->isBoundaryVertex = false;

				for (std::vector<uint32_t>::iterator it = vertex->incomingEids.begin();
						it != vertex->incomingEids.end(); it++) {
					Vertex *srcVertex = (Vertex*)(Edge::edgeList[*it].srcVtx);
					if (srcVertex->blockID != i) {
						vertex->isBoundaryVertex = true;
						break;
					}
				}
			}
		}


		if (task->dynamicSweep) {
			task->inSchedule.numBlocks = GraphBlock::numBlocks;
			task->inSchedule.schedulingBit[0] = new bool*[GraphBlock::numBlocks];
			task->inSchedule.schedulingBit[1] = new bool*[GraphBlock::numBlocks];
			task->inSchedule.boundaryVertex = new bool*[GraphBlock::numBlocks];

			for (int i = 0; i < GraphBlock::numBlocks; i++) {
				task->inSchedule.schedulingBit[0][i] = new bool[GraphBlock::blocks[i].size];
				task->inSchedule.schedulingBit[1][i] = new bool[GraphBlock::blocks[i].size];
				task->inSchedule.boundaryVertex[i] = new bool[GraphBlock::blocks[i].size];

				for (int j = 0; j < GraphBlock::blocks[i].size; j++) {
					task->inSchedule.schedulingBit[0][i][j] = false;
					task->inSchedule.schedulingBit[1][i][j] = false;
					task->inSchedule.boundaryVertex[i][j] = false;

					int vertexID = GraphBlock::blocks[i].GetVertexID(j);
					Vertex *vertex = &Vertex::vtxList[vertexID];
					for (std::vector<uint32_t>::iterator it = vertex->incomingEids.begin();
							it != vertex->incomingEids.end(); it++) {
						Vertex *srcVertex = (Vertex*)(Edge::edgeList[*it].srcVtx);
						if (srcVertex->blockID != i) {
							task->inSchedule.boundaryVertex[i][j] = true;
							break;
						}
					}
				}
			}
		}

		if (task->randomPick)
			task->blockScheduler->SetRandomPick();
	}

	fprintf(stderr, "Here!! Start run!!!\n");

	// Initialize the jobs (i.e. the worker thread context)
	for (unsigned i = 0; i < task->numThreads; i++) {
		Job * job = new Job(task, i);
		task->jobs.push_back(job);
	}
	task->curEpoch++;

//	fprintf(stderr, "!!!Start to parepare!!!");

	// TODO: parallel preparing?
	prepareToStart(task->jobs[0]);

//	fprintf(stderr, "!!!!!!!Prepare To start finished!!!");

	Profile::total_pt.start();

	// Create threads
	for (unsigned i = 0; i < task->numThreads; i++) {
		//	Initialize jobs
		task->jobs[i]->allVoteHalt = true;
		task->jobs[i]->signalWoker = false;
		task->jobs[i]->signalMaster = false;
		task->jobs[i]->finish = false;
		task->jobs[i]->ready = false;
		task->jobs[i]->vertexUpdateCounter = 0;
		task->jobs[i]->numEdgeVisited = 0;
		task->jobs[i]->blockProcedCnt = 0;
		task->jobs[i]->blockInnerProcedCnt = 0;

		int rc = -1;
		pthread_t pID;

		const std::vector<uint32_t> &seq = task->affinitySeq;
		if (seq.size() != 0) {	//	Affinity Sequence

#ifndef __APPLE__
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			cpu_set_t cpu_set;
			CPU_ZERO(&cpu_set);

			CPU_SET(seq[i % seq.size()], &cpu_set);
			if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set), &cpu_set) != 0) {
				fprintf(stderr, "Set Affinity Failed!\n");
				DBUTL_ASSERT_ALWAYS(false);
			}
			rc = pthread_create(&pID, &attr, workerThread, (void*)(task->jobs[i]));
#else
            //  Not support on MacOS yet
            DBUTL_ASSERT_ALWAYS(false);
#endif

		}
		else {	//	No affinity sequence
			rc = pthread_create(&pID, NULL, workerThread, (void*) (task->jobs[i]));
		}

		DBUTL_ASSERT_ALWAYS(rc == 0);
		task->workers.push_back(pID);
	}

	//	Start running.
	while (true) {
		//	Phase 1: Prepare for the epoch by calling PrefpareForNextIteration() (same as onPrepare() in GRACE-alpha)

		Profile::prepare_pt.start();

		task->blockScheduler->PrepareForNextIteration();
		task->opt = TaskBase::NoCmd;					//	Clear command.
	
		Profile::prepare_pt.finish();
		Profile::prepare_time += Profile::prepare_pt.realResult();

		//	Profile for the actual iteration	
		Profile::iter_pt.start();

		// Phase 2: Let workers finish for epoch
		for (unsigned i = 0; i < task->numThreads; i++) {
			task->jobs[i]->allVoteHalt = true;
			task->jobs[i]->ready = true;
			task->jobs[i]->signalWoker = true;
			task->jobs[i]->signalMaster = false;

			pthread_cond_signal( &(task->jobs[i]->cond_worker) );
			pthread_mutex_unlock( &(task->jobs[i]->mutex) );
		}

		// Wait for all workers to finish, the main barrier.
		for (unsigned i = 0; i < task->numThreads; i++) {
			pthread_mutex_lock( &(task->jobs[i]->mutex) );
			while ( !task->jobs[i]->signalMaster )
				pthread_cond_wait( &(task->jobs[i]->cond_master), &(task->jobs[i]->mutex));

			task->jobs[i]->ready = false;
		}
	
		Profile::iter_pt.finish();
		Profile::iter_time += Profile::iter_pt.realResult();

		//	Phase 3: Check for stop at end of each tick.
		if (!multiRun && task->curEpoch % 50 == 0)
			printf("Now the tick for %u\n", task->curEpoch);
		task->curEpoch++;

		// If the computation have already halted,
		// inform the workers before jumping out of iterations
		if (canStop()) {
			for (unsigned i = 0; i < task->numThreads; i++) {
				task->jobs[i]->ready = true;	//	Skip the scheduling phase.
				task->jobs[i]->finish = true;
				task->jobs[i]->signalWoker = true;

				pthread_cond_signal(&(task->jobs[i]->cond_worker));
				pthread_mutex_unlock(&(task->jobs[i]->mutex));
			}
			break;
		}

		Profile::procedCnt = task->vertexUpdateCounter;
		//	Debug
//		fprintf(stderr, "Iteration: %d, # Vertex Update: %d\n", task->curEpoch, Profile::procedCnt);
	}

	// Recycle worker threads
	for (unsigned i = 0; i < task->numThreads; i++) {
		pthread_join(task->workers[i], NULL);
	}
	task->workers.clear();

	Profile::procedCnt = 0;
	Profile::edgeVisitCount = 0;
	Profile::blockProcedCnt = 0;
	Profile::blockInnerProcedCnt = 0;
	for (unsigned i = 0; i < task->numThreads; i++) {
		Profile::procedCnt += task->jobs[i]->vertexUpdateCounter;
		Profile::edgeVisitCount += task->jobs[i]->numEdgeVisited;
		Profile::blockProcedCnt += task->jobs[i]->blockProcedCnt;
		Profile::blockInnerProcedCnt += task->jobs[i]->blockInnerProcedCnt;
	}

	Profile::total_pt.finish();
	Profile::total_time = Profile::total_pt.realResult();
	Profile::accumulate_time += Profile::total_pt.realResult();
	if (!multiRun) {
		printTime();
		printf("In total %d iterations\n", task->curEpoch * task->numEpcTicks - 1);
	}
	Profile::numIter = task->curEpoch * task->numEpcTicks - 1;
}

void GRACE::registerTask(Task * _task) {
	task = _task; 
}
