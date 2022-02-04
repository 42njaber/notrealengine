
#ifndef  _QUATERNIONS_TEMPLATE_HPP_
# define _QUATERNIONS_TEMPLATE_HPP_

# include "mft/quaternion.hpp"

namespace mft
{
	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T>::quaternion( void ):
		data{1, 0, 0, 0}
	{
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T>::quaternion( T e1, T e2, T e3, T e4 ):
		data{e1, e2, e3, e4}
	{
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T>::quaternion( std::array<T,4> from ):
		data{from}
	{
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	template<typename U>
	constexpr quaternion<T>::quaternion( const quaternion<U> from ):
		data{from.data}
	{
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	quaternion<T> & quaternion<T>::operator=( const quaternion<T> & from )
	{
		std::copy(std::begin(from.data), std::end(from.data), std::begin(data));
		return *this;
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr T & quaternion<T>::operator[]( size_t index )
	{
		if (index >= 0 && index < 4)
			return (data[index]);
		else
			throw std::runtime_error("Vector index out of bounds");
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr const T & quaternion<T>::operator[]( size_t index ) const
	{
		if (index >= 0 && index < 4)
			return (data[index]);
		else
			throw std::runtime_error("Vector index out of bounds");
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T> quaternion<T>::operator+( const quaternion<T> & q2 ) const
	{
		quaternion<T> ret;
		for (int i = 0; i < 4; i++)
			ret[i] = (*this)[i] + q2[i];
		return ret;
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T> quaternion<T>::operator-( const quaternion<T> & q2 ) const
	{
		quaternion<T> ret;
		for (int i = 0; i < 4; i++)
			ret[i] = (*this)[i] - q2[i];
		return ret;
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T> quaternion<T>::operator*( const quaternion<T> & q2 ) const
	{
		return quaternion<T>(
				a * q2.a - b * q2.b - c * q2.c - d * q2.d,
				a * q2.b + b * q2.a + c * q2.d - d * q2.c,
				a * q2.c - b * q2.d + c * q2.a + d * q2.b,
				a * q2.d + b * q2.c - c * q2.b + d * q2.a
				);
	}

	template<typename T>
	requires std::is_floating_point_v<T>
		constexpr quaternion<T> quaternion<T>::operator*(const T& scalar) const
	{
		return quaternion<T>(
			a * scalar,
			b * scalar,
			c * scalar,
			d * scalar
			);
	}

	template<typename T>
	requires std::is_floating_point_v<T>
		constexpr quaternion<T> quaternion<T>::operator/(const T& scalar) const
	{
		return quaternion<T>(
			a / scalar,
			b / scalar,
			c / scalar,
			d / scalar
			);
	}

	template<typename T>
	requires std::is_floating_point_v<T>
		constexpr quaternion<T> quaternion<T>::operator-( void ) const
	{
		return quaternion<T>(-a, -b, -c, -d);
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr bool quaternion<T>::operator==( const quaternion<T> & q2 ) const
	{
		return (a == q2.a & b == q2.b & c == q2.c & d == q2.d);
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr bool quaternion<T>::operator!=( const quaternion<T> & q2 ) const
	{
		return !((*this) == q2);
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	std::ostream & operator<<( std::ostream & o, const quaternion<T> & q )
	{
		o << "(" << q.r << " + " << q.i << "i + " << q.j << "j + " << q.k << "k)";
		return o;
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	quaternion<T> & quaternion<T>::operator+=( const quaternion<T> & q2 )
	{
		for (int i = 0; i < 4; i++)
			data[i] += q2[i];
		return *this;
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	quaternion<T> & quaternion<T>::operator-=( const quaternion<T> & q2 )
	{
		for (int i = 0; i < 4; i++)
			data[i] -= q2[i];
		return *this;
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	quaternion<T> & quaternion<T>::operator*=( const quaternion<T> & q2 )
	{
		(*this) = (*this) * q2;
		return *this;
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T> quaternion<T>::rotation( const vec<T,T,T> axis, T angle )
	{
		return quaternion<T>(
				cos(angle / 2),
				axis.x * sin(angle / 2),
				axis.y * sin(angle / 2),
				axis.z * sin(angle / 2)
				);
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr T quaternion<T>::dot( const quaternion<T>& x, const quaternion<T>& y )
	{
		return (x.a * y.a + x.b * y.b + x.c * y.c + x.d * y.d);
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr T quaternion<T>::length( const quaternion<T>& quat)
	{
		return (sqrt(	quat.a * quat.a + quat.b * quat.b
								+ quat.c * quat.c + quat.d * quat.d));
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T> quaternion<T>::normalized( const quaternion<T>& quat )
	{
		T	len = length(quat);
		if (len <= 0)
			return quaternion<T>();
		T inv = 1 / len;
		return quaternion<T>(inv * quat.a, inv * quat.b, inv * quat.c, inv * quat.d);
	}

	template<typename T>
		requires std::is_floating_point_v<T>
	constexpr quaternion<T> quaternion<T>::slerp( const quaternion<T>& q1,
		const quaternion<T>& q2, T percentage )
	{
		T cosTheta = dot(q1, q2);
		quaternion<T> q3 = q2;

		if (cosTheta < 0.0)
		{
			q3 = -q2;
			cosTheta = -cosTheta;
		}
		if (cosTheta > 1.0 - std::numeric_limits<float>::epsilon())
		{
			return quaternion<T>(
				q1.a + (q3.a - q1.a) * percentage,
				q1.b + (q3.b - q1.b) * percentage,
				q1.c + (q3.c - q1.c) * percentage,
				q1.d + (q3.d - q1.d) * percentage
			);
		}
		else
		{
			float theta = std::acos(cosTheta);
			return quaternion<T>(
				normalized((q1 * std::sin((1.0 - percentage) * theta)
				+ q3 * std::sin(percentage * theta) / std::sin(theta)))
			);
		}
	}
}

#endif
