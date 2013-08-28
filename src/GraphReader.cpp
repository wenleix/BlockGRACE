/*
 *  GraphReader.cpp
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/16/12.
 *  Updated by Wenlei Xie on 5/9/12.
 *  Copyright 2012 Cornell. All rights reserved.
 */

/* GRACE Headers */

#include "GRACE/Config.h"
#include "GRACE/GraphReader.h"
#include "GRACE/Graph.h"

/* Cornell Utility Headers  */

#include <DBUtl/Log.h>
#include <DBUtl/pstdint.h>

/* Common STL Headers  */

#include <cstdio>

#include <sys/time.h>

using namespace GRACE;

/*
 * Generic graph reader.
 *
 * State initialization is done by calling `create' methods defined in Graph.h
 */

namespace GRACE {

    // read(const char *baseName, bool fastLoad)
    // Description: Read in the graph partition data
    // Returns: NO
    // Notes
    // baseName: the base name of the graph, the corresponding vertex file and edge file are baseName-vertices.txt and baseName-edges.txt, respectively.
    void read(const char * baseName, bool fastLoad) {

        int ret;

        // Read in the vertexes with values ...
        uint32_t vtxCount = 0;

		timeval start, end;
		gettimeofday(&start, NULL);

        VtxFile vf(baseName);

        while( true ) {
            Vertex vtx;

            ret = vf.read(&vtx);
            if (ret == 0) break;
            if (vtx.vertexID >= Vertex::vtxList.size()) {
            	Vertex::vtxList.resize(vtx.vertexID + 1);
            }

            Vertex::vtxList[vtx.vertexID] = vtx;
            vtxCount++;
        }

        DBUTL_ASSERT( ret == 0 );
	//	fprintf(stderr, "%d, %lu\n", vtxCount, Vertex::vtxList.size());
        DBUTL_ASSERT_ALWAYS(vtxCount == Vertex::vtxList.size());

//        VertexBase::sortByID();

		gettimeofday(&end, NULL);
		fprintf(stdout, "Vertex Read Finished! Time elapsed %.2f seconds.\n",
					(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);

        DBUTL_LOG_FPRINTF( Log::TRACE, ("Number of Vertexes: %u\n", vtxCount));

        // Read in the edges with values ...
        uint32_t edgeCount = 0;

		gettimeofday(&start, NULL);

        EdgeFile *ef = NULL;
        if (!fastLoad) {
        	ef = EdgeFile::constructEdgeFile(baseName);
        } else {
        	ef = EdgeFile::constructCompactEdgeFile(baseName);
        }

        while( true ) {
			Edge edge;
            ret = ef->read(&edge);
            if (ret == 0) break;

			Edge::edgeList.push_back(edge);
            // Add the edge info into its sender and receiver's list
            edge.dstVtx->addIncomingEid(edgeCount);
            edge.srcVtx->addOutgoingEid(edgeCount);

            edgeCount++;
        }

		gettimeofday(&end, NULL);
		fprintf(stdout, "Edge Read Finished! Time elapsed %.2f seconds.\n",
				(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);


        // Initialize the incoming and outgoing edge counts
        std::vector<Vertex>::iterator it;
        for (it = Vertex::vtxList.begin(); it != Vertex::vtxList.end(); it++) {
            (it)->numInEdges = (it)->incomingEids.size();
            (it)->numOutEdges = (it)->outgoingEids.size();
        }

        delete ef;
        DBUTL_ASSERT( ret == 0 );
    }


    // read(const char *baseName)
    // Description: Refresh the graph vertex data from the vertex file
    // Returns: NO
    // Notes
    // baseName: the base name of the graph, the corresponding vertex file is baseName-vertices.txt.
    void refresh( const char * baseName ) {
    	int ret;

    	// Read in the vertexes with values ...
    	uint32_t vtxCount = 0;

        VtxFile vf(baseName);

        while( true ) {
            ret = vf.refresh(&Vertex::vtxList);
            if (ret == 0) break;
            vtxCount++;
    	}

        DBUTL_ASSERT( ret == 0 );
        DBUTL_ASSERT_ALWAYS(vtxCount == Vertex::vtxList.size());
    }

    // VtxFile::read(Vertex * vtx)
    // Description: Read one vertex from the file, and store it into the pointer passed in.
    // Returns: 1 if read succeed. 0 if the vertex is not read.
    // Notes
    // vtx: The vertex pointer
    int VtxFile::read(Vertex * vtx) {

        DBUTL_ASSERT( f != 0 );

        uint32_t partID;
		uint32_t vertexID;
		int uuid;

        // Read in the vertex header: partitionID and vertexID
        int ret = fscanf(f, "%u %u %d", &partID, &vertexID, &uuid);
        if( ret == EOF ) return 0;

		if (uuid + 1 > VertexBase::uuid2VtxID.size()) {
			VertexBase::uuid2VtxID.resize(uuid + 1);
		}
		VertexBase::uuid2VtxID[uuid] = vertexID;
		

        // Initialize the vertex object and construct the attributes
        vtx->init(partID, vertexID);
        ret = vtx->construct(f);

        DBUTL_ASSERT( ret == 1 );

        return ret;
    }

    int VtxFile::refresh(std::vector<Vertex>* vertexList) {

        DBUTL_ASSERT( f != 0 );

        uint16_t partID;
        uint32_t vertexID;

        // Read in the vertex header: partitionID and vertexID
        int ret = fscanf(f, "%hu %u ", &partID, &vertexID);
        if( ret == EOF ) return 0;

        Vertex* vtx = &(vertexList->at(vertexID));

        // Initialize the vertex object and construct the attributes
        vtx->init(partID, vertexID);
        ret = vtx->construct(f);

        DBUTL_ASSERT( ret == 1 );

        return ret;
    }

    // EdgeFile::read(Edge * edge)
    // Description: Read one edge from the file, and store it into the pointer passed in.
    // Returns: 1 if read succeed. 0 if the edge is not read.
    // Notes
    // edge: The edge pointer
    int EdgeFile::read(Edge * edge) {

        DBUTL_ASSERT( f != 0 );
		char buf[1000];
		if (fgets(buf, 1000, f) == NULL)
			return 0;

        uint16_t srcPartID;
        uint32_t srcVertexID;
        uint16_t dstPartID;
        uint32_t dstVertexID;

        char *pch = strtok(buf, " ");
        if (!compact) {
			srcPartID = atoi(pch);
			pch = strtok(NULL, " ");
			srcVertexID = atoi(pch);
			pch = strtok(NULL, " ");
			dstPartID = atoi(pch);
			pch = strtok(NULL, " ");
			dstVertexID = atoi(pch);
			pch = strtok(NULL, " ");
        } else {
			srcVertexID = atoi(pch);
			pch = strtok(NULL, " ");
			dstVertexID = atoi(pch);
			pch = strtok(NULL, " ");
        }

        // Initialize the edge object and construct the attributes
        edge->init(srcPartID, srcVertexID, dstPartID, dstVertexID);
        int ret = edge->construct(pch);

        DBUTL_ASSERT( ret == 1 );

        return ret;
    }

} /* namespace GRACE */


