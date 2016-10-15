#ifndef  FILLVOLUME_INC
#define  FILLVOLUME_INC

#define METRICS 10
#include "stdint.h"
#include "stdlib.h"
#include "pthread.h"

class FillVolume
{
	public:
		FillVolume(uint64_t x, uint64_t y, uint64_t z);
		~FillVolume();
		void createUnion(const FillVolume& fv) const;
		void createIntersection(const FillVolume& fv) const;

		void lock();
		void unlock();
	private:
		bool* m_fillVolume;
		uint64_t x, y, z;
		pthread_mutex_t m_mutex;
};

#endif
