#include "pch.h"

thread_local unsigned int Lthreadid = 1;


shared_ptr<Timer> GTimer = make_shared<Timer>();
