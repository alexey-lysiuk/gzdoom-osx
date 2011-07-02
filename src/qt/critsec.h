
// Wraps an Qt mutex object. (A critical section is a Windows synchronization
// object similar to a mutex but optimized for access by threads belonging to
// only one process, hence the class name.)

#ifndef SRC_QT_CRITSEC_H_INCLUDED
#define SRC_QT_CRITSEC_H_INCLUDED

#include <QtCore/QMutex>

class FCriticalSection
{
public:
	FCriticalSection()
	: m_mutex( QMutex::Recursive )
	{
		
	}
	
	void Enter()
	{
		m_mutex.lock();
	}
	
	void Leave()
	{
		m_mutex.unlock();
	}
	
private:
	QMutex m_mutex;
	
};

#endif // SRC_QT_CRITSEC_H_INCLUDED
