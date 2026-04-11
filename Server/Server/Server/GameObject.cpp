#include "pch.h"
#include "GameObject.h"

GameObject::GameObject(Object_Type objectType)
{
	_objectInfo.objectType = objectType;
}

MovableObject::MovableObject(Object_Type objectType)
    : GameObject(objectType)
{
    _maxSpeed = 500.0f;
    _lastMoveTimePoint = chrono::steady_clock::now();
}

void MovableObject::Update(float deltaTime)
{
    if (deltaTime <= 0.f) return;

    ApplyMovement(deltaTime);
}

void MovableObject::Move(PositionInfo& posInfo)
{
    PositionInfo& pos = _objectInfo.position;
    pos = posInfo;
    _velocity = { posInfo.v_x, posInfo.v_y, posInfo.v_z };

    XMVECTOR vVel = XMLoadFloat3(&_velocity);
    _currentSpeed = XMVectorGetX(XMVector3Length(vVel));

    if (_currentSpeed > 0.0001f)
    {
        XMStoreFloat3(&_moveDir, XMVector3Normalize(vVel));
    }
    else
    {
        _moveDir = { 0.f, 0.f, 0.f };
    }

    _lastMoveTimePoint = chrono::steady_clock::now();

}

void MovableObject::Move(const XMFLOAT3& desPos)
{
}

void MovableObject::ApplyMovement(float deltaTime)
{
    if (_currentSpeed <= 0.0f) return;

    PositionInfo& pos = _objectInfo.position;

	XMVECTOR vDir = XMLoadFloat3(&_moveDir);

	XMVECTOR vVel = XMVectorScale(vDir, _currentSpeed);
	XMStoreFloat3(&_velocity, vVel); // 계산된 최종 속도 저장

	// 3. 물리 이동 연산 (현재 위치 + 속도 * 시간)
	XMVECTOR vCurr = XMVectorSet(pos.x, pos.y, pos.z, 0);
	XMVECTOR nextPos = XMVectorMultiplyAdd(vVel, XMVectorReplicate(deltaTime), vCurr);

	XMFLOAT3 finalPos;
	XMStoreFloat3(&finalPos, nextPos);

	pos.x = finalPos.x;
	pos.y = finalPos.y;
	pos.z = finalPos.z;

	// 패킷 동기화용 데이터 세팅
	pos.v_x = _velocity.x;
	pos.v_y = _velocity.y;
	pos.v_z = _velocity.z;

	// 시간 갱신
	auto now = chrono::steady_clock::now();
	_lastMoveTimePoint = now;
	_last_moveTime = static_cast<unsigned int>(
		chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count()
		);
}

