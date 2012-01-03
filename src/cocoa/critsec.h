
#ifndef CRITSEC_H
#define CRITSEC_H

#include <pthread.h>

#include "i_system.h"


class FCriticalSection
{
public:
	FCriticalSection()
	{
		pthread_mutexattr_t attributes;
		pthread_mutexattr_init( &attributes );
		pthread_mutexattr_settype( &attributes, PTHREAD_MUTEX_RECURSIVE );

		if ( 0 != pthread_mutex_init( &m_mutex, &attributes ) )
		{
			I_FatalError( "Failed to create a critical section mutex." );
		}
		
		pthread_mutexattr_destroy( &attributes );
	}
	
	~FCriticalSection()
	{
		pthread_mutex_destroy( &m_mutex );
	}
	
	void Enter()
	{
		if ( 0 != pthread_mutex_lock( &m_mutex ) )
		{
			I_FatalError( "Failed entering a critical section." );
		}
	}
	
	void Leave()
	{
		if ( 0 != pthread_mutex_unlock( &m_mutex ) )
		{
			I_FatalError( "Failed to leave a critical section." );
		}
	}
	
private:
	pthread_mutex_t m_mutex;
	
};

#endif // CRITSEC_H
