#include "pch.h"
#include "GameObject.h"

GameObject::GameObject(ObjectInfo::Object_Type type)
{
	_objectInfo.type = type;
}

MovableObject::MovableObject(ObjectInfo::Object_Type type)
    : GameObject(type)
{
    _maxSpeed = 500.0f;
    _lastMoveTimePoint = chrono::steady_clock::now();
}

void MovableObject::Update(float deltaTime)
{
    if (deltaTime <= 0.f) return;

    UpdateMovement(deltaTime);

    PositionInfo& pos = _objectInfo.position;
    if(GetType() == ObjectInfo::Player)
        printf("Player[%d] - (%f, %f) Yaw: %f)\n", GetId(), pos.x, pos.y, pos.yaw);
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
    PositionInfo& pos = _objectInfo.position;
    XMVECTOR vCurrPos = XMVectorSet(pos.x, pos.y, pos.z, 0);
    XMVECTOR vDest = XMVectorSet(desPos.x, desPos.y, desPos.z, 0);

    // 목적지를 향하는 벡터 계산
    XMVECTOR vDir = XMVectorSubtract(vDest, vCurrPos);
    float dist = XMVectorGetX(XMVector3Length(vDir));

    // 이미 목적지에 너무 가깝다면 정지 처리
    if (dist < 10.0f)
    {
        _currentSpeed = 0.0f;
        _velocity = { 0.f, 0.f, 0.f };
        _moveDir = { 0.f, 0.f, 0.f };
        pos.v_x = 0.f; pos.v_y = 0.f; pos.v_z = 0.f;
        pos.state = Move_State::IDLE;
        return;
    }

    // 방향 정규화 및 속도 적용
    XMStoreFloat3(&_moveDir, XMVector3Normalize(vDir));
    _currentSpeed = _maxSpeed;

    XMVECTOR vVel = XMVectorScale(XMLoadFloat3(&_moveDir), _currentSpeed);
    XMStoreFloat3(&_velocity, vVel);

    // 패킷 브로드캐스트를 위해 프로토콜 데이터에도 반영
    pos.v_x = _velocity.x;
    pos.v_y = _velocity.y;
    pos.v_z = _velocity.z;
    pos.yaw = XMConvertToDegrees(atan2f(_moveDir.y, _moveDir.x));
    pos.state = Move_State::RUN;

    _lastMoveTimePoint = chrono::steady_clock::now();
}

void MovableObject::UpdateMovement(float deltaTime)
{
    return;
    if (_currentSpeed <= 0.0f) return;

    PositionInfo& pos = _objectInfo.position;

    XMVECTOR currPos = XMVectorSet(pos.x, pos.y, pos.z, 0);
    XMVECTOR velocity = XMLoadFloat3(&_velocity);

    // 다음 위치 = 현재 위치 + (속도 * 시간)
    XMVECTOR nextPos = XMVectorMultiplyAdd(velocity, XMVectorReplicate(deltaTime), currPos);

    XMFLOAT3 finalPos;
    XMStoreFloat3(&finalPos, nextPos);

    // TODO: Room->CanGo() 등으로 벽 충돌 검사 추가 부분

    pos.x = finalPos.x;
    pos.y = finalPos.y;
    pos.z = finalPos.z;

    // 시간 갱신
    auto now = chrono::steady_clock::now();
    _lastMoveTimePoint = now;
    _last_moveTime = static_cast<unsigned int>(
        chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count()
        );
}

