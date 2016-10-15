#include "FillVolume.h"

FillVolume::FillVolume(uint64_t x, uint64_t y, uint64_t z) : m_x(x), m_y(y), m_z(z)
{
	pthread_mutex_init(&m_mutex, NULL);
	m_fillVolume = (bool*)calloc((x*METRICS*y*METRICS*z*METRICS+7)*sizeof(bool)/8);
}

FillVolume::~FillVolume()
{
	free(m_fillVolume);
	unlock();
}

FillVolume* FillVolume::createUnion(const FillVolume& fv) const
{
	FillVolume* result = new FillVolume(m_x, m_y, m_z);
	for(uint64_t i=0; i < fmin(m_x, fv.m_x); i++)
	{
		for(uint64_t j=0; j < fmin(m_y, fv.m_y); j++)
		{
			for(uint64_t k=0; k < fmin(m_z, fv.m_z); k+=8)
			{
				uint8_t diff = min(fv.m_z - k, m_z - k);
				if(diff < 8)
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*z;
					bool self          = (m_fillVolume + selfShift)/8;
					self               = (self >> (selfShift % 8)) & (0xff >> diff);

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*z;
					bool its           = (fv.m_fillVolume + itsShift)/8;
					its                = (its >> (itsShift % 8)) & (0xff >> diff);

					result->m_fillVolume[selfShift] = (its | self);
					break;
				}
				else
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*z;
					bool self          = (m_fillVolume + selfShift)/8;

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*z;
					bool its           = (fv.m_fillVolume + itsShift)/8;

					result->m_fillVolume[selfShift] = (its | self);
				}
			}
		}
	}
	return result;
}

FillVolume* FillVolume::createIntersection(const FillVolume& fv) const
{
	FillVolume* result = new FillVolume(m_x, m_y, m_z);
	for(uint64_t i=0; i < fmin(m_x, fv.m_x); i++)
	{
		for(uint64_t j=0; j < fmin(m_y, fv.m_y); j++)
		{
			for(uint64_t k=0; k < fmin(m_z, fv.m_z); k+=8)
			{
				uint8_t diff = min(fv.m_z - k, m_z - k);
				if(diff < 8)
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*z;
					bool self          = (m_fillVolume + selfShift)/8;
					self               = (self >> (selfShift % 8)) & (0xff >> diff);

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*z;
					bool its           = (fv.m_fillVolume + itsShift)/8;
					its                = (its >> (itsShift % 8)) & (0xff >> diff);

					result->m_fillVolume[selfShift] = (self & its);
					break;
				}
				else
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*z;
					bool self          = (m_fillVolume + selfShift)/8;

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*z;
					bool its           = (fv.m_fillVolume + itsShift)/8;

					result->m_fillVolume[selfShift] = (self & its);
				}
			}
		}
	}
	return result;
}

void FillVolume::lock()
{
	pthread_mutex_lock(&m_mutex);
}

void FillVolume::unlock()
{
	pthread_mutex_unlock(&m_mutex);
}
