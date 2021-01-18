#include "autolock.h"

//��ʼ���ٽ���Դ����   
CriSection::CriSection()  
{  
	pthread_mutex_init( &m_critclSection, NULL );
}

//�ͷ��ٽ���Դ����   
CriSection::~CriSection()  
{  
	pthread_mutex_destroy( &m_critclSection );
}  

//�����ٽ���������   
void CriSection::Lock() const  
{
	pthread_mutex_lock( (pthread_mutex_t*)&m_critclSection );
}     

//�뿪�ٽ���������   
void CriSection::Unlock() const  
{  
	pthread_mutex_unlock( (pthread_mutex_t*)&m_critclSection );
} 

//����C++���ԣ������Զ�����   
CAutoLock::CAutoLock( const ILock& m, bool lock ) : m_lock(m)  
{ 
	if( lock )
	{
		m_lock.Lock(); 
		m_block = true; 
	}
}  

//����C++���ԣ������Զ�����   
CAutoLock::~CAutoLock()  
{  
        if( m_block )
        {
                m_lock.Unlock();
                m_block=false;
        }
}  

void CAutoLock::Lock()
{
        if( !m_block )
        {
                m_lock.Lock();
                m_block = true;
        }
}

void CAutoLock::Unlock()
{
        if( m_block )
        {
                m_lock.Unlock();
                m_block = false;
        }
}
