#ifndef VECTOR3
#define VECTOR3

#include <math.h>
#include <iostream>

class Vector3
{
public:
	Vector3();
	Vector3(double x, double y, double z);
	Vector3(const Vector3& v);
	Vector3(const double* v);
	Vector3(double x);
	~Vector3();

	double Length() const;
	void Normalized();

	friend double Dot(const Vector3& a, const Vector3& b);
	friend double Length(const Vector3& v);
	friend Vector3 Cross(const Vector3& a, const Vector3& b);
	friend Vector3 Rotate(double rotate_angle, Vector3 v_norm, Vector3  v);

	//Assignment
	Vector3& operator =  (const Vector3& a);
	Vector3& operator += (const Vector3& a);
	Vector3& operator += (double s);
	Vector3& operator -= (const Vector3& a);
	Vector3& operator -= (double s);
	Vector3& operator *= (double s);
	Vector3& operator /= (double s);

	//Arithmetic
	friend Vector3 operator + (const Vector3& a, const Vector3& b);
	friend Vector3 operator + (const Vector3& a, double s);
	friend Vector3 operator + (double s, const Vector3& a);
	friend Vector3 operator - (const Vector3& a, const Vector3& b);
	friend Vector3 operator - (const Vector3& a, double s);
	friend Vector3 operator - (const Vector3& a);
	friend Vector3 operator * (const Vector3& a, const Vector3& b);
	friend Vector3 operator * (const Vector3& a, double s);
	friend Vector3 operator * (double s, const Vector3& a);
	friend Vector3 operator / (const Vector3& a, const Vector3& b);
	friend Vector3 operator / (const Vector3& a, double s);

	//Comparison
	friend bool operator == (const Vector3& a, const Vector3& b);
	friend bool operator != (const Vector3& a, const Vector3& b);
	friend bool operator <= (const Vector3& a, const Vector3& b);
	friend bool operator <  (const Vector3& a, const Vector3& b);
	friend bool operator >= (const Vector3& a, const Vector3& b);
	friend bool operator >  (const Vector3& a, const Vector3& b);

	double m_x;
	double m_y;
	double m_z;
};

#endif
