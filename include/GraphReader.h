/*
*  GraphReader.h
*  GRACE
*
*  Created by Guozhang Wang on 1/16/12.
*  Updated by Wenlei Xie on 5/7/12.
*  Copyright 2012 Cornell. All rights reserved.
*/

#ifndef _GRACE_GRAPH_READER_H_
#define _GRACE_GRAPH_READER_H_

#ifndef _GRACE_CONFIG_H_
#	include "Config.h"
#endif

#include "GRACE/Graph.h"

/* Commmon STL Headers */

#include <cstdio>

/* DBUtl Headers */

#include <DBUtl/Log.h>
#include <DBUtl/pstdint.h>


namespace GRACE {

    /* Read in the graph partition data */
	void read(const char * baseName, bool fastload);

	/* Refresh graph vertex data*/
	void refresh(const char * baseName);

    /*
	* Base graph file ...
	*/
	struct GraphFile {
		FILE * f;
		int fileType;

		GraphFile(const char * baseName, const char * suffix)
		{
			f = 0;

			this->fileType = fileType;
			char fn[256];
			int n = snprintf(fn, (sizeof fn), "%s-%s.%s", baseName, suffix, "txt");

			DBUTL_ASSERT( n < (sizeof fn) );
			f = fopen(fn, "r");
			DBUTL_ASSERT( f != 0 );
		}

		~GraphFile()
		{
			if( f != 0 ) fclose(f);
		}
	};

	/*
	* Vertex file ...
	*/
	struct VtxFile : public GraphFile {

		int read(Vertex * vtx);

	    int refresh(std::vector<Vertex>* vertexList);

		VtxFile(const char * baseName) : GraphFile(baseName, "vertices") {}
		~VtxFile() {}
	};

	/* 
	* Edge file ... 
	*/
	struct EdgeFile : public GraphFile {
		int read(Edge * edge);

//		EdgeFile(const char * baseName) : GraphFile(baseName, "edges") {}

		static EdgeFile* constructEdgeFile(const char* baseName) {
			EdgeFile *ef = new EdgeFile(baseName, "edges");
			ef->compact = false;
			return ef;
		}

		static EdgeFile* constructCompactEdgeFile(const char* baseName) {
			//	c stands for compact.
			EdgeFile *ef = new EdgeFile(baseName, "cedges");
			ef->compact = true;
			return ef;
		}

		~EdgeFile() {}

	private:
		EdgeFile(const char* baseName, const char* suffix) : GraphFile(baseName, suffix) {}
		bool compact;
	};

} /* namespace GRACE */


#endif /* _GRACE_GRAPH_READER_H_ */
