#include "Movement.hpp"
#include <Core/Sdk/Datatypes/QAngle.hpp>


static CMovement g_CMovement{ };

void AngleQangles( const QAngle_t& angles, QAngle_t* forward, QAngle_t* right, QAngle_t* up )
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	// Convert angles from degrees to radians
	angle = angles.y * ( MATH::_PI / 180.0 );
	sy = sin( angle );
	cy = cos( angle );
	angle = angles.x * ( MATH::_PI / 180.0 );
	sp = sin( angle );
	cp = cos( angle );
	angle = angles.z * ( MATH::_PI / 180.0 );
	sr = sin( angle );
	cr = cos( angle );

	if ( forward )
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	if ( right )
	{
		right->x = ( -1 * sr * sp * cy + -1 * cr * -sy );
		right->y = ( -1 * sr * sp * sy + -1 * cr * cy );
		right->z = -1 * sr * cp;
	}

	if ( up )
	{
		up->x = ( cr * sp * cy + -sr * -sy );
		up->y = ( cr * sp * sy + -sr * cy );
		up->z = cr * cp;
	}
}

auto CMovement::MovementCorrect( CBaseUserCmdPB* pBaseUserCmd, QAngle_t angle ) -> void
{
	if ( !pBaseUserCmd )
		return;

	auto ViewAngles = pBaseUserCmd->viewangles( );
	Vector_t wish_angle{
		ViewAngles.has_x( ) ? ViewAngles.x( ) : 0.f,
		ViewAngles.has_y( ) ? ViewAngles.y( ) : 0.f,
		ViewAngles.has_z( ) ? ViewAngles.z( ) : 0.f
	};
	int sign = wish_angle.x > 89.f ? -1.f : 1.f;
	wish_angle.Clamp( );

	Vector_t forward, right, up, old_forward, old_right, old_up;
	Vector_t view_angles = Vector_t(angle.x, angle.y, angle .z);

	MATH::AngleVectors( wish_angle, forward, right, up );

	forward.z = right.z = up.x = up.y = 0.f;

	forward.NormalizeInPlace( );
	right.NormalizeInPlace( );
	up.NormalizeInPlace( );

	MATH::AngleVectors( view_angles, old_forward, old_right, old_up );

	old_forward.z = old_right.z = old_up.x = old_up.y = 0.f;

	old_forward.NormalizeInPlace( );
	old_right.NormalizeInPlace( );
	old_up.NormalizeInPlace( );

	forward *= pBaseUserCmd->forwardmove( );
	right *= pBaseUserCmd->leftmove( );
	up *= pBaseUserCmd->upmove( );

	float fixed_forward_move = old_forward.dot( right ) + old_forward.dot( forward ) + old_forward.dot( up, true );

	float fixed_side_move = old_right.dot( right ) + old_right.dot( forward ) + old_right.dot( up, true );

	float fixed_up_move = old_up.dot( right, true ) + old_up.dot( forward, true ) + old_up.dot( up );

	pBaseUserCmd->set_forwardmove( fixed_forward_move );
	pBaseUserCmd->set_leftmove( fixed_side_move );
	pBaseUserCmd->set_upmove( fixed_up_move );

	fixed_forward_move = sign * ( old_forward.dot( right ) + old_forward.dot( forward ) );
	fixed_side_move = old_right.dot( right ) + old_right.dot( forward );

	pBaseUserCmd->set_forwardmove( std::clamp( fixed_forward_move, -1.f, 1.f ) );
	pBaseUserCmd->set_leftmove( std::clamp( fixed_side_move, -1.f, 1.f ) );
}

auto CMovement::FixCmdButtons( CUserCmd* pUserCmd, CBaseUserCmdPB* pBaseUserCmd ) -> void
{
	if ( !pUserCmd || !pBaseUserCmd )
		return;

	if ( pBaseUserCmd->forwardmove( ) > 0.f )
		pUserCmd->buttonState.SetButtonState( IN_FORWARD, CInButtonState::e_button_state_t::in_button_down );
	else if ( pBaseUserCmd->forwardmove( ) < 0.f )
		pUserCmd->buttonState.SetButtonState( IN_BACK, CInButtonState::e_button_state_t::in_button_down );

	if ( pBaseUserCmd->leftmove( ) > 0.f )
		pUserCmd->buttonState.SetButtonState( IN_MOVELEFT, CInButtonState::e_button_state_t::in_button_down );
	else if ( pBaseUserCmd->leftmove( ) < 0.f )
		pUserCmd->buttonState.SetButtonState( IN_MOVERIGHT, CInButtonState::e_button_state_t::in_button_down );
}

auto GetMovement( ) -> CMovement*
{
	return &g_CMovement;
}