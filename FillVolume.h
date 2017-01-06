#ifndef  FILLVOLUME_INC
#define  FILLVOLUME_INC

#define METRICS 5.0
#include "stdint.h"
#include "stdlib.h"
#include "pthread.h"
#include <cmath>
#include "global.h"
#include <algorithm>

//Structure which contain a Edge
struct Edge
{
	public:
		Edge(const Vector2_d& a, const Vector2_d& b);
		Vector2_d m_a;
		Vector2_d m_b;
		double m_incr;
		double m_yMin;
		double m_yMax;
		double m_startX;
};

struct Rectangle3f
{
	public:
		Rectangle3f(double a, double b, double c, double w, double h, double d);
		double x, y, z, width, height, depth;
};

class FillVolume
{
	public:
		FillVolume(uint64_t x, uint64_t y, uint64_t z);
		~FillVolume();

		FillVolume* createUnion(const FillVolume& fv) const;
		FillVolume* createIntersection(const FillVolume& fv) const;
		FillVolume* createExclusion(const FillVolume& fv) const;

		void fillWithSurface(const std::vector<Vector2_d>& points, double depth, const Matrix4_d& matrix);

		bool get(uint64_t x, uint64_t y, uint64_t z);
		void lock();
		void unlock();

		uint64_t getSizeX(){return m_x/METRICS;}
		uint64_t getSizeY(){return m_y/METRICS;}
		uint64_t getSizeZ(){return m_z/METRICS;}

		uint64_t getMetricsSizeX(){return m_x;}
		uint64_t getMetricsSizeY(){return m_y;}
		uint64_t getMetricsSizeZ(){return m_z;}

		bool hasSomething8Bits(uint64_t x, uint64_t y, uint64_t z){return m_fillVolume[(x+ m_x*y + m_x*m_y*z)/8];}
	private:
		bool* m_fillVolume;
		uint64_t m_x, m_y, m_z;
		pthread_mutex_t m_mutex;
};

//The compare edge function, useful for sorting the edge table
bool compareYEdge (const Edge& a, const Edge& b);
bool compareXEdge (const Edge* a, const Edge* b);

Rectangle3f computeRectangle(double x, double y, double z, const Matrix4_d& matrix);

#endif
