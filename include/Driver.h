/*
*  Driver.h
*  GRACE
*
*  Created by Guozhang Wang on 1/17/12.
*  Updated by Wenlei Xie on 5/9/12.
*  Copyright 2012 Cornell. All rights reserved.
*
*/
#ifndef _GRACE_DRIVER_H_
#define _GRACE_DRIVER_H_

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/Task.h"

namespace GRACE {

	extern Task * task;	    // Job information used

	void* workerThread(void * params);

	void registerTask(Task * _task);

	void checkStateForTick(uint32_t tick);

	void prepareToStart(void * params);

	void advanceToTick(Job* job);

	void printTime();

	bool BlockUpdate(GraphBlock* g, Job* job);

	void run(bool multiRun = false, bool firstInMultiRun = false);

	bool canStop();

} /* namespace GRACE */

#endif /* _GRACE_DRIVER_H_ */


