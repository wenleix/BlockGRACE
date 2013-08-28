/*
 *  GraphBlock.h
 *  GRACE
 *
 *  Created by Wenlei Xie on 09/25/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */

#ifndef _GRAPH_BLOCK_H_
#define _GRAPH_BLOCK_H_

//	GRACE Head
#include "GRACE/Graph.h"

//	Cornell Utility Headers
#include <DBUtl/Log.h>

#include <algorithm>


namespace GRACE {
	//	The graph block interface exposed to user.
	class GraphBlock {
	public:
		//	Used by BlockScheduler.
		static void InitalizeGraphBlocks() {
			fprintf(stderr, "Initialize Blocks.\n");

/*			if (randomBlockID) {
				fprintf(stderr, "Randomize Block ID is deprecated!\n");
				DBUTL_ASSERT_ALWAYS(false);

				if (seed == -1) {
					srand(time(NULL));
				} else {
					srand(9261986 * (seed + 5));
				}

				int numBlock = 0;
				for (int i = 0; i < Vertex::vtxList.size(); i++) {
					int bid = Vertex::vtxList[i].blockID;
					numBlock = std::max(numBlock, bid + 1);
				}
				fprintf(stderr, "Num Blocks: %d\n", numBlock);
				int * mapping = new int[numBlock];
				for (int i = 0; i < numBlock; i++)
					mapping[i] = i;
				std::random_shuffle(mapping, mapping + numBlock);
				for (int i = 0; i < Vertex::vtxList.size(); i++) {
					int bid = Vertex::vtxList[i].blockID;
					Vertex::vtxList[i].blockID = mapping[bid];
				}
				delete [] mapping;  
			} */

			std::vector< std::vector<int> > tempBlocks;
			for (int i = 0; i < Vertex::vtxList.size(); i++) {
				Vertex* vertex = &Vertex::vtxList[i];
				if (vertex->blockID >= tempBlocks.size()) {
					tempBlocks.resize(vertex->blockID + 1);
				}
				tempBlocks[vertex->blockID].push_back((int)(vertex->vertexID));
			}

			//	Prevent Memory Leak.
			DBUTL_ASSERT_ALWAYS(GraphBlock::numBlocks == 0);
			DBUTL_ASSERT_ALWAYS(GraphBlock::blocks == NULL);
			GraphBlock::numBlocks = tempBlocks.size();
			GraphBlock::blocks = new GraphBlock[tempBlocks.size()];
			for (int i = 0; i < GraphBlock::numBlocks; i++)
				GraphBlock::blocks[i].Initialize(&tempBlocks[i][0], tempBlocks[i].size(), i);

			fprintf(stderr, "Start setup blocks.\n");
			GraphBlock::SetupBlocks();
			fprintf(stderr, "Finish Initialize blocks.\n");
		}

		//	Used by VertexScheduler.
		static void InitializeVerticesAsBlocks() {
/*			if (randomBlockID) {
				fprintf(stderr, "Randomize Block ID is deprecated!!!\n");
				DBUTL_ASSERT_ALWAYS(false);

				if (seed == -1) {
					srand(time(NULL));
				} else {
					srand(9261986 * (seed + 5));
				}

				int numBlock = 0;
				for (int i = 0; i < Vertex::vtxList.size(); i++) {
					int bid = Vertex::vtxList[i].blockID;
					numBlock = std::max(numBlock, bid + 1);
				}
				if (numBlock == 1) {
					//	Simple Case, randomize block ID
					fprintf(stderr, "Pure vertex scheduling\n");	
					int * bid = new int[Vertex::vtxList.size()];
					for (int i = 0; i < Vertex::vtxList.size(); i++) {
						bid[i] = i;
					}

					std::random_shuffle(bid, bid + Vertex::vtxList.size());
					for (int i = 0; i < Vertex::vtxList.size(); i++) {
						Vertex::vtxList[i].blockID = bid[i];
					}
					delete [] bid;
				} else {
					//	Cache aware vertex scheduling, more compliated
					fprintf(stderr, "Detect cache aware vertex scheduling, numBlock: %d!\n", numBlock);
					int * blockMapping = new int[numBlock];
					int * blockSize = new int[numBlock];
					for (int i = 0; i < numBlock; i++) {
						blockMapping[i] = i;
						blockSize[i] = 0;
					}
					std::random_shuffle(blockMapping, blockMapping + numBlock);
					for (int i = 0; i < Vertex::vtxList.size(); i++) {
						int bid = blockMapping[Vertex::vtxList[i].blockID];
						blockSize[bid]++;
					}

					int * nextID = new int[numBlock];
					nextID[0] = 0;
					for (int i = 1; i < numBlock; i++)
						nextID[i] = nextID[i - 1] + blockSize[i - 1];
				
					//	Relabel
					for (int i = 0; i < Vertex::vtxList.size(); i++) {
						int bid = blockMapping[Vertex::vtxList[i].blockID];
						Vertex::vtxList[i].blockID = nextID[bid];
						nextID[bid]++;
					}

					delete [] blockMapping;
					delete [] blockSize;
					delete [] nextID;
				} 
			} */

			//	Make each single vertex as a block.
			for (int i = 0; i < Vertex::vtxList.size(); i++)
				Vertex::vtxList[i].blockID = i;

			InitalizeGraphBlocks();
		}

		//	Reset the last commtted slot.
		static void ResetForNextRun() {
			for (int i = 0; i < GraphBlock::numBlocks; i++)
				GraphBlock::blocks[i].lastCommittedSlot = 0;
		}

		//	Set up the incommingBlockIDs, the recvBlockIndex on the edge, etc.
		static void SetupBlocks();

		//	Take a snapshot of the graph before calling the block update function.
		void PrepareForUpdate() {
			//	Lazy replication
			for (int i = 0; i < this->size; i++) {
				Vertex* vertex = &Vertex::vtxList[this->vertexID[i]];

				// TODO(guozhang): so users need to overwrite the =operator for complicated vertex data right?
				vertex->data[this->lastCommittedSlot ^ 1] = vertex->data[this->lastCommittedSlot];

				//	Edge data is not allowed.
				//	Replicate the outgoing edge data.
/*				for (std::vector<uint32_t>::iterator it = vertex->outgoingEids.begin();
						it != vertex->outgoingEids.end(); it++)
					Edge::edgeList[*it].data[this->lastCommittedSlot ^ 1] = Edge::edgeList[*it].data[this->lastCommittedSlot]; */
			}
//			fprintf(stderr, "Lazy replication done\n");

			//	Take snapshot
			//	TODO(wenleix): Is this enough for block? Or we should lock per incoming block?
			//  TODO(guozhang): I think this is OK that we use finer-grained locks per block, although we would not get a "consistent" snapshot
			LockBarrier();
			this->pushSlot ^= 1;	//	Flip the push slot.
			for (int i = 0; i < this->numIncomingBlocks; i++) {
				int blockID = this->incomingBlockID[i];
	//			GraphBlock::blocks[blockID].LockBarrier();
				this->incomingBlockSlot[i] = GraphBlock::blocks[blockID].lastCommittedSlot;
	//			GraphBlock::blocks[blockID].UnlockBarrier();
			}
			UnlockBarrier();
//			fprintf(stderr, "snapshot done!\n");
		}

		void Commit() {
			LockBarrier();
			lastCommittedSlot ^= 1; 
			UnlockBarrier();
		}

		int GetBlockSize() { return size; }
		int GetVertexID(int subID) {
			return vertexID[subID];
		}

		bool* GetReadVertexScheduling(Job *job);
		bool* GetWriteVertexScheduling(Job *job);

		bool IsLocalVertex(Vertex* vertex) {
			if (vertexIsConsecutive)
				return vertexID[0] <= vertex->vertexID && vertex->vertexID < vertexID[0] + size;

			DBUTL_ASSERT_ALWAYS(false);

			//	TODO(guozhang): may not work if it is not sorted?
			int* vertexIdIter = std::lower_bound(vertexID, vertexID + size, (int)(vertex->vertexID));
			if (vertexIdIter == vertexID + size || *vertexIdIter != vertex->vertexID) {
				//	Ghost node.
				return false;
			} else {
				return true;
			}
		}

		const VertexData* GetGhostLastCommittedVertexData(Edge* incomingEdge) {
			int blockIndex = incomingEdge->receiverBlockIndex;
			DBUTL_ASSERT(blockIndex != -1);
			int slot = incomingBlockSlot[blockIndex];
			const VertexData* data = &((Vertex*)(incomingEdge->srcVtx))->data[slot].d;

			// DEBUG
			/*
			fprintf(stderr, "srcVertexid = %u, recvBlockIndex = %d, slot = %d, rank = %.2f\n",
					incomingEdge->srcVtx->vertexID, blockIndex, slot, data->rank);  */

			return data;
		}

		const VertexData* GetLocalVertexLastCommittedData(Vertex* vertex) {
			DBUTL_ASSERT(IsLocalVertex(vertex));
			return &vertex->data[lastCommittedSlot].d;
		}

		//	Should only be called if it is a local vertex.
		VertexData* GetLocalVertexWriteData(Vertex* vertex) {
			DBUTL_ASSERT(IsLocalVertex(vertex));
			return &vertex->data[lastCommittedSlot ^ 1].d;
		}

/*		const EdgeData* GetGhostLastCommittedEdgeData(Edge* incomingEdge) {
			int blockIndex = incomingEdge->receiverBlockIndex;
			DBUTL_ASSERT(blockIndex != -1);
			int slot = incomingBlockSlot[blockIndex];
			return &incomingEdge->data[slot];
		}

		EdgeData* GetLocalEdgeWriteData(Edge* edge) {
			DBUTL_ASSERT(IsLocalVertex((Vertex*)(edge->srcVtx)));
			return &edge->data[lastCommittedSlot ^ 1];
		} */

		//	Wrapper Function
		const VertexData* GetVertexData(Edge* incomingEdge) {
			Vertex* srcVertex = (Vertex*)(incomingEdge->srcVtx);
			if (incomingEdge->receiverBlockIndex == -1) {
				//	For local vertex, we get the writable data.
				return GetLocalVertexWriteData(srcVertex);
			} else {
				//	Ghost vertex, get the read-only data.
				return GetGhostLastCommittedVertexData(incomingEdge);
			}
			DBUTL_ASSERT_ALWAYS(false);
			return NULL;
		}

		//	Wrapper Function
/*		const EdgeData* GetEdgeData(Edge* edge) {
			if (edge->receiverBlockIndex == -1) {
				//	Local vertex, get the writable data.
				return GetLocalEdgeWriteData(edge);
			} else {
				//	Ghost vertex, get the read-only data.
				return GetGhostLastCommittedEdgeData(edge);
			}
			DBUTL_ASSERT_ALWAYS(false);
			return NULL;
		} */

		bool VertexIsConsecutive() {
			return vertexIsConsecutive;
		}

		~GraphBlock() {
			delete [] vertexID;
			delete [] incomingBlockID;
			delete [] incomingBlockSlot;
			delete [] vertexPriority;
#ifdef MEMORY_BARRIER
			pthread_mutex_destroy(&mutexBarrier);
#endif
		}

		static GraphBlock * blocks;
		static int numBlocks;

		int blockID;				// Block ID

		// TODO(guozhang): better make it a boolean?
		short lastCommittedSlot;		// Slot ID for the last committed data, would be either 1 or 0
		short pushSlot;					//  Slot ID for other blocks to push into the data;


		bool convergent;		//	Is it convergent given the boundary conditions?

		bool firstTimeUpdate;	//	Is this block the first time getting updated during the computation?
								//	Used in Dynamic Inner Schedulng

		int size;
		//	TODO(wenleix): Now the vertices in a block are always consecutive, don't need to keep
		//	the array of vertices.
		int * vertexID;

		float *vertexPriority;

	private:
		GraphBlock()
			: lastCommittedSlot(0), pushSlot(0), vertexPriority(NULL), firstTimeUpdate(true) {
#ifdef MEMORY_BARRIER
			pthread_mutex_init(&mutexBarrier, NULL);
#endif
		}

		void LockBarrier() {
#ifdef MEMORY_BARRIER
			pthread_mutex_lock(&mutexBarrier);
#endif
		}

		void UnlockBarrier() {
#ifdef MEMORY_BARRIER
			pthread_mutex_unlock(&mutexBarrier);
#endif
		}

		//	Construct the block with the vertex IDs.
		void Initialize(int* vertexID, int size, int BlockID);

		int * incomingBlockID;
		int * incomingBlockSlot;
		int numIncomingBlocks;

		bool vertexIsConsecutive;



#ifdef MEMORY_BARRIER
		pthread_mutex_t	mutexBarrier;
#endif
	};
}


#endif /* _GRAPH_BLOCK_H_ */
