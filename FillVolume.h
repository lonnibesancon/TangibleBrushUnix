#ifndef  FILLVOLUME_INC
#define  FILLVOLUME_INC

#define METRICS 10
#include "stdint.h"
#include "stdlib.h"
#include "pthread.h"
#include <cmath>

class FillVolume
{
	public:
		FillVolume(uint64_t x, uint64_t y, uint64_t z);
		~FillVolume();
		FillVolume* createUnion(const FillVolume& fv) const;
		FillVolume* createIntersection(const FillVolume& fv) const;

		bool get();
		void lock();
		void unlock();
	private:
		bool* m_fillVolume;
		uint64_t m_x, m_y, m_z;
		pthread_mutex_t m_mutex;
};

#endif
