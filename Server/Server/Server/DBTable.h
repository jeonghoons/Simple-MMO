#pragma once
#include "pch.h"

struct UserAccount
{
	string accountId;
	string password;
	string name;
	TIMESTAMP_STRUCT creatDate;
	ObjectInfo	objectInfo;
};