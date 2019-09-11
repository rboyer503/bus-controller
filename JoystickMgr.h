/*
 * JoystickMgr.h
 *
 *  Created on: August 22, 2019
 *      Author: rboyer
 */

#ifndef JOYSTICKMGR_H_
#define JOYSTICKMGR_H_

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

class SocketMgr;

class JoystickMgr
{
	SocketMgr * m_pSocketMgr;
	boost::thread m_thread;

public:
	JoystickMgr(SocketMgr * pSocketMgr);
	~JoystickMgr();

	void Terminate();

private:
	void WorkerFunc();

};

#endif /* JOYSTICKMGR_H_ */
