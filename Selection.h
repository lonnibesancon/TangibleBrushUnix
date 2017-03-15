#ifndef  SELECTION_INC
#define  SELECTION_INC

#include <vector>
#include "global.h"

enum SelectionMode
{
	UNION=1,
	INTERSECT=3,
	EXCLUSION=2
};

class Selection
{
	public:
		//move constructor
		//use std::move(vector). Don't use your vector then.
		Selection(SelectionMode s, std::vector< Vector2_f >& pointSelection): m_pointSelection(pointSelection), m_selectionMode(s){}

		void addPostTreatmentMatrix(SelectionMode s, double sx, double sy, const Matrix4& matrix);
		int nextIndice(); //Return the next indice of the selection, -1 if none
		uint32_t getNbData() const{return m_moveMatrix.size();}
		const std::vector<Vector2_f>& getSelectionPoint() const{return m_pointSelection;}
		const Matrix4_f* getMatrix(int i){return ((i >= getNbData()) ? NULL : &m_moveMatrix[i]);}
		const Vector2_f* getScaleFactor(int i){return ((i < 0 || i >= getNbData()) ? NULL : &m_scale[i]);}
		SelectionMode getSelectionMode() const{return m_selectionMode;}
	private:
		std::vector<Vector2_f> m_pointSelection; //The point drawn on the tablette
		std::vector<Matrix4_f> m_moveMatrix;     //The array of all the postTreatment (movement of the tablette after the tablette is in selectionMode) matrix
		std::vector<Vector2_f> m_scale;
		uint32_t m_currentData=0;
		SelectionMode m_selectionMode;
};

#endif
