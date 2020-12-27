/*
 * author	: jack
 * email	: xulongzhong2004@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */
 
#ifndef _DBUTIL_H_
#define _DBUTIL_H_

#include <vector>
#include <map>
#include <deque>
#include<sys/timeb.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

#define SQLNOTFOUND		100					//��ѯ���ݺ�û���ݣ����ش������
#define DB_SUCCESS		0   /*!< �ɹ���־*/
#define DB_FAILURE		-1   /*!< ʧ�ܱ�־*/

#define DB_MAX_RECORD_ROW 	10001   /*ÿ�����ȡ100������*/
#define DB_MAX_BUFFER_LEN 	8192  /*�Զ��建�����ĳ���*/
#define DB_MAX_VARCHAR_LEN 	4001  /*���ݿ���varchar2����󳤶�Ϊ4000*/
#define DB_MAX_SQLSTMT_LEN 	8192  //ִ�е�sql�������󳤶�


#ifdef WIN32 // win32 thread
#include <winsock2.h>
#include <Windows.h>
#include <process.h>
#define SP_THREAD_CALL __stdcall
#else
#include <pthread.h>
#include <unistd.h>
#define SP_THREAD_CALL
#endif

enum emSQLType
{
	SQLTYPE_DEFAULT = 0,
        SQLTYPE_SELECT = 1,
        SQLTYPE_UPDATE = 2,
        SQLTYPE_DELETE = 3,
        SQLTYPE_INSERT = 4,
        SQLTYPE_DROP = 6,
        SQLTYPE_ALTER = 7
};

enum emDataType
{
	DATA_TYPE_FLOAT,
	DATA_TYPE_DOUBLE,
	DATA_TYPE_INT,
        DATA_TYPE_UINT,
        DATA_TYPE_LONG,
        DATA_TYPE_ULONG,
        DATA_TYPE_LONGLONG,
        DATA_TYPE_ULONGLONG,
        DATA_TYPE_STRING
};

//������� 
enum emDBTYPE
{
	DBTYPE_UNKNOW,
	DBTYPE_STRING,		   	
	DBTYPE_INT,
	DBTYPE_DOUBLE,
	DBTYPE_DATE,
	DBTYPE_DATETIME
};

typedef struct _STColPropty
{
	char	sColName[50];
	int	nColLen;	
	int	nColType;   
	int	nColScale;	//����
	int     nStrLen;
	int     nColOffset;
}STColPropty;

struct  STSqlPropty
{
	STSqlPropty()
	{
		nColNum = 0;
		nTotalColSize = 0;
		nTotalColNameSize = 0;
	}

	int			nColNum;	 //����
	vector<STColPropty> 	vecColPropty; //���������
	int			nTotalColSize;    //һ�е��������ĳ���
	int			nTotalColNameSize; //�ܵ���������
};

int GetSQLType( const char *szSQL );

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

#ifdef __cplusplus
}
#endif

#endif
