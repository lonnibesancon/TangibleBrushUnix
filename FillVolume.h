#ifndef  FILLVOLUME_INC
#define  FILLVOLUME_INC

#define METRICS 1.0

#include "stdint.h"
#include "stdlib.h"
#include "pthread.h"
#include <cmath>
#include "global.h"
#include <algorithm>
#include "Selection.h"
#include <cstring>

//Structure which contain a Edge
struct Edge
{
	public:
		Edge(const Vector2_f& a, const Vector2_f& b);
		Vector2_f m_a;
		Vector2_f m_b;
		double m_incr;
		double m_yMin;
		double m_yMax;
		double m_startX;
		double m_endX;
		bool m_linear;

		double computeX(double j) const{return (m_linear) ? m_endX : m_a.x + m_incr * (j-m_a.y);}
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
		void init(const std::vector<Vector2_f>& p);
		~FillVolume();

		FillVolume* createUnion(const FillVolume& fv) const;
		FillVolume* createIntersection(const FillVolume& fv) const;
		FillVolume* createExclusion(const FillVolume& fv) const;

		void fillWithSurface(double depth, const Matrix4_f& matrix, const Vector2_f* factor);

		bool get(uint64_t x, uint64_t y, uint64_t z) const;
		bool get(uint64_t v) const{return m_fillVolume[v/8] & (1 << (v%8));}
		void lock();
		void unlock();

		uint64_t getSizeX() const{return m_x/METRICS;}
		uint64_t getSizeY() const{return m_y/METRICS;}
		uint64_t getSizeZ() const{return m_z/METRICS;}

		uint64_t getMetricsSizeX() const{return m_x;}
		uint64_t getMetricsSizeY() const{return m_y;}
		uint64_t getMetricsSizeZ() const{return m_z;}

		bool hasSomething8Bits(uint64_t x, uint64_t y, uint64_t z){return m_fillVolume[(x+ m_x*y + m_x*m_y*z)/8];}
		bool isInit() const{return m_isInit;}
		void setSelectionMode(SelectionMode s);
	private:
		bool m_isInit=false;

		uint8_t* m_fillVolume;
		uint8_t* m_saveVolume;

		uint64_t m_x, m_y, m_z;
		pthread_mutex_t m_mutex;
		std::vector<Vector2_f> m_selectionPoints;
		SelectionMode m_selectionMode=UNION;
};

//The compare edge function, useful for sorting the edge table
bool compareYEdge (const Edge& a, const Edge& b);
bool compareXEdge (const Edge* a, const Edge* b);

Rectangle3f computeRectangle(double x, double y, double z, const Matrix4_f& matrix);

#endif
