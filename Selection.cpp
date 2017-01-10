#include "Selection.h"

void Selection::addPostTreatmentMatrix(SelectionMode s, const Matrix4& matrix)
{
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
