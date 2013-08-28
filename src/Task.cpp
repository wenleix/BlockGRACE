/*
 *  Task.cpp
 *  GRACE
 *
 *  User instantiated task for original PageRank
 */

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/Task.h"

/* Common STL Headers */

#include <time.h>

using namespace GRACE;

/* -------- User must instantiate functions below -------- */

/* 
 * Set the config information
 */
void Task::startTask(int argc, char **argv)
{
	// Starting from the third parameter, reading and parsing
	unsigned epochSize = ((argc > 2) ? atoi(argv[2]) : 1);		// number of ticks per epoch
	unsigned jobThreads = ((argc > 3) ? atoi(argv[3]) : 1);		// number of threads used
	unsigned totalTicks = ((argc > 4) ? atoi(argv[4]) : 10);	// number of total ticks

	// Register job info
	setNumEpcTicks(epochSize);
	setNumThreads(jobThreads);
	setNumTotalTicks(totalTicks);

	this->SetGraphBlcokScheduler((argc > 5 ? argv[5] : "static"));

	/*	setSchedulePolicy((argc > 6 ? argv[6] : "normal"));
	setProceedPolicy((argc > 7 ? argv[7] : "jacobi")); */

	this->getNumCPU();
	printf("There are %hu CPUs!\n", this->numCPU);

	this->setAffinitySeq((argc > 6 ? argv[6] : "none"));
	printf("Affinity Seq: (empty for none):");

	this->setEarlyTermination(argc > 7 ? argv[7] : "noet");

	this->setInnerIteration(argc > 8 ? atoi(argv[8]) : 1);

	this->setInnerScheduling(argc > 9 ? argv[9] : "simplesweep");
	
	this->setStopPolicy(argc > 10 ? argv[10] : "ticks");

	this->setNumGrabBlock(argc > 11 ? atoi(argv[11]) : 1);

	this->setBlockSweep(argc > 12 ? argv[12] : "normal");

	this->setMaintainInternalPriority(argc > 13 ? argv[13] : "noInternalPrior");

	for (size_t i = 0; i < this->affinitySeq.size(); i++)
		printf(" %u", affinitySeq[i]);
	printf(".\n");
}

/* -------- User must instantiate functions above -------- */
