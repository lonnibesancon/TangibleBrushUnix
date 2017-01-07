#ifndef  SELECTION_INC
#define  SELECTION_INC

#include "util/linear_math.h"
#include <vector>

class Selection
{
	public:
		//move constructor
		//use std::move(vector). Don't use your vector then.
		Selection(std::vector<Vector3_f>&& pointSelection) : m_pointSelection(pointSelection){}

		void addPostTreatmentMatrix(const Matrix4& matrix);
		const Matrix4* nextMatrix(); //Return the next matrix. NULL if we attain the end
		uint32_t getNbData() const{return m_moveMatrix.size();}
		const std::vector<Vector3_f> getSelectionPoint() const{return m_pointSelection;}
	private:
		std::vector<Vector3_f> m_pointSelection; //The point drawn on the tablette
		std::vector<Matrix4>   m_moveMatrix;     //The array of all the postTreatment (movement of the tablette after the tablette is in selectionMode) matrix
		uint32_t m_currentData=0;
};

#endif
