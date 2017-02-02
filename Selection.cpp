#include "Selection.h"

void Selection::addPostTreatmentMatrix(SelectionMode s, double sx, double sy, const Matrix4& matrix)
{
	m_scale.push_back(Vector2_f(sx, sy));
	m_moveMatrix.push_back(matrix);
}

int Selection::nextIndice()
{
	if(m_currentData < m_moveMatrix.size())
	{
		m_currentData++;
		return m_currentData-1;
	}
	return -1;
}
