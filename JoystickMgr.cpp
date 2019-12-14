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
	bool firstServo = true; // Skip first servo and motor readings since joystick sends -32767 instead of the expected 0.
	bool firstMotor = true;
	bool laneAssist = false;
	bool autoPilot = false;
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
				else if ( (eventNum == 3) && (event.value) )
				{
					autoPilot = !autoPilot;
					oss.str("");
					oss << "autopilot " << (autoPilot ? "on" : "off");
					m_pSocketMgr->SendCommand(oss.str().c_str());
				}
				else if ( (eventNum == 4) && (event.value) )
				{
					oss.str("");
					oss << "shiftleft";
					m_pSocketMgr->SendCommand(oss.str().c_str());
				}
				else if ( (eventNum == 5) && (event.value) )
				{
					oss.str("");
					oss << "shiftright";
					m_pSocketMgr->SendCommand(oss.str().c_str());
				}
				else if ( (eventNum == 6) && (event.value) )
				{
					oss.str("");
					oss << "debugmode";
					m_pSocketMgr->SendCommand(oss.str().c_str());
				}
			}
			else if (event.isAxis())
			{
				cout << "Axis " << eventNum << " is " << event.value << endl;

				if (eventNum == 0)
				{
					if (firstServo)
					{
						currServo = 0;
						firstServo = false;
					}
					else
						currServo = event.value;

					updateServo = true;
				}
				else if (eventNum == 3)
				{
					if (firstMotor)
					{
						currMotor = 0;
						firstMotor = false;
					}
					else
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
