/*
 * author	: jack
 * email	: xulongzhong2004@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */
#ifndef _H_AUTOLOCK_IN_
#define _H_AUTOLOCK_IN_

#include<pthread.h>
 
//���ӿ���   
class ILock  
{  
public:  
	virtual ~ILock() {}  
	virtual void Lock() const = 0;  
	virtual void Unlock() const = 0;  

};  

//�ٽ�������   
class CriSection : public ILock  
{  
public:  
	CriSection();  
	~CriSection();  

	virtual void Lock() const;  
	virtual void Unlock() const;  
	int	GetLockref() const;

private:  
	 pthread_mutex_t	m_critclSection;
};  

//��   
class CAutoLock  
{  
public:  
	CAutoLock(const ILock&);  
	~CAutoLock();  
	void Unlock();
	void Lock();

private:  
	const ILock& m_lock;
	bool  m_block;  

}; 

#endif