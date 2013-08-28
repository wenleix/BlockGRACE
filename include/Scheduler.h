/*
 *  Scheduler.h
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/17/12.
 *  Updated by Wenlei Xie on 5/9/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */
#ifndef _GRACE_SCHEDULER_H_
#define _GRACE_SCHEDULER_H_

/* GRACE Headers */
#include "GRACE/Config.h"
#include "GRACE/ThreadTools.h"
#include "GRACE/TaskBase.h"
#include "GRACE/Job.h"

#include <DBUtl/Log.h>

#include <cstdio>
#include <vector>
#include <map>

namespace GRACE {
	const int SampleSize = 1000;
	const float PriorityMax = 1e10;

	class Vertex;
	class Job;
	class GraphBlock;

	//	TODO(wenleix): Many scheduling code are duplicate, and we can factor them into "Scheduler" and "Policy".
	//	as we did in GRACE-alhpa.
	class AbstractBlockScheduler {
	public:
		AbstractBlockScheduler() : randomPick(false), scheduleIndex(NULL), numBlockReserved(-1),
			schedule(NULL), isVertexScheduler(false), blockPriorLock(NULL) {
			InitLock();
		}

		virtual ~AbstractBlockScheduler();

		virtual GraphBlock* GetNextBlock(Job* job) = 0;

		// Commit the update, update priority, etc...
		virtual void FinishBlockUpdate(GraphBlock* g, Job* job, bool init = false) = 0;

		virtual void PrepareForNextIteration() = 0;

		//	Call it after the graph is read. Initialize, for example, the partition array.
		virtual void Initialize() = 0;

		virtual void ResetForNextRun() = 0;

		//	Only called at barrier
		virtual void BarrierSetScheduleBit(int blockID) const = 0;


		//	TODO(wenleix): We currently have two independent way for priority aggregation,
		//					need to fix it.
		virtual void InitializePriorAggr(float *p) = 0;
		virtual void UpdateVertexPrior(float *p, float imp) = 0;
		virtual void PriorAggrAdd(int blockID, float val) = 0;
		virtual void IncUpdatePriorAggr(int blockID, float oldValue, float newValue) = 0;


		void SetNumBlockReserved(int numBlock) { this->numBlockReserved = numBlock; }

		void SetAsVertexScheduler() { this->isVertexScheduler = true; }

		bool IsVertexScheduler() { return this->isVertexScheduler; }

		void setTask(TaskBase* t) { task = t; }

        int numBlockReserved;

		//	TODO(wenleix): This function should be called after block is initalized.
		void SetRandomPick();

		void LockPrior(int blockID) {
			SpinLockLock(&blockPriorLock[blockID]);
		}

		void UnlockPrior(int blockID) {
			SpinLockUnlock(&blockPriorLock[blockID]);
		}

	private:
        SpinLock spin;

        inline void InitLock() {
            SpinLockInit(&spin);
		}

		inline void DestroyLock() {
            SpinLockDestroy(&spin);
		}

	protected:
		inline void Lock() {
            SpinLockLock(&spin);
		}

		inline void Unlock() {
            SpinLockUnlock(&spin);
		}

		TaskBase* task;

		void InitializeBlocks();

		void Reset() {
			currentBlockID = 0;
		}

		GraphBlock* GrabBlock(Job* job);

		bool* schedule;	//	The schedule bit array.
		bool isVertexScheduler;

        SpinLock *blockPriorLock;

	private:
		bool randomPick;		//	Pick the next block in a fixed random sequence.
		int* scheduleIndex;		//	The fixed random sequence to pick up the next block.
		int currentBlockID;
	};



	//	Static scheduler: Schedule all the blocks in each iteration.
	class StaticScheduler : public AbstractBlockScheduler {
	public:
		virtual GraphBlock* GetNextBlock(Job *job);


		virtual void FinishBlockUpdate(GraphBlock* g, Job* job, bool init = false);

		virtual void PrepareForNextIteration() { 
/*			reversed = !reversed;
			if (!reversed) {
				curPartitionID = 0; 
			} else {
				curPartitionID = partition.size() - 1;
			} */

			this->Reset();
		}
		virtual void Initialize();

		virtual void ResetForNextRun() {
			//	Nothing to do for Static Scheduler.
		}

		virtual ~StaticScheduler() {}

		virtual void BarrierSetScheduleBit(int blockID) const {
			//	Don't need it in static scheduler.
			DBUTL_ASSERT_ALWAYS(false);
		}

		virtual void PriorAggrAdd(int blockID, float val) {
			DBUTL_ASSERT_ALWAYS(false);
		}

		virtual void InitializePriorAggr(float *p) {
			DBUTL_ASSERT_ALWAYS(false);
		}

		virtual void UpdateVertexPrior(int blockID, float imp) {
			DBUTL_ASSERT_ALWAYS(false);
		}

		virtual void IncUpdatePriorAggr(int blockID, float oldValue, float newValue) {
			DBUTL_ASSERT_ALWAYS(false);
		}

		virtual void UpdateVertexPrior(float *p, float imp) {
			DBUTL_ASSERT_ALWAYS(false);
		}


	private:
		//	TODO(wenleix): Use it as a parameter.
//		bool reversed;
	};



	//	Eager Scheduler: Only schedule those blocks whose boundary value changes or it is not convergent yet.
	class EagerScheduler : public AbstractBlockScheduler {
	public:
		virtual GraphBlock* GetNextBlock(Job* job);

		//virtual void FinishBlockUpdate(GraphBlock* gblock);

		virtual void FinishBlockUpdate(GraphBlock* g, Job* job, bool init = false);

		virtual void PrepareForNextIteration() {
			task->scheduleVertices();
			this->Reset();
		}

		virtual void Initialize();

		virtual void ResetForNextRun();

		void BarrierSetScheduleBit(int blockID) const {
/*			if (task->curEpoch == 1) {
				if (!scheduleNext[blockID]) {
					fprintf(stderr, "INFO: not scheduled for block %d\n", blockID);
				}
			}*/

			schedule[blockID] = scheduleNext[blockID];
			scheduleNext[blockID] = false;
		}

		EagerScheduler() { }

		virtual ~EagerScheduler() {
			delete [] scheduleNext;
		}

		virtual void InitializePriorAggr(float *p) {
			DBUTL_ASSERT_ALWAYS(false);
		}

		virtual void PriorAggrAdd(int blockID, float val) {
			DBUTL_ASSERT_ALWAYS(false);
		}

		virtual void UpdateVertexPrior(float *p, float imp) {
			DBUTL_ASSERT_ALWAYS(false);
		}

		virtual void IncUpdatePriorAggr(int blockID, float oldValue, float newValue) {
			DBUTL_ASSERT_ALWAYS(false);
		}


	private:
		bool* scheduleNext;
	};




	//	Prior Scheduler.
	//	The default prior scheduler is a Min-Min Prior Scheduler.
	class PriorScheduler : public AbstractBlockScheduler {
	public:
		virtual GraphBlock* GetNextBlock(Job* job);

		//virtual void FinishBlockUpdate(GraphBlock* gblock);

		virtual void FinishBlockUpdate(GraphBlock* g, Job* job, bool init = false);
		virtual void PrepareForNextIteration();

		virtual void Initialize();

		virtual void ResetForNextRun();

		void SetRatio(float schedulingRatio) {
			ratio = schedulingRatio;
		}

		virtual void BarrierSetScheduleBit(int blockID) const;

		virtual ~PriorScheduler();

		void UpdatePriorityMin(int blockID, float prior) {
			SpinLockLock(&p[blockID].lock);
			if (prior < p[blockID].prior)
				p[blockID].prior = prior;
			SpinLockUnlock(&p[blockID].lock);
		}

		void ResetPriority(int blockID) {
			SpinLockLock(&p[blockID].lock);
			p[blockID].prior = PriorityMax;
			SpinLockUnlock(&p[blockID].lock);
		}

		virtual void InitializePriorAggr(float *p) {
			*p = PriorityMax;
		}

		virtual void UpdateVertexPrior(float *p, float imp) {
			if (imp < *p)
				*p = imp;
		}

		virtual void PriorAggrAdd(int blockID, float val) {
			if (val < p[blockID].prior)
				p[blockID].prior = val;
		}

		virtual void IncUpdatePriorAggr(int blockID, float oldValue, float newValue) {
			if (newValue < p[blockID].prior) {
				p[blockID].prior = newValue;
			}
		}


	private:
		float ratio;
		float rankThreshold;
		float samplePrior[SampleSize];

		struct PriorStruct {
			float prior;
			SpinLock lock;
		};
		PriorStruct* p;

//		float* blockPriority;
//		SpinLock* priorityLock;
	};



	//	MaxSum Prior Scheduler
	//	Currently it is specific for PageRank.
	class MaxSumPriorScheduler : public AbstractBlockScheduler {
	public:
		virtual GraphBlock* GetNextBlock(Job* job);

		//virtual void FinishBlockUpdate(GraphBlock* gblock);

		virtual void FinishBlockUpdate(GraphBlock* g, Job* job, bool init = false);

		virtual void PrepareForNextIteration();

		virtual void Initialize();

		virtual void ResetForNextRun();

		virtual void BarrierSetScheduleBit(int blockID) const;

		void SetRatio(double schedulingRatio) {
			ratio = schedulingRatio;
		}

		virtual ~MaxSumPriorScheduler();

		void UpdatePrioritySum(int blockID, float prior) {
			SpinLockLock(&p[blockID].lock);
			p[blockID].prior += prior;
			SpinLockUnlock(&p[blockID].lock);
		}

		void ResetPriority(int blockID) {
			SpinLockLock(&p[blockID].lock);
			p[blockID].prior = 0.0;
			SpinLockUnlock(&p[blockID].lock);
		}

		virtual void InitializePriorAggr(float *p) {
			*p = 0.0;
		}

		virtual void UpdateVertexPrior(float *p, float imp) {
			*p += imp;
		}

		virtual void PriorAggrAdd(int blockID, float val) {
			p[blockID].prior += val;
		}

		virtual void IncUpdatePriorAggr(int blockID, float oldValue, float newValue) {
			p[blockID].prior += newValue - oldValue;
		}


	private:
		float ratio;
		float rankThreshold;
		float samplePrior[SampleSize];

//		float* blockPriority;
//		SpinLock* priorityLock;
		struct PriorStruct {
			float prior;
			SpinLock lock;
		};
		PriorStruct* p;
	};

} /* namespace GRACE */

#endif /* _GRACE_SCHEDULER_H_ */


