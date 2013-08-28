/*
 *  Profile.cpp
 *  GRACE
 *
 *  Created by Guozhang Wang on 2/11/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

#include "GRACE/Profile.h"
#include <DBUtl/Log.h>

#include <cstdio>

using namespace DBUtl;

namespace GRACE{

	double Profile::total_time = 0;
	double Profile::accumulate_time = 0;
	double Profile::iter_time = 0.0;
	double Profile::prepare_time = 0.0;
	int Profile::numIter = 0;
	long long Profile::procedCnt = 0;
	long long Profile::edgeVisitCount = 0;
	DBUtl::PTimer Profile::total_pt;
	DBUtl::PTimer Profile::iter_pt;
	DBUtl::PTimer Profile::prepare_pt;

	int Profile::blockProcedCnt = 0;
	int Profile::blockInnerProcedCnt = 0;

	void printTime() {
		DBUTL_LOG_FPRINTF( Log::TRACE, ("Total Time after start each vtx:\t%fs\n", Profile::total_time)); fflush(stdout);
		DBUTL_LOG_FPRINTF( Log::TRACE, ("Total Time for the preparation:\t%fs\n", Profile::prepare_time)); fflush(stdout);
		DBUTL_LOG_FPRINTF( Log::TRACE, ("Total Time for the iterations:\t%fs\n", Profile::iter_time)); fflush(stdout);

///		DBUTL_LOG_FPRINTF( Log::TRACE, ("Phase One Parallel Time:\t%fs\n", Profile::pllone_time)); fflush(stdout);
//		DBUTL_LOG_FPRINTF( Log::TRACE, ("Phase Two Parallel Time:\t%fs\n\n", Profile::plltwo_time)); fflush(stdout);

//		DBUTL_LOG_FPRINTF( Log::TRACE, ("Times of MsgBuffer Alloc:\t%d\n", Profile::bufferAllocCnt)); fflush(stdout);
		DBUTL_LOG_FPRINTF( Log::TRACE, ("Times of Proceed Function Call:\t%lld\n", Profile::procedCnt)); fflush(stdout);

		DBUTL_LOG_FPRINTF( Log::TRACE, ("Num of Block Proceed Function Call:\t%d\n", Profile::blockProcedCnt)); fflush(stdout);
		DBUTL_LOG_FPRINTF( Log::TRACE, ("Num of Block Sweepings:\t%d\n", Profile::blockInnerProcedCnt)); fflush(stdout);
	}

}
