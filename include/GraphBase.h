/*
 *  GraphBase.h
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/16/12.
 *  Updated by Wenlei Xie on 5/9/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

#ifndef _GRACE_GRAPH_BASE_H_
#define _GRACE_GRAPH_BASE_H_

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/ThreadTools.h"

/* Cornell Utility Headers  */

#include <DBUtl/Log.h>


using namespace DBUtl;

/* Common STL Headers  */

#include <pthread.h>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <cstring>

namespace GRACE {
	class Job;
	class Task;

	class VertexBase;
	class EdgeBase;

	class GraphBlock;

	/*
	 *  Base Graph edge ...
	 */
	class EdgeBase {
        /* Graph Fields */
	public:
		//  TODO: Change it to unsigned
		int receiverBlockIndex;			// Index on the receiver block.

        //  TODO: Use uint32_t instead of the pointer? (Save space)
		VertexBase* srcVtx;		// Source vertex
		VertexBase* dstVtx;		// Destination vertex

	public:
		/* Edge initialization procedure */
		void init(uint16_t srcPID, uint32_t srcVID, uint16_t dstPID, uint32_t dstVID);

		/* Construct the edge list from file */
		virtual int construct(char *str) = 0;

		/* Edge destructor */
		virtual ~EdgeBase() {}
	};

	//  Base Graph vertex...
	class VertexBase {
	public:
		static void sortByID();
		static std::vector<int> uuid2VtxID;

		/* Graph Fields */
	public:
		uint32_t blockID;    	 // Block ID
		uint32_t vertexID;       // Vertex ID
		short	inBlockID;		 //	In BlockID

		int numInEdges;     // Number of incoming edges
		int numOutEdges;    // Number of outgoing edges

		bool isBoundaryVertex;

		std::vector <uint32_t> incomingEids;    // Incoming edge ids
		std::vector <uint32_t> outgoingEids;    // Outgoing edge ids

	public:
		// Add one incoming edge
		void addIncomingEid(uint32_t eID) { incomingEids.push_back(eID); }

		// Add one outgoing edge
		void addOutgoingEid(uint32_t eID) { outgoingEids.push_back(eID); }

		/* Base Functions */
	public:
		// Vertex destructor
		virtual ~VertexBase() { }

		// Vertex initialization
		void init(uint32_t blockID, uint32_t vID) {
			this->blockID = blockID;
			vertexID = vID; 
		}

		/* Communication Functions */

	public:
		virtual int construct(void* file) = 0;
		virtual void show(FILE * file) = 0;
		virtual void onInitialize(Job* const job) = 0;

	};
} /* namespace GRACE */


#endif /* _GRACE_GRAPH_BASE_H_ */
