#include "Vector3.h"

Vector3::Vector3() : m_x(0.0), m_y(0.0), m_z(0.0)
{

}

Vector3::Vector3(double x, double y, double z)
{
	this->m_x = x;
	this->m_y = y;
	this->m_z = z;
}

Vector3::Vector3(const Vector3& v)
{
	this->m_x = v.m_x;
	this->m_y = v.m_y;
	this->m_z = v.m_z;
}

Vector3::Vector3(const double* v)
{
	this->m_x = v[0];
	this->m_y = v[1];
	this->m_z = v[2];
}

Vector3::Vector3(double x)
{
	this->m_x = x;
	this->m_y = x;
	this->m_z = x;
}

Vector3::~Vector3()
{
}

double Vector3::Length() const
{
	double length = 0.0f;
	length = sqrtf((this->m_x * this->m_x) + (this->m_y * this->m_y) + (this->m_z * this->m_z));
	return length;
}

void Vector3::Normalized()
{
	double len = this->Length();
	if (len > 0.0f) {
		this->m_x = this->m_x / len;
		this->m_y = this->m_y / len;
		this->m_z = this->m_z / len;
	}
}

double Dot(const Vector3& a, const Vector3& b)
{
	double dot = 0.0f;
	dot = (a.m_x * b.m_x) + (a.m_y * b.m_y) + (a.m_z * b.m_z);
	return dot;
}

double Length(const Vector3& v)
{
	double length = 0.0f;
	length = sqrtf((v.m_x * v.m_x) + (v.m_y * v.m_y) + (v.m_z * v.m_z));
	return length;
}

Vector3 Cross(const Vector3& a, const Vector3& b)
{
	Vector3 cross;
	cross.m_x = (a.m_y * b.m_z) - (a.m_z * b.m_y);
	cross.m_y = (a.m_z * b.m_x) - (a.m_x * b.m_z);
	cross.m_z = (a.m_x * b.m_y) - (a.m_y * b.m_x);
	return cross;
}

Vector3 Rotate(double rotate_angle, Vector3 v_norm, Vector3  v)
{
	v_norm.Normalized();
	double cosa = cos(rotate_angle);
	double sina = sin(rotate_angle);

	Vector3 v1 = Vector3(cosa + v_norm.m_x * v_norm.m_x * (1 - cosa), v_norm.m_x * v_norm.m_y * (1 - cosa) - v_norm.m_z * sina, v_norm.m_x * v_norm.m_z * (1 - cosa) + v_norm.m_y * sina);
	Vector3 v2 = Vector3(v_norm.m_y * v_norm.m_x * (1 - cosa) + v_norm.m_z * sina, cosa + v_norm.m_y * v_norm.m_y * (1 - cosa), v_norm.m_y * v_norm.m_z * (1 - cosa) - v_norm.m_x * sina);
	Vector3 v3 = Vector3(v_norm.m_z * v_norm.m_x * (1 - cosa) - v_norm.m_y * sina, v_norm.m_z * v_norm.m_y * (1 - cosa) + v_norm.m_x * sina, cosa + v_norm.m_z * v_norm.m_z * (1 - cosa));
	return Vector3(Dot(v1, v), Dot(v2, v), Dot(v3, v));
}

//Assignment
Vector3& Vector3::operator =  (const Vector3& a)
{
	this->m_x = a.m_x;
	this->m_y = a.m_y;
	this->m_z = a.m_z;
	return *this;
}
Vector3& Vector3::operator += (const Vector3& a)
{
	this->m_x += a.m_x;
	this->m_y += a.m_y;
	this->m_z += a.m_z;

	return *this;
}
Vector3& Vector3::operator += (double s)
{
	this->m_x += s;
	this->m_y += s;
	this->m_z += s;

	return *this;
}
Vector3& Vector3::operator -= (const Vector3& a)
{
	this->m_x -= a.m_x;
	this->m_y -= a.m_y;
	this->m_z -= a.m_z;

	return *this;
}
Vector3& Vector3::operator -= (double s)
{
	this->m_x -= s;
	this->m_y -= s;
	this->m_z -= s;

	return *this;
}
Vector3& Vector3::operator *= (double s)
{
	this->m_x *= s;
	this->m_y *= s;
	this->m_z *= s;

	return *this;
}
Vector3& Vector3::operator /= (double s)
{
	this->m_x /= s;
	this->m_y /= s;
	this->m_z /= s;

	return *this;
}

//Arithmetic
Vector3 operator + (const Vector3& a, const Vector3& b)
{
	Vector3 r;

	r.m_x = a.m_x + b.m_x;
	r.m_y = a.m_y + b.m_y;
	r.m_z = a.m_z + b.m_z;

	return r;
}
Vector3 operator + (const Vector3& a, double s)
{
	Vector3 r;

	r.m_x = a.m_x + s;
	r.m_y = a.m_y + s;
	r.m_z = a.m_z + s;

	return r;
}
Vector3 operator + (double s, const Vector3& a)
{
	Vector3 r;

	r.m_x = a.m_x + s;
	r.m_y = a.m_y + s;
	r.m_z = a.m_z + s;

	return r;
}
Vector3 operator - (const Vector3& a, const Vector3& b)
{
	Vector3 r;

	r.m_x = a.m_x - b.m_x;
	r.m_y = a.m_y - b.m_y;
	r.m_z = a.m_z - b.m_z;

	return r;
}
Vector3 operator - (const Vector3& a, double s)
{
	Vector3 r;

	r.m_x = a.m_x - s;
	r.m_y = a.m_y - s;
	r.m_z = a.m_z - s;

	return r;
}
Vector3 operator - (const Vector3& a)
{
	Vector3 r;

	r.m_x = -a.m_x;
	r.m_y = -a.m_y;
	r.m_z = -a.m_z;

	return r;
}
Vector3 operator * (const Vector3& a, double s)
{
	Vector3 r;

	r.m_x = a.m_x * s;
	r.m_y = a.m_y * s;
	r.m_z = a.m_z * s;

	return r;
}
Vector3 operator * (double s, const Vector3& a)
{
	Vector3 r;

	r.m_x = a.m_x * s;
	r.m_y = a.m_y * s;
	r.m_z = a.m_z * s;

	return r;
}
Vector3 operator / (const Vector3& a, double s)
{
	Vector3 r;

	r.m_x = a.m_x / s;
	r.m_y = a.m_y / s;
	r.m_z = a.m_z / s;

	return r;
}

Vector3 operator * (const Vector3& a, const Vector3& b)
{
	Vector3 r;

	r.m_x = a.m_x * b.m_x;
	r.m_y = a.m_y * b.m_y;
	r.m_z = a.m_z * b.m_z;

	return r;
}

//Comparison
bool operator == (const Vector3& a, const Vector3& b)
{
	return(a.m_x == b.m_x && a.m_y == b.m_y && a.m_z == b.m_z);
}
bool operator != (const Vector3& a, const Vector3& b)
{
	return(a.m_x != b.m_x || a.m_y != b.m_y || a.m_z != b.m_z);
}
bool operator <= (const Vector3& a, const Vector3& b)
{
	return(a.m_x <= b.m_x && a.m_y <= b.m_y && a.m_z <= b.m_z);
}
bool operator <  (const Vector3& a, const Vector3& b)
{
	return(a.m_x < b.m_x&& a.m_y < b.m_y&& a.m_z < b.m_z);
}
bool operator >= (const Vector3& a, const Vector3& b)
{
	return(a.m_x >= b.m_x && a.m_y >= b.m_y && a.m_z >= b.m_z);
}
bool operator >  (const Vector3& a, const Vector3& b)
{
	return(a.m_x > b.m_x && a.m_y > b.m_y && a.m_z > b.m_z);
}
