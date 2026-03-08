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
    _acceleration = 2048.0f;
    _lastMoveTimePoint = chrono::steady_clock::now();
}

void MovableObject::Update(float deltaTime)
{
    if (deltaTime <= 0.f) return;

    PositionInfo& pos = _objectInfo.position;

    if (pos.inputX != 0 || pos.inputY != 0 || _currentSpeed > 0)
    {
        UpdateMovement(deltaTime);
    }

    if(GetType() == ObjectInfo::Player)
        printf("Player[%d] - (%f, %f) Yaw: %f)\n", GetId(), pos.x, pos.y, pos.yaw);
}

void MovableObject::UpdateMovement(float deltaTime)
{
    PositionInfo& pos = _objectInfo.position;

    // 1. 입력 방향 벡터 생성 (Y는 0으로 고정하여 평면 이동)
    XMVECTOR inputVec = XMVectorSet(pos.inputX, 0, pos.inputY, 0);

    // 2. 입력 여부에 따른 처리
    if (!XMVector3Equal(inputVec, XMVectorZero()))
    {
        XMVECTOR dirVec = XMVector3Normalize(inputVec);
        XMStoreFloat3(&_moveDir, dirVec);

        // [Acceleration] 속도 증가
        _currentSpeed += _acceleration * deltaTime;
        if (_currentSpeed > _maxSpeed)
            _currentSpeed = _maxSpeed;

        // [Rotation] 회전값 갱신 (Look At)
        pos.yaw = XMConvertToDegrees(atan2f(pos.inputX, pos.inputY));
        pos.state = Move_State::RUN;
    }
    else
    {
        _currentSpeed = 0.0f;
        _velocity = { 0.f, 0.f, 0.f };
        pos.state = Move_State::IDLE;
    }

    // 3. 최종 속도 벡터 계산 (Velocity = Direction * CurrentSpeed)
    XMVECTOR velVec = XMVectorScale(XMLoadFloat3(&_moveDir), _currentSpeed);
    XMStoreFloat3(&_velocity, velVec);

    ApplyMovement(deltaTime);
}

void MovableObject::ApplyMovement(float deltaTime)
{
    PositionInfo& pos = _objectInfo.position;

    XMVECTOR currPos = XMVectorSet(pos.x, pos.y, pos.z, 0);
    XMVECTOR velocity = XMLoadFloat3(&_velocity);

    XMVECTOR nextPos = XMVectorMultiplyAdd(velocity, XMVectorReplicate(deltaTime), currPos);

    // 2. 결과 저장
    XMFLOAT3 finalPos;
    XMStoreFloat3(&finalPos, nextPos);

    // TODO: 맵 경계 체크 또는 Room->CanGo() 로직 삽입
    pos.x = finalPos.x;
    pos.y = finalPos.y;
    pos.z = finalPos.z;

    _lastMoveTimePoint = chrono::steady_clock::now();

    _last_moveTime = static_cast<unsigned int>(
        chrono::duration_cast<chrono::milliseconds>(
            _lastMoveTimePoint.time_since_epoch()
        ).count()
        );
}