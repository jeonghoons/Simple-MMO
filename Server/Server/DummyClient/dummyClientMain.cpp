
#include "pch.h"
#include "Network.h"
#include <string>
#include "Object.h"
#include "ClientApp.h"

//void HandleError(const char* cause)
//{
//	int errCode = ::WSAGetLastError();
//	cout << cause << " ErrorCode : " << errCode << endl;
//}

int main()
{
    ClientApp app;
    app.Run();
    return 0;
}