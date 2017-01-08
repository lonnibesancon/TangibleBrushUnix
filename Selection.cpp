#include "Selection.h"

void Selection::addPostTreatmentMatrix(const Matrix4& matrix)
{
	m_moveMatrix.push_back(matrix);
}

const Matrix4* Selection::nextMatrix()
{
	if(m_currentData < m_moveMatrix.size())
	{
		m_currentData++;
		return &(m_moveMatrix[m_currentData-1]);
	}
	return NULL;
}
