#include <iostream>
#include <opencv2/opencv.hpp>
#include "SocketMgr.h"

using namespace std;
using namespace cv;


int main()
{
	const char * serverHostname = "busdriverpi0";
	const string windowName = "Bus Driver Monitor";

	SocketMgr socketMgr;
	if (!socketMgr.Connect(serverHostname))
		return 1;

	while (!socketMgr.HasExited())
	{
		char c = waitKey(50);
		if (c == 'q')
			break;
		else if (c == 's')
			socketMgr.SendCommand("status");
		else if (c == 'c')
			socketMgr.SendCommand("config");
		else if (c == 'm')
			socketMgr.SendCommand("mode");
		else if (c == 'p')
			socketMgr.SendCommand("page");
		else if (c == 'd')
			socketMgr.SendCommand("debug");
		else if (c == '[')
			socketMgr.SendCommand("param1 down");
		else if (c == ']')
			socketMgr.SendCommand("param1 up");
		else if (c == '{')
			socketMgr.SendCommand("param2 down");
		else if (c == '}')
			socketMgr.SendCommand("param2 up");

		auto currFrame = socketMgr.GetCurrentFrame();
		if (currFrame)
		{
			imshow(windowName, *currFrame);
		}
	}

	return 0;
}