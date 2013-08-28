/*
 *  main.cpp
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/18/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/GraphReader.h"
#include "GRACE/Graph.h"
#include "GRACE/Driver.h"
#include "GRACE/Profile.h"
#include "GRACE/GraphBlock.h"
#include "GRACE/Scheduler.h"

/* User Input Headers*/

#include "GRACE/Task.h"

/* Common STL Headers  */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include <sys/time.h>

using namespace std;

/* Cornell Utility Headers  */

#include <DBUtl/Log.h>


using namespace DBUtl;

#define LOG_SEVERITY ((Log::Severity)(99))

using namespace GRACE;

#define	SIZE_TO_PRINT	15

void printGraph(FILE *f, bool printEdges = true, int maxNumVertex = 20) {
	if (maxNumVertex == -1)
		maxNumVertex = Vertex::vtxList.size();

	fprintf(stdout, "# Vertex: %lu\n", Vertex::vtxList.size());

	for( unsigned i = 0; i < Vertex::vtxList.size() && i < maxNumVertex; i++) {
		VertexBase * vertex = &(Vertex::vtxList[i]);
		fprintf(f, "%u ", vertex->vertexID);
		vertex->show(f);
		if( printEdges ) {
			for( unsigned j = 0; j < vertex->outgoingEids.size(); j++ ) {
				VertexBase * neighbor = Edge::edgeList[vertex->outgoingEids[j]].dstVtx;
				fprintf(f, "\t-> [%d %d]\n",  neighbor->blockID, neighbor->vertexID);
			}
		}
	}
	fflush(f);
}


void clearGraph() {
	Vertex::vtxList.clear();
	Edge::edgeList.clear();
}


int main(int argc, char **argv) {
//	fprintf(stderr, "VertexSize = %lu\n", sizeof(Vertex));
//	fprintf(stderr, "EdgeSize = %lu\n", sizeof(Edge));
//	fprintf(stderr, "BlockSize = %lu\n", sizeof(GraphBlock));

	(void) Log::open("-", false);
//	Log::setLevel( LOG_SEVERITY );
	Log::setLevel( Log::TRACE );

	char buffer[256];

	bool exit = false;
	while (!exit) {
		// Read in and parse input parameters
		printf("Please input new command: ");
		cin.getline(buffer, 256);

		fprintf(stdout, "Command: %s\n", buffer);
		char cmdout[256];
		strcpy(cmdout, buffer);
		fflush(stdout);

		argc = 1;
		char * pch = strtok(buffer, " ");
		while (pch != NULL) {
			argv[argc] = pch;
			argc++;
			pch = strtok(NULL, " ");
		}

		// Usage help
		if ( argc <= 1 || strcmp(argv[1], "-h") == 0 ) {
			fprintf(stdout, "Usage: %s [[-l filename] | [-r params] | [-h] | [-e]] \n\n", argv[0]);
			fprintf(stdout, "   -l filename:  Load in the graph vertex (and edge) data from the specified files \n");
			fprintf(stdout, "   -o filename:  Write the graph vertex data snapshot to the specified file \n");
			fprintf(stdout, "   -v filename:  Refresh the graph vertex data from the specified files \n");
			fprintf(stdout, "   -s [numRuns] params: Simulate the scenario to run the applications with different application parameters.\n");
			fprintf(stdout, "   -r params:    Run the application with the specified job parameters \n");
			fprintf(stdout, "   -h:           Usage help\n");
			fprintf(stdout, "   -e:           Exit\n");
		}
		// Load graph vertex (and edge) data
		else if (strcmp(argv[1], "-l") == 0 || strcmp(argv[1], "-fastload") == 0) {
			const char * baseName = argv[2];

			timeval start, end;
			gettimeofday(&start, NULL);

			// Load in graph data
			fprintf(stdout, "Reading graph, base name %s ...", baseName);  fflush(stdout);
			bool fastLoad = strcmp(argv[1], "-fastload") == 0;
			(void) read(baseName, fastLoad);

			gettimeofday(&end, NULL);
			fprintf(stdout, "Graph Read Finished! Time elapsed %.2f seconds.\n",
					(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);
		}
		// Refresh graph vertex data
		else if ( strcmp(argv[1], "-v") == 0 ) {
			const char * baseName = argv[2];

			timeval start, end;
			gettimeofday(&start, NULL);

			// Refresh graph vertex data
			fprintf(stdout, "Refreshing vertex data, base name %s ...", baseName);  fflush(stdout);
			(void) refresh(baseName);
			gettimeofday(&end, NULL);
			fprintf(stdout, "Graph Refresh Finished! Time elapsed %.2f seconds.\n",
					(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);
		}
		// Run application as a new job
		else if (strcmp(argv[1], "-r") == 0) {
			//	Clean the old blocks
			//	TODO(wenleix): We should have a better mechanism.
			if (GraphBlock::blocks != NULL) {
				delete [] GraphBlock::blocks;
			}
			GraphBlock::blocks = NULL;
			GraphBlock::numBlocks = 0;

			//	The graph block initialization is now pushed down into Scheduler Initialization.
			//			GraphBlock::InitalizeGraphBlocks();

			Task * task = new Task();
			task->startTask(argc, argv);

			if ( task->checkTask() ) {
				registerTask(task);

				Graph::GenerateParamter(0);

				timeval start, end;
				gettimeofday(&start, NULL);
				run();
				gettimeofday(&end, NULL);

				fprintf(stdout, "Finished! Time elapsed %.2f seconds!!!\n",
						(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);
				fprintf(stdout, "#\t%s\n", cmdout);
				fprintf(stdout, "#\tNumIter\tRunTime\tNumVertexUpdate\t#NmEdgeVisited\n");
				fprintf(stdout, "#\t%d\t%.2f\t%lld\t%lld\n", Profile::numIter, Profile::total_time, 
															 Profile::procedCnt, Profile::edgeVisitCount);

			} else {
				fprintf(stdout, "Inappropriate Job Information\n");
			}

			delete task;
			//	We don't delete the blocks here since we need them when output the results.
			//	TODO(wenleix): A bit unsound...
			fprintf(stderr, "Task deleted!\n");
		} else if (strcmp(argv[1], "-s") == 0) {
			//	Simulate to run the application for many times.
			Task* task = new Task();
			task->startTask(argc - 1, argv + 1);

			if (task->checkTask()) {
				registerTask(task);
				timeval start, end;
				gettimeofday(&start, NULL);
				int numRuns = atoi(argv[2]);
				for (int i = 0; i < numRuns; i++) {
					fprintf(stdout, "%d out of %d runs.\n", i + 1, numRuns);
					Graph::GenerateParamter(i);
					run(true /* multiRun */, i == 0 /* firstInMultiRun */);
					task->blockScheduler->ResetForNextRun();
					GraphBlock::ResetForNextRun();
				}
				gettimeofday(&end, NULL);
				fprintf(stdout, "Workload simulation finished! Total time %.2f seconds! \n",
						(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);
				fprintf(stdout, "#\t%s\n", cmdout);
				fprintf(stdout, "#\tSumTime\n");
				fprintf(stdout, "#\t%.2f\n", Profile::accumulate_time);
			}

		}
		// Print the snapshot
		else if (strcmp(argv[1], "-o") == 0) {
			fprintf(stderr, "filename: %s\n", argv[2]);
			FILE* snapshot_file = fopen(argv[2], "w");
			printGraph(snapshot_file, false, -1);
			fclose(snapshot_file);
		}
		// Finish the exit
		else if (strcmp(argv[1], "-e") == 0)
		{
			clearGraph();
			exit = true;

			fprintf(stdout, "Bye Bye !\n\n");
		}
	}
}
