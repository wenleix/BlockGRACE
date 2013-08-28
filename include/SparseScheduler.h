/*
 *  SparseScheduler.h
 *  GRACE
 *
 *  Created by Wenlei Xie on 2/1/13.
 *  Copyright 2013 Cornell. All rights reserved.
 *
 */
#ifndef _GRACE_SPARSE_SCHEDULER_H_
#define _GRACE_SPARSE_SCHEDULER_H_

/* GRACE Headers */
#include "GRACE/Scheduler.h"

//	Including some alternating implementations for dynamic schedulers to improve their performance on sparse scheduling case.

namespace GRACE {

class SparseEagerScheduler : public AbstractBlockScheduler {
public:
	virtual GraphBlock* GetNextBlock(Job* job);

	virtual void FinishBlockUpdate(GraphBlock* gblock, Job* job, bool init = false);

	virtual void PrepareForNextIteration();

	virtual void Initialize();

	virtual void ResetForNextRun() {
		DBUTL_ASSERT_ALWAYS(false);
	}

	SparseEagerScheduler() : sortList(false) {}

	virtual ~SparseEagerScheduler() {
		delete [] scheduleNext;
	}

	virtual void BarrierSetScheduleBit(int blockID) const {
		DBUTL_ASSERT_ALWAYS(false);
	}

	void SetSortGlobaList() { sortList = true; }

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
	bool sortList;
	//	The global scheduling list.
	std::vector<int> globalScheduleList;
	int currentScheduleIdx;			//	The schedule index in the global schedule list.

	bool* scheduleNext;
};


} /* namespace GRACE */

#endif /* _GRACE_SCHEDULER_H_ */


