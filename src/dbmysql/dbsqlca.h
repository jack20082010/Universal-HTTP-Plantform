
/*
 * author	: jack
 * email	: xulongzhong2004@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#pragma warning(disable:4996)

#ifndef _CDBSQLCA_H_
#define _CDBSQLCA_H_
#include <stdio.h>
#include "mysqldb.h"

class CDbSqlca
{
public:

	CDbSqlca( );
	~CDbSqlca( );
	/***************************************************************************
	��������: ConnetDB
	��������: �������ݿ�
	��������: 
	***************************************************************************/
	int Connect( char* sUser, char* sPassWd, char* sSvrName, const char *dbname = NULL, int port = 3306 );

	/***************************************************************************
	��������: DisconnectDB
	��������: �Ͽ����ݿ�����
	��������: 
	***************************************************************************/
	int Disconnect( );
	
	inline int ClearBindParam( )
	{
		return m_ClassSqlca.ClearBindParam();
	}
	
	/***************************************************************************
	��������: DBBindIn
	��������: ���������
	***************************************************************************/
	
	inline int BindIn( int nPos, int nValue )
	{
		return m_ClassSqlca.BindIn( nPos, nValue );
	}
	
	inline int BindIn( int nPos, long lValue )
	{
		return m_ClassSqlca.BindIn( nPos, lValue );
	}
	
	inline int BindIn( int nPos, long long llValue )
	{
		return m_ClassSqlca.BindIn( nPos, llValue );
	}
	
	inline int BindIn( int nPos, unsigned long long lluValue )
	{
		return m_ClassSqlca.BindIn( nPos, lluValue );
	}
	
	inline int BindIn( int nPos, double dValue )
	{
		return m_ClassSqlca.BindIn( nPos, dValue );
	}

	inline int BindIn( int nPos, char *strValue )
	{
		return m_ClassSqlca.BindIn( nPos, strValue );
	}
	
	inline int BindIn( int nPos, const char *strValue )
	{
		return m_ClassSqlca.BindIn( nPos, (char*)strValue );
	}
	
	int BindIn( STBindParamVec BinVec );
	

	/***************************************************************************
	��������: DBBindOutParam
	��������: ���������
	***************************************************************************/
	
	inline int BindOut( int nPos, char *format, void *buffer, int buffer_length = -1 )
	{
		return m_ClassSqlca.BindOut( nPos, format, buffer, buffer_length );
	}

	int BindOut( STBindParamVec BinOutVec );
	/***************************************************************************
	��������: BindArrayIn
	��������: �������������
	***************************************************************************/
	//inline int BindArrayIn( int nColNum, char* sCnvtType, void* pVoid, int nLen = 255 )
	//{
	//	return m_ClassSqlca.BindArrayIn( nColNum, sCnvtType, pVoid );
	//}
	/***************************************************************************
	��������: DBExecSql
	��������: ִ��sql���ݿ�ӿڣ�ͬ��֧�ֲַ�ʽ�ֿ�
	
	***************************************************************************/
	int ExecSql( char *szSQL );

	

	int ExecuteArraySql( char* szSQL, int nCount, int pvskip );

	inline int ExecuteArraySql( const char *szSQL, int nCount, int pvskip )
	{
		return ExecuteArraySql( (char*)szSQL, nCount, pvskip );
	}

	/***************************************************************************
	��������: DBOpenCursor
	�������: szSQL --��ѯsql���
			  pvskip--������ȡ���ݣ���һ���ĳ��ȣ�ͨ��ָ�ṹ��ĳ���
	��������: ���α�
	
	***************************************************************************/
	void* OpenCursor( char *szSQL, int pvskip = 0 ); 


	/***************************************************************************
	��������: FetchData
	��������: ���¶�̬��ȡ�α�����
	��������: nFetchRow--��Ҫ��ȡ������	

	����ֵ��    nAffectRows--���ݿ�Ӱ�������,�α�ʵ�ʵ�λ��(�������ۼ�)
				DB_FAILURE--ʧ��	
	***************************************************************************/
	int FetchData( int nFetchRow, void *pStmthandle ); 

	/***************************************************************************
	��������: DBCloseCursor
	��������: �ر��α�,�ͷ���Դ
	
	***************************************************************************/
	int CloseCursor( void *pStmthandle ); 

	/***************************************************************************
	��������: DbSqlcaCommit
	��������: �ύ����
	��������: 
	***************************************************************************/
	int Commit( bool bDestroyStmthp = true );
	
	/***************************************************************************
	��������: DbSqlcaRollback
	��������: �ع�����
	��������: 
	***************************************************************************/
	int Rollback( bool bDestroyStmthp = true);

	//�����е�����
	int SetColAttr( const char* strSql );

	int GetColType( int nColIndex );
	char* GetColName( int nColIndex );
	int GetColSize( int nColIndex );
	int GetColScale( int nColIndex );

	STBindParamVec GetCurBindInVec( );

	STBindParamVec GetCurBindOutVec( );

	inline STSqlPropty* GetSqlPropty( )
	{
		return &m_ClassSqlca.m_stSqlPropty;
	}
	inline int Destory_stmthp( )
	{
		return m_ClassSqlca.Destory_stmthp();
	}

	/*-------------------------------------��������-----------------------------*/
	inline void* OpenCursor( const char *szSQL )
	{
		return OpenCursor( (char*)szSQL );
	}
	inline int ExecSql( const char *szSQL )
	{
		return ExecSql( (char*)szSQL );
	}

	inline int GetDbErrorCode( )
	{
		return m_ClassSqlca.m_errCode;
	}

	inline char* GetDbErrorMsg( )
	{
		return m_ClassSqlca.m_errMsg;
	}

	inline char* GetDbSQLStmt( )
	{
		return m_ClassSqlca.m_SqlStmt;
	}


	inline int  QuerySql( char* szSQL, int nStartRow, int nFetchRows )
	{
		return m_ClassSqlca.QuerySql( szSQL, nStartRow, nFetchRows );
	}

	inline int GetFieldValue( int rowIndex, int colIndex, char* pStrValue, int nStrLen = 0 )
	{
		return m_ClassSqlca.GetFieldValue( rowIndex, colIndex, pStrValue, nStrLen );
	}

	inline int GetFieldValue( int rowIndex, int colIndex, double *pDvalue )
	{
		return m_ClassSqlca.GetFieldValue( rowIndex, colIndex, pDvalue );
	}

	inline int GetFieldValue( int rowIndex,int colIndex, int *pLvalue )
	{
		return m_ClassSqlca.GetFieldValue( rowIndex, colIndex, pLvalue);
	}

	inline int  GetColNum()
	{
		return m_ClassSqlca.GetColNum();
	}

public:
	CMysqlSqlca  		m_ClassSqlca; 

};

class CDbSqlcaPool
{
public:
	CDbSqlcaPool( const char* username, const char* password, const char* dblink, const char* dbname = NULL , int port = 3306 );
	~CDbSqlcaPool();
	int SetMaxConnections( int nNum );
	int SetMinConnections( int nNum );
	int ConnectDB( int nNum );

	CDbSqlca *GetIdleDbSqlca();
	int ReleaseDbsqlca( CDbSqlca* pDbSqlca );
	int GetConnections();
	int GetUsedConnections();
	int HeartBeat();
	void SetConTimeout( int millisecond );
	void SetIdleSleep( int millisecond );
	int GetLastErrCode();
	char* GetLastErrMsg();


private:
	CDbSqlca* ConnectDB();
	int ClearAllDbSqlca();

	CriSection 			m_deqDbSqlcaLock;  
	deque< CDbSqlca* > 		m_deqDbSqlca;
	int 				m_nCurConnections;             //��ǰ���Ӹ���
	map< CDbSqlca*,CDbSqlca* >	m_mapDbSqlca_Resume;
	int 				m_nMaxConnections;
	int 				m_nMinConnections;
	int				m_idleSleep;

	char				m_sUserName[21];
	char				m_sPassword[21];
	char				m_sLinkstr[21]; 
	char    			m_sDbname[21];
	int				m_nTimeout;
	int				m_port;

	char        			m_errMsg[256]; 
	int         			m_errCode;

};

#endif
