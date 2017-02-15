#ifndef LINEAR_MATH_H
#define LINEAR_MATH_H

#include <cmath>
#include <cassert>
#include <cstdlib>
#include <ostream>
#include <iomanip>

namespace LinearMath
{

template<typename T>
 	void printAny(const T& v, const std::string& tag){
	    std::ostringstream oss ;
	    oss << tag << ": " << v ;
	    //LOGW("%s", oss.str().c_str());
	    std::cout << oss.str().c_str() << std::endl ;
 	}



template <typename T>
struct Quaternion;

template <typename T>
struct Vector2
{
	Vector2()
	{ /* uninitialized */ }

	Vector2(T x_, T y_)
	 : x(x_), y(y_)
	{}

	explicit Vector2(T scale)
	 : x(scale), y(scale)
	{}

	explicit Vector2(T const (&array)[2])
	 : x(array[0]), y(array[1])
	{}

	static const Vector2& zero()
	{
		static const Vector2 zero(0, 0);
		return zero;
	}

	static const Vector2& unit()
	{
		static const Vector2 unit(1, 1);
		return unit;
	}

	static const Vector2& unitX()
	{
		static const Vector2 unitX(1, 0);
		return unitX;
	}

	static const Vector2& unitY()
	{
		static const Vector2 unitY(0, 1);
		return unitY;
	}

	Vector2& operator=(const Vector2& other)
	{
		x = other.x;
		y = other.y;
		return *this;
	}

	bool isNull() const
	{
		return (x == 0 || y == 0);
	}

	T dot(const Vector2& other) const
	{
		return x*other.x + y*other.y;
	}

	T angle(const Vector2& other) const
	{
		const T dot = normalized().dot(other.normalized());
		return std::acos(dot > 1 ? 1 : (dot < -1 ? -1 : dot));
	}

	T distance(const Vector2& other) const
	{
		return (*this - other).length();
	}

	T squaredDistance(const Vector2& other) const
	{
		return (*this - other).squaredLength();
	}

	T squaredLength() const
	{
		return dot(*this);
	}

	T length() const
	{
		const T len = squaredLength();
		return (len != 0.0 ? std::sqrt(len) : len);
	}

	Vector2& normalize()
	{
		return (*this /= length());
	}

	Vector2 normalized() const
	{
		return (*this / length());
	}

	const Vector2& operator+() const
	{
		return *this;
	}

	const Vector2 operator-() const
	{
		return Vector2(-x, -y);
	}

	Vector2& operator+=(const Vector2& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	const Vector2 operator+(const Vector2& other) const
	{
		return (Vector2(*this) += other);
	}

	Vector2& operator+=(T scalar)
	{
		x += scalar;
		y += scalar;
		return *this;
	}

	const Vector2 operator+(T scalar) const
	{
		return (Vector2(*this) += scalar);
	}

	Vector2& operator-=(const Vector2& other)
	{
		return (*this += -other);
	}

	Vector2& operator-=(T scalar)
	{
		return (*this += -scalar);
	}

	const Vector2 operator-(const Vector2& other) const
	{
		return (Vector2(*this) -= other);
	}

	const Vector2 operator-(T scalar) const
	{
		return (Vector2(*this) -= scalar);
	}

	Vector2& operator*=(const Vector2& other)
	{
		x *= other.x;
		y *= other.y;
		return *this;
	}

	const Vector2 operator*(const Vector2& other) const
	{
		return (Vector2(*this) *= other);
	}

	Vector2& operator*=(T scale)
	{
		x *= scale;
		y *= scale;
		return *this;
	}

	const Vector2 operator*(T scale) const
	{
		return (Vector2(*this) *= scale);
	}

	Vector2& operator/=(const Vector2& other)
	{
		assert(other.squaredLength() != 0);
		Vector2 invVector = 1 / other;
		return (*this *= invVector);
	}

	const Vector2 operator/(const Vector2& other) const
	{
		return (Vector2(*this) /= other);
	}

	Vector2& operator/=(T scale)
	{
		assert(scale != 0);
		T invScale = 1 / scale;
		return (*this *= invScale);
	}

	const Vector2 operator/(T scale) const
	{
		return (Vector2(*this) /= scale);
	}

	bool operator==(const Vector2& other) const
	{
		return (x == other.x && y == other.y);
	}

	bool operator!=(const Vector2& other) const
	{
		return !(*this == other);
	}

	bool operator<(const Vector2& other) const
	{
		return (x < other.x && y < other.y);
	}

	bool operator>(const Vector2& other) const
	{
		return (x > other.x && y > other.y);
	}

	friend Vector2 operator+(T scalar, const Vector2& v)
	{
		return (Vector2(v) += scalar);
	}

	friend Vector2 operator-(T scalar, const Vector2& v)
	{
		return (Vector2(v) -= scalar);
	}

	friend Vector2 operator*(T scalar, const Vector2& v)
	{
		return (Vector2(v) *= scalar);
	}

	friend Vector2 operator/(T scalar, const Vector2& v)
	{
		assert(v.x != 0);
		assert(v.y != 0);
		return Vector2(scalar/v.x, scalar/v.y);
	}

	friend std::ostream& operator<<(std::ostream& os, const Vector2& v)
	{
		os << "Vector2(" << v.x << ", " << v.y << ")";
		return os;
	}

	T x, y;
};

template <typename T>
struct Vector3
{
	Vector3()
	{ /* uninitialized */ }

	Vector3(T x_, T y_, T z_)
	 : x(x_), y(y_), z(z_)
	{}

	explicit Vector3(T scale)
	 : x(scale), y(scale), z(scale)
	{}

	explicit Vector3(T const (&array)[3])
	 : x(array[0]), y(array[1]), z(array[2])
	{}

	static const Vector3& zero()
	{
		static const Vector3 zero(0, 0, 0);
		return zero;
	}

	static const Vector3& unit()
	{
		static const Vector3 unit(1, 1, 1);
		return unit;
	}

	static const Vector3& unitX()
	{
		static const Vector3 unitX(1, 0, 0);
		return unitX;
	}

	static const Vector3& unitY()
	{
		static const Vector3 unitY(0, 1, 0);
		return unitY;
	}

	static const Vector3& unitZ()
	{
		static const Vector3 unitZ(0, 0, 1);
		return unitZ;
	}

	Vector3& operator=(const Vector3& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}

	bool isNull() const
	{
		return (x == 0 && y == 0 && z == 0);
	}

	T dot(const Vector3& other) const
	{
		return x*other.x + y*other.y + z*other.z;
	}

	Vector3 cross(const Vector3& other) const
	{
		return Vector3(
			y*other.z - z*other.y,
			z*other.x - x*other.z,
			x*other.y - y*other.x
		);
	}

	Vector3 project(const Vector3& other) const
	{
		return *this * dot(other);
	}

	Vector3 projectOnPlane(const Vector3& normal) const
	{
		return *this - normal*dot(normal);
	}

	T angle(const Vector3& other) const
	{
		const T dot = normalized().dot(other.normalized());
		return std::acos(dot > 1 ? 1 : (dot < -1 ? -1 : dot));
	}

	T distance(const Vector3& other) const
	{
		return (*this - other).length();
	}

	T squaredDistance(const Vector3& other) const
	{
		return (*this - other).squaredLength();
	}

	Quaternion<T> rotationTo(const Vector3& other) const;

	T squaredLength() const
	{
		return dot(*this);
	}

	T length() const
	{
		const T len = squaredLength();
		return (len != 0.0 ? std::sqrt(len) : len);
	}

	Vector3& normalize()
	{
		return (*this /= length());
	}

	Vector3 normalized() const
	{
		return (*this / length());
	}

	const Vector3& operator+() const
	{
		return *this;
	}

	const Vector3 operator-() const
	{
		return Vector3(-x, -y, -z);
	}

	Vector3& operator+=(const Vector3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	const Vector3 operator+(const Vector3& other) const
	{
		return (Vector3(*this) += other);
	}

	Vector3& operator+=(T scalar)
	{
		x += scalar;
		y += scalar;
		z += scalar;
		return *this;
	}

	const Vector3 operator+(T scalar) const
	{
		return (Vector3(*this) += scalar);
	}

	Vector3& operator-=(const Vector3& other)
	{
		return (*this += -other);
	}

	const Vector3 operator-(const Vector3& other) const
	{
		return (Vector3(*this) -= other);
	}

	Vector3& operator-=(T scalar)
	{
		return (*this += -scalar);
	}

	const Vector3 operator-(T scalar) const
	{
		return (Vector3(*this) -= scalar);
	}

	Vector3& operator*=(const Vector3& other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	const Vector3 operator*(const Vector3& other) const
	{
		return (Vector3(*this) *= other);
	}

	Vector3& operator*=(T scale)
	{
		x *= scale;
		y *= scale;
		z *= scale;
		return *this;
	}

	const Vector3 operator*(T scale) const
	{
		return (Vector3(*this) *= scale);
	}

	Vector3& operator/=(const Vector3& other)
	{
		assert(other.squaredLength() != 0);
		Vector3 invVector = 1 / other;
		return (*this *= invVector);
	}

	const Vector3 operator/(const Vector3& other) const
	{
		return (Vector3(*this) /= other);
	}

	Vector3& operator/=(T scale)
	{
		assert(scale != 0);
		T invScale = 1 / scale;
		return (*this *= invScale);
	}

	const Vector3 operator/(T scale) const
	{
		return (Vector3(*this) /= scale);
	}

	bool operator==(const Vector3& other) const
	{
		return (x == other.x && y == other.y && z == other.z);
	}

	bool operator!=(const Vector3& other) const
	{
		return !(*this == other);
	}

	bool operator<(const Vector3& other) const
	{
		return (x < other.x && y < other.y && z < other.z);
	}

	bool operator>(const Vector3& other) const
	{
		return (x > other.x && y > other.y && z > other.z);
	}

	friend Vector3 operator+(T scalar, const Vector3& v)
	{
		return (v + scalar);
	}

	friend Vector3 operator-(T scalar, const Vector3& v)
	{
		return (v - scalar);
	}

	friend Vector3 operator*(T scalar, const Vector3& v)
	{
		return (v * scalar);
	}

	friend Vector3 operator/(T scalar, const Vector3& v)
	{
		assert(v.x != 0);
		assert(v.y != 0);
		assert(v.z != 0);
		return Vector3(scalar/v.x, scalar/v.y, scalar/v.z);
	}

	friend std::ostream& operator<<(std::ostream& os, const Vector3& v)
	{
		os << "Vector3(" << v.x << ", " << v.y << ", " << v.z << ")";
		return os;
	}

	T x, y, z;
};

template <typename T>
struct Matrix3
{
	Matrix3()
	{
		// (uninitialized)
	}

	Matrix3(T _00, T _01, T _02,
	        T _10, T _11, T _12,
	        T _20, T _21, T _22)
	{
		data[0][0] = _00;
		data[1][0] = _01;
		data[2][0] = _02;

		data[0][1] = _10;
		data[1][1] = _11;
		data[2][1] = _12;

		data[0][2] = _20;
		data[1][2] = _21;
		data[2][2] = _22;
	}

	explicit Matrix3(T const (&array)[9])
	{
		for (std::size_t i = 0; i < 9; ++i)
			data_[i] = array[i];
	}

	static const Matrix3& zero()
	{
		static const Matrix3 zero(
			0, 0, 0,
			0, 0, 0,
			0, 0, 0
		);
		return zero;
	}

	static const Matrix3& identity()
	{
		static const Matrix3 identity(
			1, 0, 0,
			0, 1, 0,
			0, 0, 1
		);
		return identity;
	}

	T* operator[](std::size_t column)
	{
		assert(column < 3);
		return data[column];
	}

	T const* operator[](std::size_t column) const
	{
		assert(column < 3);
		return data[column];
	}

	T determinant() const
	{
		return data[0][0] * (data[1][1] * data[2][2] - data[1][2] * data[2][1])
		     - data[1][0] * (data[0][1] * data[2][2] - data[0][2] * data[2][1])
		     + data[2][0] * (data[0][1] * data[1][2] - data[0][2] * data[1][1]);
	}

	Matrix3 inverse() const
	{
		T det = determinant();
		assert(det != 0);

		Matrix3 result(
			data[1][1] * data[2][2] - data[2][1] * data[1][2],
			data[2][0] * data[1][2] - data[1][0] * data[2][2],
			data[1][0] * data[2][1] - data[2][0] * data[1][1],
			data[2][1] * data[0][2] - data[0][1] * data[2][2],
			data[0][0] * data[2][2] - data[2][0] * data[0][2],
			data[2][0] * data[0][1] - data[0][0] * data[2][1],
			data[0][1] * data[1][2] - data[1][1] * data[0][2],
			data[1][0] * data[0][2] - data[0][0] * data[1][2],
			data[0][0] * data[1][1] - data[1][0] * data[0][1]
		);

		for (int i = 0; i < 9; ++i)
			result.data_[i] /= det;

		return result;
	}

	Matrix3 transpose() const
	{
		return Matrix3(
			data[0][0], data[0][1], data[0][2],
			data[1][0], data[1][1], data[1][2],
			data[2][0], data[2][1], data[2][2]
		);
	}

	Matrix3 operator*(const Matrix3& other) const
	{
		Matrix3 result(Matrix3::zero());

		for (std::size_t i = 0; i < 3; ++i)
		{
			for (std::size_t j = 0; j < 3; ++j)
			{
				for (std::size_t k = 0; k < 3; ++k)
					// result[i][k] += data[i][j] * other.data[j][k];
					result[j][i] += data[k][i] * other.data[j][k];
			}
		}

		return result;
	}

	Vector3<T> operator*(const Vector3<T>& v) const
	{
		Vector3<T> result;

		result.x = data[0][0] * v.x
		         + data[1][0] * v.y
		         + data[2][0] * v.z;

		result.y = data[0][1] * v.x
		         + data[1][1] * v.y
		         + data[2][1] * v.z;

		result.z = data[0][2] * v.x
		         + data[1][2] * v.y
		         + data[2][2] * v.z;

		return result;
	}

	friend std::ostream& operator<<(std::ostream& os, const Matrix3& m)
	{
		os << "Matrix3(\n";
		for (std::size_t col = 0; col < 3; ++col)
		{
			os << ' ';
			for (std::size_t row = 0; row < 3; ++row)
			{
				os << std::setprecision(6)
				   << std::setw(13)
				   << std::fixed
				   << m.data[row][col]
				   << (row != 2 ? ' ' : '\n');
			}
		}
		os << ')';
		return os;
	}

	union
	{
		T data[3][3];
		T data_[9];
	};
};

template <typename T>
struct Matrix4
{
	Matrix4()
	{
		// (uninitialized)
	}

	Matrix4(T _00, T _01, T _02, T _03,
	        T _10, T _11, T _12, T _13,
	        T _20, T _21, T _22, T _23,
	        T _30, T _31, T _32, T _33)
	{
		data[0][0] = _00;
		data[1][0] = _01;
		data[2][0] = _02;
		data[3][0] = _03;

		data[0][1] = _10;
		data[1][1] = _11;
		data[2][1] = _12;
		data[3][1] = _13;

		data[0][2] = _20;
		data[1][2] = _21;
		data[2][2] = _22;
		data[3][2] = _23;

		data[0][3] = _30;
		data[1][3] = _31;
		data[2][3] = _32;
		data[3][3] = _33;
	}

	explicit Matrix4(const Matrix3<T>& mat)
	{
		for (std::size_t y = 0; y < 3; ++y)
		{
			for (std::size_t x = 0; x < 3; ++x)
				data[y][x] = mat.data[y][x];
		}

		data[3][0] = data[3][1] = data[3][2] = 0;
		data[0][3] = data[1][3] = data[2][3] = 0;
		data[3][3] = 1;
	}

	explicit Matrix4(T const (&array)[16])
	{
		for (std::size_t i = 0; i < 16; ++i)
			data_[i] = array[i];
	}

	static const Matrix4& zero()
	{
		static const Matrix4 zero(
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0
		);
		return zero;
	}

	static const Matrix4& identity()
	{
		static const Matrix4 identity(
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);
		return identity;
	}

	// http://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
	static Matrix4 perspective(T angle, T aspect, T near, T far)
	{
		Matrix4 result = zero();

		assert(near > 0);
		assert(near < far);
		assert(aspect != 0);

		const T f = 1 / std::tan((angle * 0.5) * M_PI / 180);

		result[0][0] = f/aspect;
		result[1][1] = f;
		result[2][2] = (near+far)/(near-far);
		result[3][2] = (2*near*far)/(near-far);
		result[2][3] = -1;

		return result;
	}

	// http://www.opengl.org/sdk/docs/man2/xhtml/glOrtho.xml
	static Matrix4 ortho(T left, T right, T bottom, T top,
	                     T near, T far)
	{
		Matrix4 result = zero();

		assert(left != right);
		assert(top != bottom);
		// assert(near < far);

		result[0][0] =  2 / (right-left);
		result[1][1] =  2 / (top-bottom);
		result[2][2] = -2 / (far-near);
		result[3][0] = -(right+left) / (right-left);
		result[3][1] = -(top+bottom) / (top-bottom);
		result[3][2] = -(far+near) / (far-near);
		result[3][3] = 1;

		return result;
	}

	static Matrix4 makeTransform(
		const Vector3<T>& position,
		const Quaternion<T>& rotation,
		const Vector3<T>& scale);

	T* operator[](std::size_t column)
	{
		assert(column < 4);
		return data[column];
	}

	T const* operator[](std::size_t column) const
	{
		assert(column < 4);
		return data[column];
	}

	Matrix4& setPosition(T xpos, T ypos, T zpos)
	{
		data[3][0] = xpos;
		data[3][1] = ypos;
		data[3][2] = zpos;
		return *this;
	}

	Matrix4& setPosition(const Vector3<T>& pos)
	{
		return setPosition(pos.x, pos.y, pos.z);
	}

	Vector3<T> position() const
	{
		return Vector3<T>(data[3][0], data[3][1], data[3][2]);
	}

	Matrix4& translate(T xpos, T ypos, T zpos)
	{
		data[3][0] += xpos;
		data[3][1] += ypos;
		data[3][2] += zpos;
		return *this;
	}

	Matrix4& translate(const Vector3<T>& pos)
	{
		return translate(pos.x, pos.y, pos.z);
	}

	Matrix4& setScale(T xscale, T yscale, T zscale)
	{
		data[0][0] = xscale;
		data[1][1] = yscale;
		data[2][2] = zscale;
		return *this;
	}

	Matrix4& setScale(T scale)
	{
		return setScale(scale, scale, scale);
	}

	Matrix4& setScale(const Vector3<T>& scaleVector)
	{
		return setScale(scaleVector.x, scaleVector.y, scaleVector.z);
	}

	Matrix4& rescale(T xscale, T yscale, T zscale)
	{
		data[0][0] *= xscale;
		data[1][1] *= yscale;
		data[2][2] *= zscale;
		return *this;
	}

	Matrix4& rescale(const Vector3<T>& scaleVector)
	{
		return rescale(scaleVector.x, scaleVector.y, scaleVector.z);
	}

	// http://jeux.developpez.com/faq/math/?page=determinants_inverses#Q24
	// http://www.matheureka.net/Q119.htm

	T minor_(unsigned int col, unsigned int row) const
	{
		Matrix3<T> subMat;
		unsigned int resultCol = 0, resultRow = 0;

		for (std::size_t origCol = 0; origCol < 4; ++origCol)
		{
			// Skip the column to remove
			if (origCol < col)
				resultCol = origCol;
			else if (origCol > col)
				resultCol = origCol - 1;

			for (std::size_t origRow = 0; origRow < 4; ++origRow)
			{
				// Skip the row to remove
				if (origRow < row)
					resultRow = origRow;
				else if (origRow > row)
					resultRow = origRow - 1;

				if (origCol != col && origRow != row)
					subMat[resultCol][resultRow] = data[origCol][origRow];
			}
		}

		return subMat.determinant();
	}

	T determinant() const
	{
		T result = 0;
		int sign = 1;

		for (std::size_t i = 0; i < 4; ++i)
		{
			// The row used to compute the determinant
			// (0 in this case) can be chosen arbitrarily
			T cofactor = sign * minor_(i, 0);
			result += data[i][0] * cofactor;
			sign *= -1;
		}

		return result;
	}

	Matrix4 inverse() const
	{
		Matrix4 result;

		T det = determinant();
		assert(det != 0);

		T inv[16];
		int i;

		inv[0] = data_[5]  * data_[10] * data_[15] - 
				 data_[5]  * data_[11] * data_[14] - 
				 data_[9]  * data_[6]  * data_[15] + 
				 data_[9]  * data_[7]  * data_[14] +
				 data_[13] * data_[6]  * data_[11] - 
				 data_[13] * data_[7]  * data_[10];

		inv[4] = -data_[4]  * data_[10] * data_[15] + 
				  data_[4]  * data_[11] * data_[14] + 
				  data_[8]  * data_[6]  * data_[15] - 
				  data_[8]  * data_[7]  * data_[14] - 
				  data_[12] * data_[6]  * data_[11] + 
				  data_[12] * data_[7]  * data_[10];

		inv[8] = data_[4]  * data_[9] * data_[15] - 
				 data_[4]  * data_[11] * data_[13] - 
				 data_[8]  * data_[5] * data_[15] + 
				 data_[8]  * data_[7] * data_[13] + 
				 data_[12] * data_[5] * data_[11] - 
				 data_[12] * data_[7] * data_[9];

		inv[12] = -data_[4]  * data_[9] * data_[14] + 
				   data_[4]  * data_[10] * data_[13] +
				   data_[8]  * data_[5] * data_[14] - 
				   data_[8]  * data_[6] * data_[13] - 
				   data_[12] * data_[5] * data_[10] + 
				   data_[12] * data_[6] * data_[9];

		inv[1] = -data_[1]  * data_[10] * data_[15] + 
				  data_[1]  * data_[11] * data_[14] + 
				  data_[9]  * data_[2] * data_[15] - 
				  data_[9]  * data_[3] * data_[14] - 
				  data_[13] * data_[2] * data_[11] + 
				  data_[13] * data_[3] * data_[10];

		inv[5] = data_[0]  * data_[10] * data_[15] - 
				 data_[0]  * data_[11] * data_[14] - 
				 data_[8]  * data_[2] * data_[15] + 
				 data_[8]  * data_[3] * data_[14] + 
				 data_[12] * data_[2] * data_[11] - 
				 data_[12] * data_[3] * data_[10];

		inv[9] = -data_[0]  * data_[9] * data_[15] + 
				  data_[0]  * data_[11] * data_[13] + 
				  data_[8]  * data_[1] * data_[15] - 
				  data_[8]  * data_[3] * data_[13] - 
				  data_[12] * data_[1] * data_[11] + 
				  data_[12] * data_[3] * data_[9];

		inv[13] = data_[0]  * data_[9] * data_[14] - 
				  data_[0]  * data_[10] * data_[13] - 
				  data_[8]  * data_[1] * data_[14] + 
				  data_[8]  * data_[2] * data_[13] + 
				  data_[12] * data_[1] * data_[10] - 
				  data_[12] * data_[2] * data_[9];

		inv[2] = data_[1]  * data_[6] * data_[15] - 
				 data_[1]  * data_[7] * data_[14] - 
				 data_[5]  * data_[2] * data_[15] + 
				 data_[5]  * data_[3] * data_[14] + 
				 data_[13] * data_[2] * data_[7] - 
				 data_[13] * data_[3] * data_[6];

		inv[6] = -data_[0]  * data_[6] * data_[15] + 
				  data_[0]  * data_[7] * data_[14] + 
				  data_[4]  * data_[2] * data_[15] - 
				  data_[4]  * data_[3] * data_[14] - 
				  data_[12] * data_[2] * data_[7] + 
				  data_[12] * data_[3] * data_[6];

		inv[10] = data_[0]  * data_[5] * data_[15] - 
				  data_[0]  * data_[7] * data_[13] - 
				  data_[4]  * data_[1] * data_[15] + 
				  data_[4]  * data_[3] * data_[13] + 
				  data_[12] * data_[1] * data_[7] - 
				  data_[12] * data_[3] * data_[5];

		inv[14] = -data_[0]  * data_[5] * data_[14] + 
				   data_[0]  * data_[6] * data_[13] + 
				   data_[4]  * data_[1] * data_[14] - 
				   data_[4]  * data_[2] * data_[13] - 
				   data_[12] * data_[1] * data_[6] + 
				   data_[12] * data_[2] * data_[5];

		inv[3] = -data_[1] * data_[6] * data_[11] + 
				  data_[1] * data_[7] * data_[10] + 
				  data_[5] * data_[2] * data_[11] - 
				  data_[5] * data_[3] * data_[10] - 
				  data_[9] * data_[2] * data_[7] + 
				  data_[9] * data_[3] * data_[6];

		inv[7] = data_[0] * data_[6] * data_[11] - 
				 data_[0] * data_[7] * data_[10] - 
				 data_[4] * data_[2] * data_[11] + 
				 data_[4] * data_[3] * data_[10] + 
				 data_[8] * data_[2] * data_[7] - 
				 data_[8] * data_[3] * data_[6];

		inv[11] = -data_[0] * data_[5] * data_[11] + 
				   data_[0] * data_[7] * data_[9] + 
				   data_[4] * data_[1] * data_[11] - 
				   data_[4] * data_[3] * data_[9] - 
				   data_[8] * data_[1] * data_[7] + 
				   data_[8] * data_[3] * data_[5];

		inv[15] = data_[0] * data_[5] * data_[10] - 
				  data_[0] * data_[6] * data_[9] - 
				  data_[4] * data_[1] * data_[10] + 
				  data_[4] * data_[2] * data_[9] + 
				  data_[8] * data_[1] * data_[6] - 
				  data_[8] * data_[2] * data_[5];

		for (i = 0; i < 16; i++)
			result.data_[i] = inv[i]/det;

		/* 

		int sign = 1;

		for (std::size_t i = 0; i < 4; ++i)
		{
			for (std::size_t j = 0; j < 4; ++j)
			{
				T cofactor = sign * minor_(i, j);
				result.data[j][i] = cofactor / det;
				sign *= -1;
			}
			sign *= -1;
		}
		*/

		return result;
	}

	Matrix4 transpose() const
	{
		return Matrix4(
			data[0][0], data[0][1], data[0][2], data[0][3],
			data[1][0], data[1][1], data[1][2], data[1][3],
			data[2][0], data[2][1], data[2][2], data[2][3],
			data[3][0], data[3][1], data[3][2], data[3][3]
		);
	}

	Matrix3<T> get3x3Matrix() const
	{
		return Matrix3<T>(
			data[0][0], data[1][0], data[2][0],
			data[0][1], data[1][1], data[2][1],
			data[0][2], data[1][2], data[2][2]
		);
	}

	Vector3<T> transformPos(const Vector3<T>& v, bool dividew = true) const
	{
		Vector3<T> result;

		result.x = data[0][0] * v.x
		        +  data[1][0] * v.y
		        +  data[2][0] * v.z
		        +  data[3][0];

		result.y = data[0][1] * v.x
		        +  data[1][1] * v.y
		        +  data[2][1] * v.z
		        +  data[3][1];

		result.z = data[0][2] * v.x
		        +  data[1][2] * v.y
		        +  data[2][2] * v.z
		        +  data[3][2];

		if (dividew) {
			T w =      data[0][3] * v.x
			        +  data[1][3] * v.y
			        +  data[2][3] * v.z
			        +  data[3][3];
			result /= w;
		}

		return result;
	}

	Vector3<T> transformDir(const Vector3<T>& v) const
	{
		Vector3<T> result;

		result.x = data[0][0] * v.x
		        +  data[1][0] * v.y
		        +  data[2][0] * v.z;

		result.y = data[0][1] * v.x
		        +  data[1][1] * v.y
		        +  data[2][1] * v.z;

		result.z = data[0][2] * v.x
		        +  data[1][2] * v.y
		        +  data[2][2] * v.z;

		return result;
	}

	Vector3<T> operator*(const Vector3<T>& v) const
	{
		return transformPos(v);
	}

	Matrix4 operator*(const Matrix4& other) const
	{
		Matrix4 result(Matrix4::zero());

		for (std::size_t i = 0; i < 4; ++i)
		{
			for (std::size_t j = 0; j < 4; ++j)
			{
				for (std::size_t k = 0; k < 4; ++k)
					// result[i][k] += data[i][j] * other.data[j][k];
					result[j][i] += data[k][i] * other.data[j][k];
			}
		}

		return result;
	}

	friend std::ostream& operator<<(std::ostream& os, const Matrix4& m)
	{
		os << "Matrix4(\n";
		for (std::size_t col = 0; col < 4; ++col)
		{
			os << ' ';
			for (std::size_t row = 0; row < 4; ++row)
			{
				os << std::setprecision(6)
				   << std::setw(13)
				   << std::fixed
				   << m.data[row][col]
				   << (row != 3 ? ' ' : '\n');
			}
		}
		os << ')';
		return os;
	}

	union
	{
		T data[4][4];
		T data_[16];
	};
};

template <typename T>
struct Quaternion
{
	Quaternion()
	{
		// (uninitialized)
	}

	Quaternion(T x_, T y_, T z_, T w_)
	{
		x = x_;
		y = y_;
		z = z_;
		w = w_;
	}

	Quaternion(const Quaternion<float>& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
	}

	Quaternion(const Quaternion<double>& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
	}

	// http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation#Quaternion_from_axis-angle
	Quaternion(const Vector3<T>& axis, T angle)
	{
		Vector3<T> vn = axis.normalized();
		angle *= 0.5;
		T sinAngle = std::sin(angle);
		x = vn.x * sinAngle;
		y = vn.y * sinAngle;
		z = vn.z * sinAngle;
		w = std::cos(angle);
	}

	static const Quaternion& zero()
	{
		static const Quaternion zero(0, 0, 0, 0);
		return zero;
	}

	static const Quaternion& identity()
	{
		static const Quaternion identity(0, 0, 0, 1);
		return identity;
	}

	Quaternion& operator=(const Quaternion& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
		return *this;
	}

	T dot(const Quaternion& other) const
	{
		return x*other.x + y*other.y + z*other.z + w*other.w;
	}

	T squaredLength() const
	{
		return dot(*this);
	}

	T length() const
	{
		return std::sqrt(squaredLength());
	}

	Quaternion& normalize()
	{
		assert(squaredLength() > 0);
		*this = *this * (1 / length());
		return *this;
	}

	Quaternion normalized() const
	{
		return Quaternion(*this).normalize();
	}

	Quaternion inverse() const
	{
		assert(squaredLength() > 0);
		return Quaternion(-x, -y, -z, w) * (1 / squaredLength());
	}

	Quaternion conjugate() const
	{
		return Quaternion(-x, -y, -z, w);
	}

	bool operator==(const Quaternion& other) const
	{
		return (x == other.x && y == other.y
		        && z == other.z && w == other.w);
	}

	bool operator!=(const Quaternion& other) const
	{
		return !(*this == other);
	}

	const Quaternion& operator+() const
	{
		return *this;
	}

	Quaternion operator-() const
	{
		return conjugate();
	}

	Quaternion operator+(const Quaternion& q) const
	{
		return Quaternion(x+q.x, y+q.y, z+q.z, w+q.w);
	}

	Quaternion operator-(const Quaternion& q) const
	{
		return Quaternion(x-q.x, y-q.y, z-q.z, w-q.w);
	}

	Quaternion operator*(T scale) const
	{
		return Quaternion(x*scale, y*scale, z*scale, w*scale);
	}

	friend Quaternion operator*(T scale, const Quaternion& q)
	{
		return Quaternion(q.x*scale, q.y*scale, q.z*scale, q.w*scale);
	}

	// http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation#Multiplying_quaternions
	Quaternion operator*(const Quaternion& q) const
	{
		return Quaternion(
			w*q.x + x*q.w + y*q.z - z*q.y,
			w*q.y + y*q.w + z*q.x - x*q.z,
			w*q.z + z*q.w + x*q.y - y*q.x,
			w*q.w - x*q.x - y*q.y - z*q.z
		);
	}

	// http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation#Rotating_vectors
	Vector3<T> operator*(const Vector3<T>& v) const
	{
		Quaternion q = Quaternion(v.x, v.y, v.z, 0) * conjugate();
		q = *this * q;
		return Vector3<T>(q.x, q.y, q.z);
	}

	// http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation#Quaternion_to_Matrix
	Matrix3<T> rotationMatrix3() const
	{
		T x2 = x*x;
		T y2 = y*y;
		T z2 = z*z;
		T xy = x*y;
		T xz = x*z;
		T yz = y*z;
		T wx = w*x;
		T wy = w*y;
		T wz = w*z;

		return Matrix3<T>(
			1-2*(y2+z2), 2*(xy-wz),   2*(xz+wy),
			2*(xy+wz),   1-2*(x2+z2), 2*(yz-wx),
			2*(xz-wy),   2*(yz+wx),   1-2*(x2+y2)
		);
	}

	Matrix4<T> rotationMatrix() const
	{
		return Matrix4<T>(rotationMatrix3());
	}

	friend std::ostream& operator<<(std::ostream& os, const Quaternion& q)
	{
		os << "Quaternion(" << q.x << ", " << q.y << ", " << q.z << ", " << q.w << ")";
		return os;
	}

	T x, y, z, w;
};

template <typename T>
inline Quaternion<T> Vector3<T>::rotationTo(const Vector3& other) const
{
	return Quaternion<T>(cross(other), angle(other));
}

// Highly unoptimized implementation...
template <typename T>
inline Matrix4<T> Matrix4<T>::makeTransform(
	const Vector3<T>& position,
	const Quaternion<T>& rotation = Quaternion<T>::identity(),
	const Vector3<T>& scale = Vector3<T>::unit())
{
	Matrix4 result = Matrix4::identity();

	result.setScale(scale);
	result = rotation.rotationMatrix() * result;
	result.setPosition(position);

	return result;
}

} // namespace LinearMath

typedef LinearMath::Vector2<float> Vector2_f;
typedef LinearMath::Vector3<float> Vector3_f;
typedef LinearMath::Matrix3<float> Matrix3_f;
typedef LinearMath::Matrix4<float> Matrix4_f;
typedef LinearMath::Quaternion<float> Quaternion_f;

typedef LinearMath::Vector2<double> Vector2_d;
typedef LinearMath::Vector3<double> Vector3_d;
typedef LinearMath::Matrix3<double> Matrix3_d;
typedef LinearMath::Matrix4<double> Matrix4_d;
typedef LinearMath::Quaternion<double> Quaternion_d;

#ifdef DOUBLE_PRECISION

typedef Vector2_d Vector2;
typedef Vector3_d Vector3;
typedef Matrix3_d Matrix3;
typedef Matrix4_d Matrix4;
typedef Quaternion_d Quaternion;

#else

typedef Vector2_f Vector2;
typedef Vector3_f Vector3;
typedef Matrix3_f Matrix3;
typedef Matrix4_f Matrix4;
typedef Quaternion_f Quaternion;

#endif /* DOUBLE_PRECISION */

#endif /* LINEAR_MATH_H */
