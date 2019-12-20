#include <iostream>
#include <opencv2/opencv.hpp>
#include "SocketMgr.h"
#include "JoystickMgr.h"

using namespace std;
using namespace cv;


int main()
{
	const char * serverHostname = "busdriverpi0";
	const string windowName = "Bus Driver Monitor";

	SocketMgr socketMgr;
	if (!socketMgr.Connect(serverHostname))
		return 1;

	JoystickMgr joystickMgr(&socketMgr);

	namedWindow(windowName, WINDOW_NORMAL);
	resizeWindow(windowName, 640, 480);

	Mat rotationMatrix = getRotationMatrix2D(Point2f(160.0, 120.0), 8.0, 1.0);

	bool debugMode = false;
	bool autoPilot = false;
	while (!socketMgr.HasExited())
	{
		char c = waitKey(50);
		if (c == 'q')
			break;
		else if (c == '*')
		{
			socketMgr.SendCommand("debugmode");
			debugMode = !debugMode;
		}
		else if (c == ' ')
		{
			autoPilot = !autoPilot;
			if (autoPilot)
				socketMgr.SendCommand("autopilot on");
			else
				socketMgr.SendCommand("autopilot off");
		}
		else
		{
			if (debugMode)
			{
				if (c == 's')
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
			}
		}

		auto currFrame = socketMgr.GetCurrentFrame();
		if (currFrame)
		{
			if ( (currFrame->size().width > 0) && (currFrame->size().height > 0) )
			{
				if ( !debugMode && (currFrame->size().height > 40) )
				{
					Mat rotated;
					warpAffine(*currFrame, rotated, rotationMatrix, currFrame->size());

					Mat roi = rotated(Rect(14, 20, rotated.cols - 28, rotated.rows - 40));
					imshow(windowName, roi);
				}
				else if (currFrame->size().height <= 40)
					imshow(windowName, *currFrame);
			}
			else
				cerr << "Debug: Missed frame" << endl;
		}
	}

	return 0;
}
