/*
 *  TaskBase.h
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/16/12.
 *  Updated by Wenlei Xie on 5/9/12.
 *  Copyright 2012 Cornell. All rights reserved.
 */

#ifndef _GRACE_TASK_BASE_H_
#define _GRACE_TASK_BASE_H_

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/GraphBase.h"

/* Cornell Utility Headers  */

#include <DBUtl/Log.h>

/* Common STL Headers  */

#include <cstdio>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

using namespace DBUtl;

namespace GRACE {

	//	A simple struct to hold inner scheduling data, such as scheduling bit.
	//	TODO(wenleix): The implementation- is very hacking at this moment...
	struct InnerScheduling {
		bool **schedulingBit[2];
		bool **boundaryVertex;	//	Has has incoming edges from other blocks.
		int numBlocks;

		InnerScheduling() : numBlocks(0) {
			schedulingBit[0] = NULL;
			schedulingBit[1] = NULL;
			boundaryVertex = NULL;
		}
		~InnerScheduling() {
			for (int slot = 0; slot < 2; slot++) {
				if (schedulingBit[slot] != NULL) {
					for (int i = 0; i < numBlocks; i++) {
						if (schedulingBit[slot][i] != NULL)
							delete [] schedulingBit[slot][i];
					}
					delete [] schedulingBit[slot];
				}
			}

			if (boundaryVertex != NULL) {
				for (int i = 0; i < numBlocks; i++) {
					if (boundaryVertex[i] != NULL)
						delete [] boundaryVertex[i];
				}
				delete [] boundaryVertex;
			}
		}
	};

	class AbstractBlockScheduler;

	/*
	* Base Task Descriptor
	*/
	class TaskBase {

		/* Config Fields */
	public:
		uint32_t numEpcTicks;			// Number of ticks an epoch contains
		uint32_t numThreads;			// Number of threads used for the job

		enum stopPolicy {			// Job stopping policy 
			Ticks,					//	1.	Stop after a fixed number of ticks
			Convergence			    //  2.  Stop after convergence.
		} stop;

		enum prepareOpt {
			NoCmd,
			Schedule
		} opt;

		uint32_t numTotalTicks;				// Used by the 1st stop policy: number of total ticks to execute

		uint16_t numCPU;					//  Num of CPU
		std::vector<uint32_t> affinitySeq;	//  The sequence to affinity CPU

		bool earlyTermination;
		int numInnerIteration;
		bool alternatingSweep;				//	Alternating sweep inside block.

		//	Several dynamic inner schedling options.
		//	At most one of them would be true. If this is the case, would ignore numInnerIteration and alternatingSweep.
		//	TODO(wenleix): Make inner scheduling as a enum.
		bool dynamicSweep;
		bool innerFIFO;

		bool randomPick;					//	Enable random pick for scheduler.


		bool avoidInternalPriority;			//	Optimization for avoiding internal vertex priority maintainance.

		InnerScheduling inSchedule;

		/* Driver Fields*/
	public:
		uint32_t	curEpoch;						// Current epoch

		AbstractBlockScheduler* blockScheduler;

		std::vector<pthread_t> workers;				// Worker thread Id

		std::vector<Job *> jobs;					// Job specifics for each worker
		int vertexUpdateCounter;					// Monitor the number of vertex update

		/* Functions */
	public:
		/* Constructor */
		TaskBase() : numEpcTicks(1), numThreads(1), numTotalTicks(0), curEpoch(0), 
					 randomPick(false), dynamicSweep(false), innerFIFO(false),
					 avoidInternalPriority(true) {
			// Initialize task info
			stop = Ticks;
			opt = NoCmd;
		}

		virtual ~TaskBase();

		/* Check if the task can stop */
		bool Stop() {
			return false;
		}

		void SetGraphBlcokScheduler(const char* blockSchedulerType);

		// Set number of epoch ticks
		void setNumEpcTicks(uint32_t num) { numEpcTicks = num; }

		// Set number of threads
		void setNumThreads(uint32_t num) { numThreads = num; }

		// Set number of total ticks to execute
		void setNumTotalTicks(uint32_t num) { numTotalTicks = num; }

		// Set number of blocks to grab
		void setNumGrabBlock(int num);

		/* Set the job stop policy */
		void setStopPolicy(const char * policy) {
			if (strcmp(policy, "ticks") == 0) {
				stop = Ticks;
			} else if (strcmp(policy, "convergence") == 0) {
				stop = Convergence;
			} else {
				DBUTL_ASSERT_ALWAYS( false );
			}
		}

        // Get the number of CPU
        void getNumCPU() {
            numCPU = sysconf(_SC_NPROCESSORS_ONLN);
        }

        /*  Set the CPU affinity Sequence, call it after getNumCPU()! */
        void setAffinitySeq(const char *type) {
            if (strcmp(type, "none") == 0) affinitySeq = std::vector<uint32_t>();
            else if (strcmp(type, "linear") == 0) {
                affinitySeq = std::vector<uint32_t>();
                for (uint16_t i = 0; i < numCPU; i++)
                	affinitySeq.push_back(i);
            }
            else if (strlen(type) == 5 && strncmp(type, "step", 4) == 0) {
				//	Make affinity seq like 0, 2, 4, 6, 1, 3, 5, 7
                affinitySeq = std::vector<uint32_t>();
            	int step = type[4] - '0';
            	for (int i = 0; i < step; i++)
            		for (int j = i; j < numCPU; j += step)
            			affinitySeq.push_back(j);
            }
            else if (strlen(type) == 6 && strncmp(type, "pstep", 4) == 0) {
				//	Make affinity seq like 0, 2, 4, 6..
                affinitySeq = std::vector<uint32_t>();
            	int step = type[5] - '0';
				for (int i = 0; i < numCPU; i++)
					affinitySeq.push_back(step * i);
            }
			else if (strncmp(type, "arb", 3) == 0) {
				for (int i = 3; i < strlen(type); i++)
					affinitySeq.push_back(type[i] - '0');
			}
            else {
            	fprintf(stderr, "Unsupported Affinity Seq %s!", type);
            	DBUTL_ASSERT_ALWAYS(false);
            }
        }

		void setEarlyTermination(const char* et) {
			if (strcmp(et, "noet") == 0) {
				this->earlyTermination = false;
			} else if (strcmp(et, "et") == 0) {
				this->earlyTermination = true;
			} else {
				fprintf(stderr, "Unsupported Early Termination Type: %s!\n", et);
				DBUTL_ASSERT_ALWAYS(false);
			}
		}

		void setInnerIteration(int numInner) {
			this->numInnerIteration = numInner;
		}

		void setInnerScheduling(const char* inScheduling) {
			if (strcmp(inScheduling, "simplesweep") == 0) {
				this->alternatingSweep = false;
			} else if (strcmp(inScheduling, "altersweep") == 0) {
				this->alternatingSweep = true;
			} else if (strcmp(inScheduling, "dynamicsweep") == 0) {
				this->dynamicSweep = true;
			} else if (strcmp(inScheduling, "fifo") == 0) {
				DBUTL_ASSERT_ALWAYS(false);
				this->innerFIFO = true;
			}
			else {
				fprintf(stderr, "Unsupported Inner Scheduling Type: %s!\n", inScheduling);
				DBUTL_ASSERT_ALWAYS(false);
			}
		}
		
		void setBlockSweep(const char* sweep);

		void setMaintainInternalPriority(const char* interalPriority) {
			fprintf(stderr, "!!!internalPr option: %s\n", interalPriority);

			if (strcmp(interalPriority, "noInternalPrior") == 0) {
				this->avoidInternalPriority = true;
			} else if (strcmp(interalPriority, "InternalPrior") == 0) {
				this->avoidInternalPriority = false;
				fprintf(stderr, "Hey Experimental Option: Internal Prior is enabled!!\n");
			} else {
				fprintf(stderr, "Invalid Internal Priority parameter: %s!\n", interalPriority);
				DBUTL_ASSERT_ALWAYS(false);
			}
		}
	
		//	Schedule vertices that satisfy the predicate for the current tick
		//	Only needed by dynamic scheduler. (Eager & Prior)
		void scheduleVertices();

	public:
		// Check Task Information
		bool checkTask();

	public:
		virtual void startTask(int argc, char **argv) {};
	};

} /* namespace GRACE */


#endif /* _GRACE_TASK_BASE_H_ */
