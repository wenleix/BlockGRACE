/*
 *  Task.h
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/17/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

#ifndef _GRACE_TASK_H_
#define _GRACE_TASK_H_

/* BRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/TaskBase.h"

namespace GRACE {

// Job Descriptor
class Task : public TaskBase {
	public:
	Task() : TaskBase() {}
	void startTask(int argc, char **argv);
};

} /* namespace GRACE */

#endif /* _GRACE_TASK_H_ */
