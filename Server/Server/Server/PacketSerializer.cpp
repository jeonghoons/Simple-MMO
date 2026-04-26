#include "pch.h"
#include "PacketSerializer.h"
#include "Player.h"

shared_ptr<SendBuffer> PacketSerializer::MAKE_SC_ADD_OBJECT(shared_ptr<GameObject> object)
{
	SC_ADD_OBJECT_PACKET packet;
	packet.header = { sizeof(packet), SC_ADD_OBJECT };
	packet.objectInfo = object->GetInfo();
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);

	
	return sendBuffer;
}

shared_ptr<SendBuffer> PacketSerializer::MAKE_SC_REMOVE_OBJECT(int ObjectId)
{
	SC_REMOVE_OBJECT_PACKET packet;
	packet.header = { sizeof(packet), SC_REMOVE_OBJECT };
	packet.objectId = ObjectId;
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);
	return sendBuffer;
}

shared_ptr<SendBuffer> PacketSerializer::MAKE_SC_MOVE_OBJECT(shared_ptr<GameObject> object)
{
	SC_MOVE_PACKET packet;
	packet.header = { sizeof(packet), SC_MOVE_OBJECT };
	packet.objectInfo = object->GetInfo();
	packet.move_time = object->_last_moveTime;
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);
	return sendBuffer;
}

shared_ptr<SendBuffer> PacketSerializer::MAKE_SC_ATTACK(int attackerId, int skillId, int targetId)
{
	SC_ATTACK_PACKET packet;
	packet.header = { sizeof(packet), SC_ATTACK };
	packet.attackerId = attackerId;
	packet.skillId = skillId;
	packet.targetId = targetId;
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);
	return sendBuffer;
}

shared_ptr<SendBuffer> PacketSerializer::MAKE_SC_DAMAGE(int attackerId, int targetId, int damage)
{
	SC_DAMAGE_PACKET packet;
	packet.header = { sizeof(packet), SC_DAMAGE };
	packet.attackerId = attackerId;
	packet.targetId = targetId;
	packet.damage = damage;
	shared_ptr<SendBuffer> sendBuffer = make_shared<SendBuffer>(sizeof(packet));
	sendBuffer->CopyData(&packet, packet.header.size);
	return sendBuffer;
}

