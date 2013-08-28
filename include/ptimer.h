#ifndef _DBUTL_PTIMER_H_
#define _DBUTL_PTIMER_H_

#include <sys/resource.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

namespace DBUtl {

class PTimer {

	private:
		struct timeval tim;
	#   ifdef __APPLE__
		struct rusage ru;
	#   else
		struct timespec ts;
	#   endif
		
		double real_start, real_end;
		double user_start, user_end;
		double system_start, system_end;
	
	public:
		PTimer() {}
		~PTimer() {}
		void start();
		void finish();
		double realResult();
		double userResult();
		double systemResult();
	
	};

#	ifdef DBUTL_INLINE
#		define INLINE inline
#		include "ptimer.hpp"
#	endif

} // namespace DBUtl

#endif /* _DBUTL_PTIMER_H_ */
