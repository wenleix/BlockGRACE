/*
 *  Config.h
 *  GRACE
 *
 *  Created by Guozhang Wang on 1/17/12.
 *  Copyright 2012 Cornell. All rights reserved.
 *
 */
#ifndef _GRACE_CONFIG_H_
#define _GRACE_CONFIG_H_

/*
 * Get stdint.h consistently everywhere ...
 */
#ifndef __STDC_LIMIT_MACROS
#	define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#	define __STDC_CONSTANT_MACROS
#endif

#include <DBUtl/pstdint.h>

/*
 * GraphReader consistency checks ...
 */
#define _GRACE_GRAPH_READER_DEBUG
#undef	_GRACE_GRAPH_READER_DEBUG

/*
 * WorkList inlining and consistency checks ...
 */
#undef	_GRACE_WORKLIST_INLINE	
#define  _GRACE_WORKLIST_INLINE

#define	_GRACE_WORKLIST_DEBUG
#undef	_GRACE_WORKLIST_DEBUG

/*
 * Message tracing ...
 */
#define _GRACE_MSG_IO_DEBUG
#undef	_GRACE_MSG_IO_DEBUG

/*
 * Driver consistency checks ...
 */
#undef	_GRACE_DRIVER_DEBUG
#define _GRACE_DRIVER_DEBUG


#endif /* _GRACE_CONFIG_H_ */
