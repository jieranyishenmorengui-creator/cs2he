#include "Math.hpp"
#include <Core/Sdk/Datatypes/Vector.hpp>
#include <Core/Sdk/Datatypes/QAngle.hpp>

auto MATH::AngleVectors( Vector_t angles, Vector_t& forward, Vector_t& right, Vector_t& up ) -> void
{
	float			angle;
	static float	sr, sp, sy, cr, cp, cy;

	angle = angles.y * ( _2PI / 360 );
	sy = sin( angle );
	cy = cos( angle );

	angle = angles.x * ( _2PI / 360 );
	sp = sin( angle );
	cp = cos( angle );

	angle = angles.z * ( _2PI / 360 );
	sr = sin( angle );
	cr = cos( angle );

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;

	right.x = ( -1 * sr * sp * cy + -1 * cr * -sy );
	right.y = ( -1 * sr * sp * sy + -1 * cr * cy );
	right.z = -1 * sr * cp;

	up.x = ( cr * sp * cy + -sr * -sy );
	up.y = ( cr * sp * sy + -sr * cy );
	up.z = cr * cp;
}

auto MATH::CalculateCameraPosition( Vector_t anchorPos, float distance, QAngle_t viewAngles ) -> Vector_t
{
	float yaw = DirectX::XMConvertToRadians( viewAngles.y );
	float pitch = DirectX::XMConvertToRadians( viewAngles.x );

	float x = anchorPos.x + distance * cosf( yaw ) * cosf( pitch );
	float y = anchorPos.y + distance * sinf( yaw ) * cosf( pitch );
	float z = anchorPos.z + distance * sinf( pitch );

	return Vector_t{ x, y, z };
}

auto MATH::NormalizeAngles( QAngle_t angles ) -> QAngle_t
{
	while ( angles.x > 89.0f )
		angles.x -= 180.0f;
	while ( angles.x < -89.0f )
		angles.x += 180.0f;
	while ( angles.y > 180.0f )
		angles.y -= 360.0f;
	while ( angles.y < -180.0f )
		angles.y += 360.0f;
	angles.z = 0.0f;
	return angles;
}

auto MATH::CalcAngle( Vector_t viewPos, Vector_t aimPos ) -> QAngle_t
{
	QAngle_t angle = { 0, 0, 0 };
	Vector_t delta = aimPos - viewPos;

	angle.x = -asin( delta.z / delta.Length( ) ) * ( 180.0f / 3.141592654f );
	angle.y = atan2( delta.y, delta.x ) * ( 180.0f / 3.141592654f );

	return angle;
}
