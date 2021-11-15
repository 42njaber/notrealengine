#ifndef _VEC3_H_
# define _VEC3_H_

#include "mft/vec.hpp"

namespace	mft
{
	template <typename T>
	struct	vec<3, T>
	{
		T	x;
		T	y;
		T	z;

		vec(): x(0), y(0), z(0)
		{
		}

		vec(vec<3, T> const & v): x(v.x), y(v.y), z(v.z)
		{
		}

		vec(T x, T y, T z): x(x), y(y), z(z)
		{
		}

		vec(T scalar): x(scalar), y(scalar), z(scalar)
		{
		}

		// Conversion constructor

		template <typename X, typename Y, typename Z>
		vec(X x, Y y, Z z): x(static_cast<T>(x)), y(static_cast<T>(y)), z(static_cast<T>(z))
		{
		}

		// Unary operators

		template <typename U>
		vec<3, T> &	operator=(vec<3, U> const & v)
		{
			x *= static_cast<T>(v.x);
			y *= static_cast<T>(v.y);
			z *= static_cast<T>(v.z);
			return *this;
		}

		template <typename U>
		vec<3, T> &	operator*=(U scalar)
		{
			x *= static_cast<T>(scalar);
			y *= static_cast<T>(scalar);
			z *= static_cast<T>(scalar);
			return *this;
		}

		template <typename U>
		vec<3, T> &	operator/=(U scalar)
		{
			x /= static_cast<T>(scalar);
			y /= static_cast<T>(scalar);
			z /= static_cast<T>(scalar);
			return *this;
		}

		static int length()
		{
			return 3;
		}
	};


	// Binary operators

	template <typename T>
	vec<3, T> operator+(vec<3, T> const & v, T scalar)
	{
		return vec<3, T>(
			v.x + scalar,
			v.y + scalar,
			v.z + scalar);
	}

	template <typename T>
	vec<3, T> operator+(T scalar, vec<3, T> const & v)
	{
		return vec<3, T>(
			scalar + v.x,
			scalar + v.y,
			scalar + v.z);
	}

	template <typename T>
	vec<3, T> operator-(vec<3, T> const & v, T scalar)
	{
		return vec<3, T>(
			v.x - scalar,
			v.y - scalar,
			v.z - scalar);
	}

	template <typename T>
	vec<3, T> operator-(T scalar, vec<3, T> const & v)
	{
		return vec<3, T>(
			scalar - v.x,
			scalar - v.y,
			scalar - v.z);
	}

	template <typename T>
	vec<3, T> operator*(vec<3, T> const & v, T scalar)
	{
		return vec<3, T>(
			v.x * scalar,
			v.y * scalar,
			v.z * scalar);
	}

	template <typename T>
	vec<3, T> operator*(T scalar, vec<3, T> const & v)
	{
		return vec<3, T>(
			scalar * v.x,
			scalar * v.y,
			scalar * v.z);
	}

	template <typename T>
	vec<3, T> operator/(vec<3, T> const & v, T scalar)
	{
		return vec<3, T>(
			v.x / scalar,
			v.y / scalar,
			v.z / scalar);
	}

	template <typename T>
	vec<3, T> operator/(T scalar, vec<3, T> const & v)
	{
		return vec<3, T>(
			scalar / v.x,
			scalar / v.y,
			scalar / v.z);
	}

	template <typename T>
	bool operator==(vec<3, T> const & v1, vec<3, T> const & v2)
	{
		return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z);
	}

	template <typename T>
	bool operator!=(vec<3, T> const & v1, vec<3, T> const & v2)
	{
		return (v1.x != v2.x || v1.y != v2.y || v1.z != v2.z);
	}

	template <typename T>
	std::ostream &		operator<<(std::ostream & o, vec<3, T> const & v)
	{
		std::cout << "{ " << v.x << ", " << v.y << ", " << v.z << " }";
		return o;
	}
}

#endif
