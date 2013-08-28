/*
 *  Job.cpp
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/18/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/Job.h"
#include "GRACE/GraphBase.h"
#include "GRACE/TaskBase.h"
#include "GRACE/Profile.h"


/* Cornell Utility Headers  */

#include <DBUtl/Log.h>

using namespace DBUtl;
using namespace GRACE;

//	Job Constructor
Job::Job(TaskBase * task, uint16_t tID) {
	taskInfo = task;
	threadID = tID;
	vertexUpdateCounter = 0;
	numEdgeVisited = 0;

	loadAndFirstInnerTime = 0.0;
	restInnerTime = 0.0;

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond_worker, NULL);
	pthread_cond_init(&cond_master, NULL);

	this->reservedBeginBlockID = -1;
	this->reservedEndBlockID = -1;
}
