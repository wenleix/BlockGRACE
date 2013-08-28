/*
 *  GRACE
 *  Profile.h
 *
 *  Created by Wenlei Xie on 2/11/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

#ifndef _GRACE_PROFILE_H_
#define _GRACE_PROFILE_H_

#include "ptimer.h"

namespace GRACE {
	struct Profile {
		static DBUtl::PTimer total_pt;
		static DBUtl::PTimer iter_pt;
		static DBUtl::PTimer prepare_pt;

		static double accumulate_time;
		static double total_time;
		static double iter_time;
		static double prepare_time;
		static long long procedCnt;
		static long long edgeVisitCount;
		static int numIter;
		static int blockProcedCnt;
		static int blockInnerProcedCnt;
	};

	/*	For profile */
	void printTime();
}

#endif /* _GRACE_PROFILE_H_ */
