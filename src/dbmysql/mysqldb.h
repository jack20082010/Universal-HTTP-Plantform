/*
 * 
 * author	: jack
 * email	: xulongzhong2004@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#pragma warning(disable:4996)

#ifndef _MYSQLDB_H_
#define _MYSQLDB_H_

#include "mysql.h"
#include "dbutil.h"
#include <stdint.h>

typedef struct _STBindParam
{
	int		nPos;     //�ڼ������� ��1��ʼ����
	char    	strName[50];
	char		*buffer;
	int		buffer_length;
	enum_field_types buffer_type;
	int64_t 	value;		//Bindin�û��������ݵ�ַ
	uint64_t	uvalue;
	double		dvalue;
	emDataType	dataType;
	my_bool 	bIsNull;
	int 		nBatCursorSkip;   //�����α���һ������
	int 		nBatCurRow;	//�����α굱ǰ��λ��

}STBindParam;

typedef vector<STBindParam> STBindParamVec;
typedef map<MYSQL_STMT*,STBindParamVec> STBindParamMap;

class CMysqlSqlca
{

public:

	CMysqlSqlca();
	virtual  ~CMysqlSqlca();

	int Connect( const char *user, const char *passwd, const char *host, const char *dbname, int nPort = 3306 );
	int Destory_stmthp( bool bClearColAttr = true );
	int PrepareSql( const char* strSql );
	int GetAffectRows( );
	int ExecuteSql( const char* szSQL, bool bCursor = false, int pvskip = 0 );
	int QuerySql( char* szSQL, int nStartRow, int nFetchRows );
	int SqlCommit( );
	int SqlRollback( );
	void* GetCurStmtHandle( );
	void SetCurStmtHandle( void* pStmthandle );

	int FetchData( int nFetchRows, int nOrientation, int nStartRow, void* pStmthandle = NULL );
	//int BindArrayIn( int nColNum, char* sCnvtType, void* pVoid,int nLen = 255 );
	int ExecuteArraySql( char *szSQL, int nCount, int pvskip );

	/*-------------------------------------------------------------------------------
	����˵����      �����ñ���λ�ð�  
	���������      nPos--������λ�� ��1��ʼ strValue�ڴ�ᰴ�˷���
	����ֵ��        DB_SUCCESS--�ɹ� DB_FAILURE--ʧ��
	---------------------------------------------------------------------------------*/

	int BindIn( int nPos, int nValue );
	int BindIn( int nPos, long lValue );
	int BindIn( int nPos, long long llValue );
	int BindIn( int nPos, unsigned long long lluValue );
	int BindIn( int nPos, double dValue );
	int BindIn( int nPos, char *strValue );
	
	int BindOut( int nPos, char* format, void* buffer, int buffer_length = -1 );

	int Disconnect( );
	time_t GetLastUseTime();
	int DestroyFreeMem( );
	MYSQL_FIELD* GetColDesHandle( );
	int GetColNum();
	int GetColType( MYSQL_FIELD *colhd );
	int GetColSize( MYSQL_FIELD *colhd );
	int GetColScale( MYSQL_FIELD *colhd );
	int GetColName( MYSQL_FIELD *colhd, char *sColName );
	my_ulonglong GetLastInsertId( );

	/*-------------------------------------------------------------------------------
	����˵����      ȡ��ĳ�е�ֵ,�к�ͨ��fetchdataѭ��ȡ��FetchData() �ӿ����ʹ��
	���������      rowIndex------�кŴ�1��ʼ colIndex------�кŴ�1��ʼ nStrLen-------pStrValue�ַ�����
	���������	pStrValue-----���ص��ַ���ֵ		
	����ֵ��        DB_SUCCESS--�ɹ� DB_FAILURE--ʧ��
	---------------------------------------------------------------------------------*/
	int GetFieldValue( int rowIndex, int colIndex, char* pStrValue, int nStrLen = 0 );

	/*-------------------------------------------------------------------------------
	����˵����      ȡ��ĳ�е�ֵ,�к�ͨ��fetchdataѭ��ȡ��FetchData() �ӿ����ʹ��
	���������      rowIndex------�кŴ�1��ʼ colIndex------�кŴ�1��ʼ
	���������	pDvalue----����double���͵�ֵ
	����ֵ��        DB_SUCCESS--�ɹ� DB_FAILURE--ʧ��
	---------------------------------------------------------------------------------*/
	int GetFieldValue( int rowIndex, int colIndex, double *pDvalue );

	/*-------------------------------------------------------------------------------
	����˵����      ȡ��ĳ�е�ֵ,�к�ͨ��fetchdataѭ��ȡ��FetchData()�ӿ����ʹ��
	�������:	rowIndex------�кŴ�1��ʼ colIndex------�кŴ�1��ʼ
	���������	pLvalue----����int���͵�ֵ
	����ֵ��        DB_SUCCESS--�ɹ� DB_FAILURE--ʧ��
	---------------------------------------------------------------------------------*/
	int GetFieldValue( int rowIndex, int colIndex, int *pLvalue );
	int ClearBindParam( );
	int SetColPropty( );
	int Execute( int iters, int mode );

public:

	STSqlPropty	m_stSqlPropty;
	MYSQL_STMT 	*m_stmthp;
	char            m_errMsg[256]; /*���ش�����Ϣ*/
	int             m_errCode;
	STBindParamMap	m_mapBindInParam;  /*�������������*/
	STBindParamMap	m_mapBindOutParam;
	char 		m_SqlStmt[DB_MAX_SQLSTMT_LEN];
	//static			CMemoryPools m_memoryPools;	
	char		*m_pResult;
	
private:
	
	MYSQL*  	m_pMysql;
	bool 		m_bConnect;
	time_t		m_tmLastUseTime;	   //��¼���һ��ִ��ʱ��
	int  		m_nSqlType;		   //�������
	MYSQL_RES 	*m_pMysqlRes;

	
	int Createm_stmthp( );
	char* ConverToMySqlStr( char *src );
	int CheckErr( int status );
	int BindExecSql( int iters, int mode, int pvskip = 0 );

	int SetOutputParam( int nFetchRows );
	int BatInsertSql( char *szSQL, int nCount, int pvskip, bool bPrepare );
	int InsertArraySql( char *szSQL, int nCount, int pvskip );
	int BindParameter( STBindParamMap &bindParamMap, int nPos, char* format, void *buffer ,int buffer_length );
};


#endif

