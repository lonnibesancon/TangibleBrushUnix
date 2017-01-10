#ifndef  SELECTION_INC
#define  SELECTION_INC

#include <vector>
#include "global.h"

enum SelectionMode
{
	UNION,
	INTERSECT,
	EXCLUSION
};

class Selection
{
	public:
		//move constructor
		//use std::move(vector). Don't use your vector then.
		Selection(std::vector<Vector2_f>&& pointSelection): m_pointSelection(pointSelection){}

		void addPostTreatmentMatrix(SelectionMode s, const Matrix4& matrix);
		int nextIndice(); //Return the next indice of the selection, -1 if none
		uint32_t getNbData() const{return m_moveMatrix.size();}
		const std::vector<Vector2_f>& getSelectionPoint() const{return m_pointSelection;}
		const Matrix4_f* getMatrix(int i){return ((i < 0 || i >= getNbData()) ? NULL : &m_moveMatrix[i]);}
		SelectionMode getSelectionMode(int i){return ((i < 0 || i >= getNbData()) ? UNION : m_modeSelection[i]);}
	private:
		std::vector<Vector2_f> m_pointSelection; //The point drawn on the tablette
		std::vector<Matrix4_f> m_moveMatrix;     //The array of all the postTreatment (movement of the tablette after the tablette is in selectionMode) matrix
		std::vector<SelectionMode> m_modeSelection;
		uint32_t m_currentData=0;
};

#endif
