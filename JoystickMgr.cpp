/*
 * JoystickMgr.cpp
 *
 *  Created on: August 22, 2019
 *      Author: rboyer
 */

#include "JoystickMgr.h"
#include "SocketMgr.h"
#include "joystick.hh"

#include <iostream>
#include <sstream>

#define UPDATE_DELAY 50

using namespace std;


JoystickMgr::JoystickMgr(SocketMgr * pSocketMgr) :
		m_pSocketMgr(pSocketMgr)
{
	m_thread = boost::thread(&JoystickMgr::WorkerFunc, this);
}

JoystickMgr::~JoystickMgr()
{
	Terminate();
	if (m_thread.joinable())
		m_thread.join();
}

void JoystickMgr::Terminate()
{
	m_thread.interrupt();
}

void JoystickMgr::WorkerFunc()
{
	Joystick joystick;
	if (!joystick.isFound())
	{
		cout << "Error: Joystick not found." << endl;
		return;
	}

	int currServo = 0;
	int currMotor = 0;
	bool updateServo = false;
	bool updateMotor = false;
	bool laneAssist = false;
	ostringstream oss;

	while (1)
	{
		JoystickEvent event;
		if (joystick.sample(&event))
		{
			int eventNum = event.number;

			if (event.isButton())
			{
				cout << "Button " << eventNum << " is " << (event.value == 0 ? "up" : "down") << endl;

				if ( (eventNum == 0) && (event.value) )
				{
					laneAssist = !laneAssist;
					oss.str("");
					oss << "laneassist " << (laneAssist ? "on" : "off");
					m_pSocketMgr->SendCommand(oss.str().c_str());
				}
			}
			else if (event.isAxis())
			{
				cout << "Axis " << eventNum << " is " << event.value << endl;

				if (eventNum == 0)
				{
					currServo = event.value;
					updateServo = true;
				}
				else if (eventNum == 3)
				{
					currMotor = event.value;
					updateMotor = true;
				}
			}
		}

		static int updateDelay = UPDATE_DELAY;
		if (--updateDelay == 0)
		{
			updateDelay = UPDATE_DELAY;
			if (updateServo)
			{
				updateServo = false;
				oss.str("");
				oss << "servo " << currServo;
				m_pSocketMgr->SendCommand(oss.str().c_str());
			}

			if (updateMotor)
			{
				updateMotor = false;
				oss.str("");
				oss << "motor " << currMotor;
				m_pSocketMgr->SendCommand(oss.str().c_str());
			}
		}

		// Give main thread an opportunity to shut this thread down.
		try
		{
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
		}
		catch (boost::thread_interrupted &)
        {
			break;
        }
	}
}