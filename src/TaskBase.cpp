/*
 *  TaskBase.cpp
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/18/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/TaskBase.h"
#include "GRACE/Graph.h"
#include "GRACE/Job.h"
#include "GRACE/Scheduler.h"
#include "GRACE/SparseScheduler.h"

/* Cornell Utility Headers  */

#include <DBUtl/Log.h>

using namespace DBUtl;
using namespace GRACE;


//	Check Job Information
bool TaskBase::checkTask() {
	return true;
}

void TaskBase::scheduleVertices() {
	opt = Schedule;

	for (unsigned i = 0; i < numThreads; i++) {
		jobs[i]->signalWoker = true;
		jobs[i]->signalMaster = false;
		jobs[i]->allVoteHalt = true;

		pthread_cond_signal(&(jobs[i]->cond_worker));
		pthread_mutex_unlock(&(jobs[i]->mutex));
	}

	for (unsigned i = 0; i < numThreads; i++) {
		pthread_mutex_lock(&(jobs[i]->mutex));
		while (!jobs[i]->signalMaster)
			pthread_cond_wait(&(jobs[i]->cond_master), &(jobs[i]->mutex));
	}
}

void TaskBase::SetGraphBlcokScheduler(const char* blockSchedulerType) {
	if (strcmp(blockSchedulerType, "v_static") == 0) {
		fprintf(stderr, "Static Vertex scheduler\n");
		blockScheduler = new StaticScheduler();
		blockScheduler->SetAsVertexScheduler();
		blockScheduler->setTask(this);
	} else if (strcmp(blockSchedulerType, "static") == 0) {
		fprintf(stderr, "Static Block scheduler\n");
		blockScheduler = new StaticScheduler();
		blockScheduler->setTask(this);
	} else if (strcmp(blockSchedulerType, "v_eager") == 0) {
		fprintf(stderr, "Eager Vertex scheduler\n");
		EagerScheduler *eagerScheduler = new EagerScheduler();
		eagerScheduler->setTask(this);
		eagerScheduler->SetAsVertexScheduler();
		blockScheduler = eagerScheduler;
	} else if (strcmp(blockSchedulerType, "eager") == 0) {
		fprintf(stderr, "Eager Block Scheduler\n");
		EagerScheduler *eagerScheduler = new EagerScheduler();
		eagerScheduler->setTask(this);
		blockScheduler = eagerScheduler;
	} else if (strncmp(blockSchedulerType, "v_prior", 7) == 0) {
		PriorScheduler *priorScheduler = new PriorScheduler();
		double ratio = 0.2;
		if (strlen(blockSchedulerType) > 7) {
			ratio = atof(blockSchedulerType + 7);
		}

		fprintf(stderr, "Prior Vertex Scheduler, selective ratio = %.2f\n", ratio);
		priorScheduler->SetAsVertexScheduler();
		priorScheduler->SetRatio(ratio);
		blockScheduler = priorScheduler;
		blockScheduler->setTask(this);
	} else if (strncmp(blockSchedulerType, "prior", 5) == 0) {
		PriorScheduler *priorScheduler = new PriorScheduler();
		double ratio = 0.2;
		if (strlen(blockSchedulerType) > 5) {
			ratio = atof(blockSchedulerType + 5);
		}

		fprintf(stderr, "Prior Block Scheduler, selective ratio = %.2f\n", ratio);
		priorScheduler->SetRatio(ratio);
		blockScheduler = priorScheduler;
		blockScheduler->setTask(this);
	} else if (strncmp(blockSchedulerType, "v_maxsumprior", 13) == 0) {
		MaxSumPriorScheduler *priorScheduler = new MaxSumPriorScheduler();
		double ratio = 0.2;
		if (strlen(blockSchedulerType) > 13) {
			ratio = atof(blockSchedulerType + 13);
		}

		fprintf(stderr, "MaxSumPrior Vertex Scheduler, selective ratio = %.2f\n", ratio);
		priorScheduler->SetAsVertexScheduler();
		priorScheduler->SetRatio(ratio);
		blockScheduler = priorScheduler;
		blockScheduler->setTask(this);
	} else if (strncmp(blockSchedulerType, "maxsumprior", 11) == 0) {
		MaxSumPriorScheduler *priorScheduler = new MaxSumPriorScheduler();
		double ratio = 0.2;
		if (strlen(blockSchedulerType) > 11) {
			ratio = atof(blockSchedulerType + 11);
		}

		fprintf(stderr, "MaxSumPrior Block Scheduler, selective ratio = %.2f\n", ratio);
		priorScheduler->SetRatio(ratio);
		blockScheduler = priorScheduler;
		blockScheduler->setTask(this);
	} else if (strcmp(blockSchedulerType, "v_eagersparse") == 0) {
		fprintf(stderr, "Sparse Eager Vertex scheduler\n");
		SparseEagerScheduler *sparseEagerScheduler = new SparseEagerScheduler();
		sparseEagerScheduler->setTask(this);
		sparseEagerScheduler->SetAsVertexScheduler();

		blockScheduler = sparseEagerScheduler;
	} else if (strcmp(blockSchedulerType, "v_eagersparsesort") == 0) {
		fprintf(stderr, "Sparse Eager Vertex Sort scheduler\n");
		SparseEagerScheduler *sparseEagerScheduler = new SparseEagerScheduler();
		sparseEagerScheduler->setTask(this);
		sparseEagerScheduler->SetAsVertexScheduler();
		sparseEagerScheduler->SetSortGlobaList();

		blockScheduler = sparseEagerScheduler;

	} else if (strcmp(blockSchedulerType, "eagersparse") == 0) {
		fprintf(stderr, "Sparse Eager Vertex scheduler\n");
		SparseEagerScheduler *sparseEagerScheduler = new SparseEagerScheduler();
		sparseEagerScheduler->setTask(this);

		blockScheduler = sparseEagerScheduler;
	}
	else {
		fprintf(stderr, "Not supported Scheduler: %s\n !!", blockSchedulerType);
		DBUTL_ASSERT_ALWAYS(false);
	}
}

TaskBase::~TaskBase() {
	delete blockScheduler;
}

void TaskBase::setNumGrabBlock(int num) {
	blockScheduler->SetNumBlockReserved(num);
	fprintf(stdout, "Actual Num Block Reserved: %d\n", blockScheduler->numBlockReserved);
}

void TaskBase::setBlockSweep(const char* sweep) {
	if (strcmp(sweep, "normal") == 0) {
		randomPick = false;
	}
	else if (strcmp(sweep, "random") == 0) {
		randomPick = true;
	}
	else {
		fprintf(stderr, "Unsupported Block Sweep %s\n", sweep);
		DBUTL_ASSERT_ALWAYS(false);
	}
}


