//
// Copyright 2019 EchoNous Inc.
//

// Implementation for geometry.h

template <typename T>
T clamp(T val, T low, T high)
{
	if (val < low)
		return low;
	if (val > high)
		return high;
	return val;
}

template <typename T>
tvec2<T>& tvec2<T>::operator+=(const tvec2<T> &rhs)
{
	this->x+=rhs.x;
	this->y+=rhs.y;
	return *this;
}
template <typename T>
tvec2<T>& tvec2<T>::operator-=(const tvec2<T> &rhs)
{
	this->x-=rhs.x;
	this->y-=rhs.y;
	return *this;
}

template <typename T>
tvec2<T>& tvec2<T>::operator*=(const T &s)
{
	this->x*=s;
	this->y*=s;
	return *this;
}
template <typename T>
tvec2<T>& tvec2<T>::operator/=(const T &s)
{
	this->x/=s;
	this->y/=s;
	return *this;
}


template <typename T>
tvec3<T>& tvec3<T>::operator+=(const tvec3<T> &rhs)
{
	this->x+=rhs.x;
	this->y+=rhs.y;
	this->z+=rhs.z;
	return *this;
}
template <typename T>
tvec3<T>& tvec3<T>::operator-=(const tvec3<T> &rhs)
{
	this->x-=rhs.x;
	this->y-=rhs.y;
	this->z-=rhs.z;
	return *this;
}

template <typename T>
tvec3<T>& tvec3<T>::operator*=(const T &s)
{
	this->x*=s;
	this->y*=s;
	this->z*=s;
	return *this;
}
template <typename T>
tvec3<T>& tvec3<T>::operator/=(const T &s)
{
	this->x/=s;
	this->y/=s;
	this->z/=s;
	return *this;
}


template <typename T>
tvec4<T>& tvec4<T>::operator+=(const tvec4<T> &rhs)
{
	this->x+=rhs.x;
	this->y+=rhs.y;
	this->z+=rhs.z;
	this->w+=rhs.w;
	return *this;
}
template <typename T>
tvec4<T>& tvec4<T>::operator-=(const tvec4<T> &rhs)
{
	this->x-=rhs.x;
	this->y-=rhs.y;
	this->z-=rhs.z;
	this->w-=rhs.w;
	return *this;
}

template <typename T>
tvec4<T>& tvec4<T>::operator*=(const T &s)
{
	this->x*=s;
	this->y*=s;
	this->z*=s;
	this->w*=s;
	return *this;
}
template <typename T>
tvec4<T>& tvec4<T>::operator/=(const T &s)
{
	this->x/=s;
	this->y/=s;
	this->z/=s;
	this->w/=s;
	return *this;
}

// external operations
template <typename T>
tvec2<T> operator-(const tvec2<T> &v)
{
	return tvec2<T>{-v.x, -v.y};
}
template <typename T>
tvec2<T> operator+(const tvec2<T> &lhs, const tvec2<T> &rhs)
{
	tvec2<T> r = lhs;
	r += rhs;
	return r;
}
template <typename T>
tvec2<T> operator-(const tvec2<T> &lhs, const tvec2<T> &rhs)
{
	tvec2<T> r = lhs;
	r -= rhs;
	return r;
}
// multiply assumes commutative
template <typename T>
tvec2<T> operator*(const tvec2<T> &lhs, const T &s)
{
	tvec2<T> r = lhs;
	r *= s;
	return r;
}
template <typename T>
tvec2<T> operator*(const T &s, const tvec2<T> &rhs)
{
	tvec2<T> r = rhs;
	r *= s;
	return r;
}
template <typename T>
tvec2<T> operator/(const tvec2<T> &lhs, const T &s)
{
	tvec2<T> r = lhs;
	r /= s;
	return r;
}
// other geometry functions
template <typename T>
T dot(const tvec2<T> &lhs, const tvec2<T> &rhs)
{
	return lhs.x*rhs.x + lhs.y*rhs.y;
}
// length and distance always returned as float
template <typename T>
float length(const tvec2<T> &v)
{
	return std::sqrt(dot(v,v));
}
template <typename T>
float distance(const tvec2<T> &lhs, const tvec2<T> &rhs)
{
	return length(rhs-lhs);
}
template <typename T>
tvec2<T> normalize(const tvec2<T> &v)
{
	return v / length(v);
}

template <typename T>
tvec3<T> operator-(const tvec3<T> &v)
{
	return tvec3<T>{-v.x, -v.y, -v.z};
}
template <typename T>
tvec3<T> operator+(const tvec3<T> &lhs, const tvec3<T> &rhs)
{
	tvec3<T> r = lhs;
	r += rhs;
	return r;
}
template <typename T>
tvec3<T> operator-(const tvec3<T> &lhs, const tvec3<T> &rhs)
{
	tvec3<T> r = lhs;
	r -= rhs;
	return r;
}
// multiply assumes commutative
template <typename T>
tvec3<T> operator*(const tvec3<T> &lhs, const T &s)
{
	tvec3<T> r = lhs;
	r *= s;
	return r;
}
template <typename T>
tvec3<T> operator*(const T &s, const tvec3<T> &rhs)
{
	tvec3<T> r = rhs;
	r *= s;
	return r;
}
template <typename T>
tvec3<T> operator/(const tvec3<T> &lhs, const T &s)
{
	tvec3<T> r = lhs;
	r /= s;
	return r;
}
// other geometry functions
template <typename T>
T dot(const tvec3<T> &lhs, const tvec3<T> &rhs)
{
	return lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z;
}
// length and distance always returned as float
template <typename T>
float length(const tvec3<T> &v)
{
	return std::sqrt(dot(v,v));
}
template <typename T>
float distance(const tvec3<T> &lhs, const tvec3<T> &rhs)
{
	return length(rhs-lhs);
}
template <typename T>
tvec3<T> normalize(const tvec3<T> &v)
{
	return v / length(v);
}
template <typename T>
tvec3<T> cross(const tvec3<T> &lhs, const tvec3<T> &rhs)
{
	return tvec3<T>
	{
		lhs.y*rhs.z - lhs.z*rhs.y,
		lhs.z*rhs.x - lhs.x*rhs.z,
		lhs.x*rhs.y - lhs.y*rhs.x
	};
}


template <typename T>
tvec4<T> operator-(const tvec4<T> &v)
{
	return tvec4<T>{-v.x, -v.y, -v.z, -v.w};
}
template <typename T>
tvec4<T> operator+(const tvec4<T> &lhs, const tvec4<T> &rhs)
{
	tvec4<T> r = lhs;
	r += rhs;
	return r;
}
template <typename T>
tvec4<T> operator-(const tvec4<T> &lhs, const tvec4<T> &rhs)
{
	tvec4<T> r = lhs;
	r -= rhs;
	return r;
}
// multiply assumes commutative
template <typename T>
tvec4<T> operator*(const tvec4<T> &lhs, const T &s)
{
	tvec4<T> r = lhs;
	r *= s;
	return r;
}
template <typename T>
tvec4<T> operator*(const T &s, const tvec4<T> &rhs)
{
	tvec4<T> r = rhs;
	r *= s;
	return r;
}
template <typename T>
tvec4<T> operator/(const tvec4<T> &lhs, const T &s)
{
	tvec4<T> r = lhs;
	r /= s;
	return r;
}
// other geometry functions
template <typename T>
T dot(const tvec4<T> &lhs, const tvec4<T> &rhs)
{
	return lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z + lhs.w*rhs.w;
}
// length and distance always returned as float
template <typename T>
float length(const tvec4<T> &v)
{
	return std::sqrt(dot(v,v));
}
template <typename T>
float distance(const tvec4<T> &lhs, const tvec4<T> &rhs)
{
	return length(rhs-lhs);
}
template <typename T>
tvec4<T> normalize(const tvec4<T> &v)
{
	return v / length(v);
}
