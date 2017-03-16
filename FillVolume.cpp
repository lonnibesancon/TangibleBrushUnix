#include "FillVolume.h"
#include "math.h"
#include "sys/stat.h"
#include <cstdlib>
#include "rendering/ParticuleObject.h"
#include <SDL2/SDL.h>

Edge::Edge(const Vector2_f& a, const Vector2_f& b) : m_a(a), m_b(b)
{
	if(b.y-a.y < 0.001 && b.y - a.y > -0.001)
	{
		m_linear = true;
		m_incr = 0;
	}
	else
	{
		m_incr = ((b.x-a.x)/(b.y-a.y));
		m_linear = false;
	}

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

	(a.x < b.x) ? m_startX = a.x : m_startX = b.x;
	(a.x > b.x) ? m_endX = a.x : m_endX = b.x;
}

bool compareYEdge(const Edge& a, const Edge& b)
{
	if(a.m_yMin < b.m_yMin)
		return true;
	return false;
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
	m_saveVolume = (uint8_t*)calloc((m_x*m_y*m_z+7)/8, sizeof(uint8_t));

//	for(uint32_t i=0; i < (m_x*m_y*m_z+7)/8; i++)
//		m_fillVolume[i] = 0xff;
}


FillVolume::FillVolume(const std::string& path)
{
	FILE* f = fopen(path.c_str(), "r");
	float buf;
	fread(&buf, sizeof(float), 1, f);
	fread(&m_x, sizeof(uint64_t), 1, f);
	fread(&m_y, sizeof(uint64_t), 1, f);
	fread(&m_z, sizeof(uint64_t), 1, f);

	m_fillVolume = (uint8_t*)malloc((m_x*m_y*m_z+7)/8);
	m_saveVolume = (uint8_t*)malloc((m_x*m_y*m_z+7)/8);

	fread(m_fillVolume, sizeof(uint8_t), (m_x*m_y*m_z+7)/8, f);
	memcpy(m_saveVolume, m_fillVolume, (m_x*m_y*m_z+7)/8);

	fclose(f);
}

void FillVolume::clear()
{
	memset(m_fillVolume, 0x00, (m_x*m_y*m_z+7)/8);
	memset(m_saveVolume, 0x00, (m_x*m_y*m_z+7)/8);
}

void FillVolume::init(const std::vector<Vector2_f>& p)
{
	m_scanline.clear();
	m_isInit = false;
	m_selectionPoints.clear();
	if(p.size() > 0)
	{
		m_selectionPoints = p;
		m_isInit          = true;

		//Create the edge table
		std::vector<Edge> et;
		for(int i=0; i < (int)(m_selectionPoints.size()-1); i++)
			et.push_back(Edge(m_selectionPoints[i], m_selectionPoints[i+1]));
		et.push_back(Edge(m_selectionPoints[m_selectionPoints.size()-1], m_selectionPoints[0]));

		std::sort(et.begin(), et.end(), compareYEdge);

		//Determine the yMin
		double yMin = m_selectionPoints[0].y;
		for(uint32_t i=0; i < m_selectionPoints.size(); i++)
			yMin = fmin(yMin, m_selectionPoints[i].y);

		//Now we need the Active Edge Table
		std::vector<const Edge*> aet;
		uint32_t etIndice=0;

		//For each scanline
		for(double j=yMin; et.rbegin()->m_yMax >= j; j+=.010)
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
			while(etIndice < et.size() && et[etIndice].m_yMin <= j)
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
			for(double i=aetSorted[0]->computeX(j); aetIndice < aetSorted.size() && i <= (*aetSorted.rbegin())->computeX(j); i+=0.015)
			{
				//Make a step
				while(aetIndice+1 < aetSorted.size() && aetSorted[aetIndice+1]->computeX(j) < i)
				{
					if(i < aetSorted[aetIndice]->m_endX)
						enable = !enable;
					aetIndice++;
				}

				if(enable)
				{
					m_scanline.push_back(Vector2(i, j));
				}
			}
		}
	}
}

FillVolume::~FillVolume()
{
	free(m_fillVolume);
	free(m_saveVolume);
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
	for(uint64_t k=0; k < fmin(m_z, fv.m_z); k+=8)
	{
		for(uint64_t j=0; j < fmin(m_y, fv.m_y); j++)
		{
			for(uint64_t i=0; i < fmin(m_x, fv.m_x); i++)
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

void FillVolume::fillWithSurface(double depth, const Matrix4_f& matrix)
{
	if(!m_isInit || m_selectionPoints.size() < 2)
		return;

	for(Vector2_f& point : m_scanline)
	{
		Vector3_f pos = matrix*Vector3(point.x, point.y, -1.0f);
		pos =  pos*METRICS;
		Vector3_f pos2 = matrix*Vector3(point.x+0.015, point.y+0.010, -1.0f);
		pos2 = pos2*METRICS;

		float maxZ = std::max(pos.z, pos2.z)+depth;
		float maxY = std::max(pos.y, pos2.y);
		float maxX = std::max(pos.x, pos2.x);

		float startZ = std::max(0.0f, std::min(pos.z, pos2.z));
		float startY = std::max(0.0f, std::min(pos.y, pos2.y));
		float startX = std::max(0.0f, std::min(pos.x, pos2.x));


		for(int32_t z=startZ; z <= maxZ; z+=1)
		{
			if(z > m_z)
				break;
			for(int32_t y=startY; y <= maxY; y+=1)
			{
				if(y > m_y)
					break;
				for(int32_t x=startX; x <= maxX; x+=1)
				{
					if(x > m_x)
						break;

					int64_t selfShift = x + m_x*y + m_x*m_y*z;
					uint8_t self      = *(m_fillVolume + (selfShift)/8);
					switch(m_selectionMode)
					{
						case UNION:
							self = self | (0x01 << (selfShift % 8));
							break;
						case INTERSECT:
							self = self | (0x01 << (selfShift % 8));
							self = self & *(m_saveVolume + selfShift/8);
							break;
						case EXCLUSION:
							self = self & (~(0x01 << (selfShift % 8)) & *(m_saveVolume + selfShift/8));
							break;
						default:
							break;
					}
					m_fillVolume[selfShift/8] = self;
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
		Vector3_f(1.0/METRICS, 0.0, 0.0), Vector3_f(1.0/METRICS, 1.0/METRICS, 0.0) //Front face
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

bool FillVolume::get(uint64_t x, uint64_t y, uint64_t z) const
{
	if(x >= m_x || y >= m_y || z >= m_z)
		return false;
	uint64_t selfShift = x + m_x*y + m_x*m_y*z;
	uint8_t self          = *(m_fillVolume + (selfShift)/8);
	self               = (self >> (selfShift % 8)) & 0x01;

	return self;
}

bool FillVolume::getSave(uint64_t x, uint64_t y, uint64_t z) const
{
	if(x >= m_x || y >= m_y || z >= m_z)
		return false;
	uint64_t selfShift = x + m_x*y + m_x*m_y*z;
	uint8_t self          = *(m_saveVolume + (selfShift)/8);
	self               = (self >> (selfShift % 8)) & 0x01;

	return self;
}

void FillVolume::setSelectionMode(SelectionMode s)
{
	switch(s)
	{
		case INTERSECT:
			if(m_selectionMode != INTERSECT)
			{
				memcpy(m_saveVolume, m_fillVolume, (m_x*m_y*m_z+7)/8);
				memset(m_fillVolume, 0x00, (m_x*m_y*m_z+7)/8);
			}
			break;
		default:
			memcpy(m_saveVolume, m_fillVolume, (m_x*m_y*m_z+7)/8);
			break;
	}

	m_selectionMode = s;
}

void FillVolume::saveToFile(const std::string& modelPath, uint32_t userID, uint32_t nbTrial)
{
    char nbTrialString[4];
	sprintf(nbTrialString, "%d", nbTrial%3);
	char userIDString[3];
	sprintf(userIDString, "%d", userID);

	mkdir(userIDString, 0755);
	mkdir((std::string(userIDString) + "/" + std::to_string(nbTrial/3)).c_str(), 0755);
	mkdir((std::string(userIDString) + "/" + std::to_string(nbTrial/3) + "/" + std::string(nbTrialString)).c_str(), 0755);

	char nbWriteString[4];
	sprintf(nbWriteString, "%d", m_nbWrite);

	std::string path = std::string(userIDString) + "/" + std::to_string(nbTrial/3) + "/"+std::string(nbTrialString) + "/" + nbWriteString;

	FILE* f = fopen(path.c_str(), "w");
/*
	//Write the user ID
	fwrite(&userID, sizeof(uint32_t), 1, f);
	//Write the model Path
	uint32_t n = modelPath.size();
	fwrite(&n, sizeof(uint32_t), 1, f);
	fwrite(modelPath.c_str(), sizeof(char), modelPath.size(), f);


	//Get the timer in Nano seconds
	struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

	uint64_t diffNS = spec.tv_nsec - m_ns;
	fwrite(&diffNS, sizeof(uint64_t), 1, f);

	m_ns = spec.tv_nsec;
	switch(m_selectionMode)
	{
		case UNION:
			m_nbUnion++;
			break;
		case INTERSECT:
			m_nbInter++;
			break;
		case EXCLUSION:
			m_nbDiff++;
			break;
	}

	//Write volume size
	float m = METRICS;
	fwrite(&m, sizeof(float), 1, f);
	fwrite(&m_x, sizeof(uint64_t), 1, f);
	fwrite(&m_y, sizeof(uint64_t), 1, f);
	fwrite(&m_z, sizeof(uint64_t), 1, f);

	//Write data
	fwrite(m_fillVolume, sizeof(char), (m_x*m_y*m_z+7)/8, f);

	fclose(f);
	m_nbWrite++;
	*/

	//Get the timer in Nano seconds
	uint64_t diffMS = SDL_GetTicks() - m_ms;
	m_ms = diffMS + m_ms;

	switch(m_selectionMode)
	{
		case UNION:
			m_nbUnion++;
			break;
		case INTERSECT:
			m_nbInter++;
			break;
		case EXCLUSION:
			m_nbDiff++;
			break;
	}

	fprintf(f, "UserID, modelPath, timer (ms), volumeSize(x), volumeSize(y), volumeSize(z)\n");
	fprintf(f, "%d;%s;%lu;%lu;%lu;%lu\n", userID, modelPath.c_str(), diffMS, m_x, m_y, m_z);
	fclose(f);
	m_nbWrite++;
}

void FillVolume::saveFinalFiles(const std::string& modelPath, uint32_t userID, uint32_t nbTrial, ParticuleObject* particuleObject)
{
    char nbTrialString[4];
	sprintf(nbTrialString, "%d", nbTrial%3);
	char userIDString[3];
	sprintf(userIDString, "%d", userID);

    mkdir(userIDString, 0755);
	mkdir((std::string(userIDString) + "/" + std::to_string(nbTrial/3)).c_str(), 0755);
	mkdir((std::string(userIDString) + "/" + std::to_string(nbTrial/3) + "/" + std::string(nbTrialString)).c_str(), 0755);

	std::string path = std::string(userIDString) + "/" + std::to_string(nbTrial/3) + "/"+std::string(nbTrialString) + "/final";

	FILE* f = fopen(path.c_str(), "w");

/*
	//Write the user ID
	fwrite(&userID, sizeof(uint32_t), 1, f);
	//Write the model Path
	uint32_t n = modelPath.size();
	fwrite(&n, sizeof(uint32_t), 1, f);
	fwrite(modelPath.c_str(), sizeof(char), modelPath.size(), f);

	//Write METRICS
	int metrics = METRICS;
	fwrite(&metrics, sizeof(int), 1, f);

	//Get the timer in Nano seconds
	struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
	m_ns = spec.tv_nsec;
	uint64_t diffNS = m_initNs - m_ns;
	fwrite(&diffNS, sizeof(uint64_t), 1, f);

	//Write the number of operation
	fwrite(&m_nbUnion, sizeof(uint32_t), 1, f);
	fwrite(&m_nbInter, sizeof(uint32_t), 1, f);
	fwrite(&m_nbDiff, sizeof(uint32_t), 1, f);

	//Write volume size
	float m = METRICS;
	fwrite(&m, sizeof(float), 1, f);
	fwrite(&m_x, sizeof(uint64_t), 1, f);
	fwrite(&m_y, sizeof(uint64_t), 1, f);
	fwrite(&m_z, sizeof(uint64_t), 1, f);

	//Write statistics
	ParticuleStats ps;
	particuleObject->getStats(&ps, this);
	fwrite(&(ps.volume), sizeof(int), 1, f);
	fwrite(&(ps.nbParticule), sizeof(int), 1, f);
	fwrite(&(ps.valid), sizeof(int), 1, f);
	fwrite(&(ps.incorrect), sizeof(int), 1, f);
	fwrite(&(ps.inNoise), sizeof(int), 1, f);

	//Write data
	fwrite(m_fillVolume, sizeof(char), (m_x*m_y*m_z+7)/8, f);
*/
	//Get the timer in Nano seconds
	m_ms = SDL_GetTicks();
	uint64_t diffMS = m_ms - m_initMs;

	//Write statistics
	ParticuleStats ps;
	particuleObject->getStats(&ps, this);

	fprintf(f, "UserID; modelPath; timer (ms); METRICS; volumeSize(x); volumeSize(y); volumeSize(z); nbUnion; nbIntersection; nbDiff; volume; nbParticule; nbValide; nbInvalide; Supposed to be selected_Selected; not supposed to be selected_Not Selected; supposed to be selected_Not selected; not supposed to be selection_Selected\n");
	fprintf(f, "%d;%s;%lu;%d;%lu;%lu;%lu;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d\n", userID, modelPath.c_str(), diffMS, METRICS, m_x, m_y, m_z, m_nbUnion, m_nbInter, m_nbDiff, ps.volume, ps.nbParticule, ps.nbValide, ps.nbInvalide, ps.valid, ps.nbParticule - ps.valid - ps.incorrect - ps.inNoise, ps.nbValide - ps.valid, ps.incorrect+ps.inNoise);
	fclose(f);
	m_nbWrite++;
}

void FillVolume::reinitTime()
{
	//Get the timer in Nano seconds
	m_initMs = m_ms = SDL_GetTicks();
}

void FillVolume::commitIntersection()
{

	if(m_selectionMode == INTERSECT)
	{
		memcpy(m_saveVolume, m_fillVolume, (m_x*m_y*m_z+7)/8);
		memset(m_fillVolume, 0x00, (m_x*m_y*m_z+7)/8);
	}
}
