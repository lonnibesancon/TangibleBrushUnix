#include "FillVolume.h"

Edge::Edge(const Vector2_f& a, const Vector2_f& b) : m_a(a), m_b(b), m_incr((b.x-a.x)/(b.y-a.y))
{
	if(b.y-a.y < 0.001 && b.y - a.y > -0.001)
		m_linear = true;
	else
		m_linear = false;

	if(a.y < b.y)
	{
		m_yMin   = a.y;
		m_yMax   = b.y;
	}

	else
	{
		m_yMin   = b.y;
		m_yMax   = a.y;
	}

	a.x < b.x ? m_startX = a.x : m_startX = b.x;
}

bool compareYEdge(const Edge& a, const Edge& b)
{
	return a.m_yMin < b.m_yMin;
}

bool compareXEdge(const Edge* a, const Edge* b)
{
	return a->m_startX < b->m_startX;
}

Rectangle3f::Rectangle3f(double a, double b, double c, double w, double h, double d) : x(a), y(b), z(c), width(w), height(h), depth(d)
{}

FillVolume::FillVolume(uint64_t x, uint64_t y, uint64_t z) : m_x(x*METRICS), m_y(y*METRICS), m_z(z*METRICS)
{
	pthread_mutex_init(&m_mutex, NULL);
	m_fillVolume = (uint8_t*)calloc((m_x*m_y*m_z+7)/8, sizeof(uint8_t));
	//for(uint32_t i=0; i < (m_x*m_y*m_z+7)/8; i++)
	//	m_fillVolume[i] = 0xff;
}

void FillVolume::init(const std::vector<Vector2_f>& p)
{
	m_selectionPoints = p;
	m_isInit          = true;
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
				uint8_t diff = fmin(fv.m_z - k, m_z - k);
				if(diff < 8)
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*k;
					uint8_t self          = *(m_fillVolume + (selfShift)/8);
					self               = (self >> (selfShift % 8)) & (0xff >> diff);

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*k;
					uint8_t its           = *(fv.m_fillVolume + (itsShift)/8);
					its                = (its >> (itsShift % 8)) & (0xff >> diff);

					result->m_fillVolume[selfShift] = (its | self);
					break;
				}
				else
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*k;
					uint8_t self          = *(m_fillVolume + (selfShift)/8);

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*k;
					uint8_t its           = *(fv.m_fillVolume + (itsShift)/8);

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
				uint8_t diff = fmin(fv.m_z - k, m_z - k);
				if(diff < 8)
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*k;
					uint8_t self          = *(m_fillVolume + (selfShift)/8);
					self               = (self >> (selfShift % 8)) & (0xff >> diff);

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*k;
					uint8_t its           = *(fv.m_fillVolume + (itsShift)/8);
					its                = (its >> (itsShift % 8)) & (0xff >> diff);

					result->m_fillVolume[selfShift] = (self & its);
					break;
				}
				else
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*k;
					uint8_t self          = *(m_fillVolume + (selfShift)/8);

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*k;
					uint8_t its           = *(fv.m_fillVolume + (itsShift)/8);

					result->m_fillVolume[selfShift] = (self & its);
				}
			}
		}
	}
	return result;
}

FillVolume* FillVolume::createExclusion(const FillVolume& fv) const
{
	FillVolume* result = new FillVolume(m_x, m_y, m_z);
	for(uint64_t i=0; i < fmin(m_x, fv.m_x); i++)
	{
		for(uint64_t j=0; j < fmin(m_y, fv.m_y); j++)
		{
			for(uint64_t k=0; k < fmin(m_z, fv.m_z); k+=8)
			{
				uint8_t diff = fmin(fv.m_z - k, m_z - k);
				if(diff < 8)
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*k;
					uint8_t self          = *(m_fillVolume + (selfShift)/8);
					self               = (self >> (selfShift % 8)) & (0xff >> diff);

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*k;
					uint8_t its           = *(fv.m_fillVolume + (itsShift)/8);
					its                = (its >> (itsShift % 8)) & (0xff >> diff);

					result->m_fillVolume[selfShift] = (self & (~its));
					break;
				}
				else
				{
					uint64_t selfShift = i + m_x*j + m_x*m_y*k;
					uint8_t self          = *(m_fillVolume + (selfShift)/8);

					uint64_t itsShift  = i + fv.m_x*j + fv.m_x*fv.m_y*k;
					uint8_t its           = *(fv.m_fillVolume + (itsShift)/8);

					result->m_fillVolume[selfShift] = (self & (~its));
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

void FillVolume::fillWithSurface(double depth, SelectionMode s, const Matrix4_f& matrix)
{
	printf("fill with surface \n");
	//Create the edge table
	std::vector<Edge> et;
	for(uint32_t i=0; i < m_selectionPoints.size()-1; i++)
		et.emplace_back(m_selectionPoints[i], m_selectionPoints[i+1]);
	et.emplace_back(m_selectionPoints[0], *(m_selectionPoints.rbegin()));

	std::sort(et.begin(), et.end(), compareYEdge);

	//Determine the yMin
	double yMin = m_selectionPoints[0].y;
	for(uint32_t i=0; i < m_selectionPoints.size(); i++)
		yMin = fmin(yMin, m_selectionPoints[i].y);

	//Now we need the Active Edge Table
	std::vector<const Edge*> aet;
	uint32_t etIndice=0;

	//For each scanline
	for(double j=yMin; etIndice < et.size(); j+=1.0/METRICS)
	{
		//Update the Active Edge Table
		//
		//Delete the useless edge

		for(uint32_t k=0; k < aet.size(); k++)
		{
			if(aet[k]->m_yMax >= j)
				break;
			aet.erase(aet.begin() + k);
		}

		//Add the new edge to handle
		while(etIndice < et.size() && et[etIndice].m_yMin < j)
		{
			aet.push_back(&(et[etIndice]));
			etIndice++;
		}

		//Sort the aet table to the x component
		std::vector<const Edge*> aetSorted = aet;
		if(aetSorted.size() == 1)
			continue;
		std::sort(aetSorted.begin(), aetSorted.end(), compareXEdge);

		if(aet.size() == 0)
			continue;

		uint8_t enable = true;
		uint32_t aetIndice = 0;

		//Go along the scanline, don't forget that the x = startX * (y - yMin) * incr
		for(double i=aetSorted[0]->computeX(j); aetIndice < aetSorted.size(); i+=1.0/METRICS)
		{
			//Make a step
			while(aetIndice < aetSorted.size() && !aetSorted[aetIndice]->m_linear && aetSorted[aetIndice]->computeX(j) < i)
			{
				enable = !enable;
				aetIndice++;
			}

			if(enable)
			{
				for(double k=0; k < depth; k+=.5/METRICS)
				{
					//Get the rect of the object at the position (i, j, k)
					Rectangle3f rect = computeRectangle(i, j, k, matrix);

					//Now fill the m_fillVolume
					for(double x=rect.x; x < rect.x+rect.width; x+=.5/METRICS)
					{
						if(x < 0 || x >= 640)
							continue;
						for(double y=rect.y; y < rect.y+rect.height; y+=.5/METRICS)
						{
							if(y < 0 || y >= 640)
								continue;
							for(double z=rect.z; z < rect.z+rect.depth; z+=.5/METRICS)
							{
								if(z < 0 || z >= 930)
									continue;

								int64_t selfShift = (int)(METRICS*x) + m_x*(int)(METRICS*y) + m_x*m_y*(int)(METRICS*z);
								if(selfShift >= m_x*m_y*m_z || selfShift < 0)
									continue;

								uint8_t self          = *(m_fillVolume + (selfShift)/8);
								m_fillVolume[selfShift/8] = self | (0x01 << (selfShift % 8));
								printf("x %f y %f z %f \n", x, y, z);
							}
						}
					}
				}
			}
		}
	}
}


Rectangle3f computeRectangle(double x, double y, double z, const Matrix4_f& matrix)
{
	//Take the object 3D rect from its default configuration
	Vector3_f v[8] = {
		Vector3_f(0.0, 0.0, 0.0), Vector3_f(0.0, 1.0/METRICS, 0.0), 
		Vector3_f(1.0/METRICS, 0.0, 0.0), Vector3_f(1.0/METRICS, 1.0/METRICS, 0.0), //Front face
		
		Vector3_f(0.0, 0.0, 0.0), Vector3_f(0.0, 1.0/METRICS, 0.0), 
		Vector3_f(1.0/METRICS, 0.0, 0.0), Vector3_f(1.0/METRICS, 1.0/METRICS, 0.0), //Front face
	};

	//Get the back face position
	for(uint32_t i=4; i < 8; i++)
		v[i] = v[i] + Vector3_f(0.0, 0.0, 1.0/METRICS);

	for(uint32_t i=0; i < 8; i++)
	{
		//Add the default position
		v[i] = v[i] + Vector3_f(x, y, z);
		//Then Calculate the transformation to these vec
		v[i] = matrix * v[i];
	}
	
	//Determine the maximum and minimum coord of the v[i] table
	float xMin, yMin, zMin, xMax, zMax, yMax;
	for(uint32_t i=0; i < 8; i++)
	{
		if(i==0)
		{
			xMin = xMax = v[i].x;
			yMin = yMax = v[i].y;
			zMin = zMax = v[i].z;
			continue;
		}

		if(v[i].x < xMin)
			xMin = v[i].x;
		else if(v[i].x > xMax)
			xMax = v[i].x;

		if(v[i].y < yMin)
			yMin = v[i].y;
		else if(v[i].y > yMax)
			yMax = v[i].y;

		if(v[i].z < zMin)
			zMin = v[i].z;
		else if(v[i].z > zMax)
			zMax = v[i].z;
	}

	return Rectangle3f(xMin, yMin, zMin, xMax - xMin, yMax - yMin, zMax - zMin);
}

bool FillVolume::get(uint64_t x, uint64_t y, uint64_t z)
{
	uint64_t selfShift = x + m_x*y + m_x*m_y*z;
	uint8_t self          = *(m_fillVolume + (selfShift)/8);
	self               = (self >> (selfShift % 8)) & 0x01;

	return self;
}
