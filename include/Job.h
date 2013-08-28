/*
 *  Job.h
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/16/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

#ifndef _GRACE_JOB_H_
#define _GRACE_JOB_H_

/* GRACE Headers */

#include "GRACE/Config.h"
#include "ptimer.h"

/* Cornell Utility Headers  */

#include <DBUtl/Log.h>

/* Common STL Headers  */

#include <pthread.h>
#include <vector>

using namespace DBUtl;


namespace GRACE {
	class TaskBase;
	class VertexBase;

	/*
	 * Job Descriptor
	 * Contains the thread context.
	 */
	class Job {

	/* Config Fields */
	public:
		uint16_t threadID;					// ID of the job thread
		int vertexUpdateCounter;			// Number of vertex update function calls for this job
		int numEdgeVisited;					// Better cost model: count the nubmer of edges visited.
		int blockProcedCnt;
		int blockInnerProcedCnt;

		TaskBase * taskInfo;				// Parent task of the job

		bool allVoteHalt, curVoteHalt;		//	Does all the vertices agree for halt?

		bool finish;						// Is this job finished already?
		bool ready;							// Has the prepare phase finished for the tick already?

		bool signalWoker;					// Signal waited by worker
		bool signalMaster;					// Signal waited by master

		pthread_mutex_t mutex;				// Mutex used for the conditional
		pthread_cond_t cond_worker;			// Conditional used for move on collecting statistics
		pthread_cond_t cond_master;			// Conditional used for move on collecting statistics

		int reservedBeginBlockID;			//	Blocks reserved by this threads.
		int reservedEndBlockID;

		std::vector<int> localScheduleList;

		//	Profile
		DBUtl::PTimer loadAndFirstInner_pt;
		DBUtl::PTimer restInner_pt;

		double loadAndFirstInnerTime;
		double restInnerTime;

		/* Functions */
	public:
		Job(TaskBase * task, uint16_t _tID);

		~Job() {
			pthread_mutex_destroy( &mutex );
			pthread_cond_destroy( &cond_worker );
			pthread_cond_destroy( &cond_master );
		}

	public:
		inline void voteForHalt() {
			curVoteHalt = true;
		}
	};

} /* namespace GRACE */


#endif /* _GRACE_JOB_H_ */
