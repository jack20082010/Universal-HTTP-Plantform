/*
 * 
 * author	: jack
 * email	: xulongzhong2004@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include "mysqldb.h"
#include "uhp_util.h"
#include <ctype.h>

CMysqlSqlca::CMysqlSqlca()
{
	m_pMysql = NULL;
	m_stmthp = NULL;
	m_bConnect = false;
	m_errCode = 0;
	m_nSqlType = SQLTYPE_DEFAULT;
	m_pMysqlRes = NULL;
	m_pResult = NULL;
	m_tmLastUseTime = 0;
	
	memset( m_SqlStmt, 0, sizeof(m_SqlStmt) );
	memset( m_errMsg, 0, sizeof(m_errMsg) );

}

CMysqlSqlca::~CMysqlSqlca()
{
	if( m_pMysql != NULL )
	{
		mysql_close( m_pMysql );
		m_pMysql = NULL;
	}
}

int CMysqlSqlca::Connect( const char *user, const char *passwd, const char *host, const char *dbname, int nPort )
{
	int nRtn = DB_SUCCESS;
	if( NULL == m_pMysql)
	{
		m_pMysql = mysql_init( NULL );
		if( NULL == m_pMysql )
		{
			m_errCode = -1;
			strcpy( m_errMsg, "��ʼ��ʧ��" );
			return DB_FAILURE;
		}
	}
	
	//�����ַ�������ֹ�����Զ��ύ
	mysql_set_character_set( m_pMysql, "gbk" );
	//�������ӳ�ʱ
	unsigned timeout = 5;
	mysql_options( m_pMysql, MYSQL_OPT_CONNECT_TIMEOUT, (char *)&timeout );

	//����������
	char reconnect = true;
	mysql_options( m_pMysql, MYSQL_OPT_RECONNECT, (char *)&reconnect );

	if( !mysql_real_connect( m_pMysql, host, user, passwd, dbname, nPort, NULL, 0 ) )
	{
		m_errCode = mysql_errno( m_pMysql );
		strcpy( m_errMsg, mysql_error(m_pMysql) );
		return DB_FAILURE;
	}
	
	m_bConnect = true;
	mysql_autocommit( m_pMysql, 0 );

	return DB_SUCCESS;
}

int CMysqlSqlca::Createm_stmthp()
{
	m_stmthp = mysql_stmt_init( m_pMysql ); //����MYSQL_STMT���
	if( NULL== m_stmthp )
	{
		m_errCode = mysql_errno( m_pMysql );
		strcpy( m_errMsg, mysql_error(m_pMysql) );
		return DB_FAILURE;
	}
	
	return DB_SUCCESS;
}
int CMysqlSqlca::Destory_stmthp( bool bClearColAttr )
{
	int nRtn = 0;

	if( NULL != m_pResult )
	{
		//delete[] m_pResult;
		free( m_pResult );
		m_pResult = NULL;
	}
	
	if( NULL != m_pMysqlRes )
	{
		mysql_free_result( m_pMysqlRes );
		m_pMysqlRes = NULL;
	}
	
	ClearBindParam();

	if( m_stmthp )
	{
		 nRtn = mysql_stmt_free_result( m_stmthp );
		 nRtn = mysql_stmt_close( m_stmthp );
		 m_stmthp = NULL;
	}

	if( bClearColAttr )
	{
		m_stSqlPropty.nColNum = 0;
		m_stSqlPropty.nTotalColNameSize = 0;
		m_stSqlPropty.nTotalColSize = 0;
		m_stSqlPropty.vecColPropty.clear();
	}
	
	return DB_SUCCESS;
}

#ifdef _WIN32
static int gettimeofday( struct timeval *tv, void *dummy )
{
        FILETIME        ftime;
        uint64_t        n;

        GetSystemTimeAsFileTime (&ftime);
        n = (((uint64_t) ftime.dwHighDateTime << 32)
             + (uint64_t) ftime.dwLowDateTime);
        if (n) {
                n /= 10;
                n -= ((369 * 365 + 89) * (uint64_t) 86400) * 1000000;
        }

        tv->tv_sec = n / 1000000;
        tv->tv_usec = n % 1000000;
       
        return 0
}
#endif

int CMysqlSqlca::PrepareSql( const char* strSql )
{
	int nRtn = DB_SUCCESS;
	int nLen = strlen( strSql ) + 1;
  	struct timeval tv_time;
  
	//��дʹ��ʱ��
	gettimeofday( &tv_time, NULL );
	m_tmLastUseTime = tv_time.tv_sec *1000 + tv_time.tv_usec / 1000;

	memset( m_SqlStmt, 0, sizeof(m_SqlStmt) );
	if( nLen > DB_MAX_SQLSTMT_LEN )
	{
		strncpy( m_SqlStmt, strSql, DB_MAX_SQLSTMT_LEN );
		m_SqlStmt[DB_MAX_SQLSTMT_LEN-1] ='\0';
	}
	else
	{
		strcpy( m_SqlStmt, strSql );
	}

	m_stmthp = NULL;

	if( NULL == m_stmthp )
	{
		nRtn = Createm_stmthp(); /*����ִ�������*/
		if( DB_SUCCESS != nRtn ) 
			return DB_FAILURE;
		
		nRtn = mysql_stmt_prepare( m_stmthp, strSql, nLen );
		if( nRtn != DB_SUCCESS)
		{
			m_errCode = mysql_errno( m_pMysql );
			strcpy( m_errMsg, mysql_error( m_pMysql) );

			//ע�⣺�����������ӣ����Ǳ��α����´ξ��ܳɹ�
			if( 2013 == m_errCode )
			{
				mysql_ping( m_pMysql );
				
				return DB_FAILURE;
			}
			
		}
	}

	//�滻��ʱkey�İ󶨲���
	STBindParamMap::iterator itBin = m_mapBindInParam.find( NULL);
	if( itBin != m_mapBindInParam.end() )
	{
		STBindParamVec vecParm = itBin->second;
		m_mapBindInParam.erase( itBin );
		m_mapBindInParam[m_stmthp] = vecParm;
	}

	STBindParamMap::iterator itBout = m_mapBindOutParam.find( NULL );
	if( itBout != m_mapBindOutParam.end() )
	{
		STBindParamVec vecParm = itBout->second;
		m_mapBindOutParam.erase( itBout );
		m_mapBindOutParam[m_stmthp] = vecParm;
	}

	return DB_SUCCESS;
}

int CMysqlSqlca::GetAffectRows()
{	
	int nRowCount = 0 ;
	if( SQLTYPE_SELECT == m_nSqlType )
	{
		nRowCount= mysql_stmt_num_rows( m_stmthp );
	}
	else
	{
		nRowCount = (int)mysql_affected_rows( m_pMysql );
	}
	
	return nRowCount;
}

int CMysqlSqlca::ExecuteSql( const char* szSQL, bool bCursor, int pvskip )
{
	int nRtn = 0;
	unsigned int i = 0;

	if( !m_bConnect ) 
		return DB_FAILURE;

	m_nSqlType = GetSQLType( szSQL );
	if( SQLTYPE_DEFAULT == m_nSqlType )
	{
		nRtn = mysql_query( m_pMysql, szSQL );
		if( nRtn != DB_SUCCESS )
		{
			m_errCode = mysql_errno( m_pMysql );
			strcpy( m_errMsg, mysql_error(m_pMysql) );

			//ע�⣺�����������ӣ����Ǳ��α����´ξ��ܳɹ�
			if( 2013 == m_errCode )
			{
				mysql_ping( m_pMysql );

				return DB_FAILURE;
			}
		}

		return DB_SUCCESS;
	}

	nRtn = PrepareSql( szSQL ); /*����sql*/
	if( DB_SUCCESS != nRtn )
	{
		Destory_stmthp();
		return DB_FAILURE;
	}

	if( SQLTYPE_SELECT == m_nSqlType ) /*�ǲ�ѯ����,������������*/
	{
		int nBoutSize = 0;
		STBindParamMap ::iterator itBout = m_mapBindOutParam.find( m_stmthp );
		if( itBout != m_mapBindOutParam.end() )
		{
			nBoutSize = itBout->second.size();
		}

		if( 0 == nBoutSize )
		{
			m_errCode = -1;
			strcpy( m_errMsg, "ִ�в�ѯ��������������" );
			Destory_stmthp();
			return DB_FAILURE;
		}

		nRtn = BindExecSql( 0, 0, pvskip );
		if( DB_SUCCESS != nRtn )
		{
			Destory_stmthp();
			return DB_FAILURE;
		}

		nRtn = mysql_stmt_store_result( m_stmthp );
		if( CheckErr( nRtn ) < 0 )
		{
			Destory_stmthp();
			return DB_FAILURE;
		}

		if( !bCursor )
		{
			nRtn = GetAffectRows();
			if( DB_FAILURE == nRtn )
			{
				Destory_stmthp();
				return DB_FAILURE;
			}
			if( nRtn > 1 )
			{
				m_errCode = -1;
				strcpy( m_errMsg, "��ѯ�����������1" );
				Destory_stmthp();
				return DB_FAILURE;
			}
			else if( 1 == nRtn )
			{
				/*FetchData ����OCI_NO_DATA ��ʾ����û�����ݣ���ǰ�е����ݻ��Ƿ��ڻ�����*/
				int nFetch = FetchData( 1, 0, 0 ); 
				if( DB_FAILURE == nFetch )
				{
					Destory_stmthp();
					return DB_FAILURE;
				}
			}
			else if( 0 == nRtn )
			{
				m_errCode = MYSQL_NO_DATA;
			}
			
		}
	}
	else
	{
		nRtn = BindExecSql( 1, 0 );
		if( DB_SUCCESS != nRtn )
		{
			Destory_stmthp();
			return DB_FAILURE;
		}
		nRtn = GetAffectRows();
		if( DB_FAILURE == nRtn )
		{
			Destory_stmthp();
			return DB_FAILURE;
		}
	}

	if( !bCursor )
		Destory_stmthp();

	return nRtn;
}

int CMysqlSqlca::SqlCommit()
{
	if( !m_bConnect ) 
		return DB_FAILURE;
	
	return mysql_commit( m_pMysql );
}

int CMysqlSqlca::SqlRollback()
{
	if( !m_bConnect ) 
		return DB_FAILURE;

	return mysql_rollback( m_pMysql );
}


int CMysqlSqlca::ClearBindParam()
{
	//����յ���ʱ��key
	STBindParamMap::iterator itBIn = m_mapBindInParam.find( NULL );
	if( itBIn != m_mapBindInParam.end() )
	{
		m_mapBindInParam.erase( itBIn );
	}

	STBindParamMap::iterator itBout = m_mapBindOutParam.find( NULL );
	if( itBout != m_mapBindOutParam.end() )
	{
		m_mapBindOutParam.erase( itBout );
	}


	itBIn = m_mapBindInParam.find( m_stmthp );
	if( itBIn != m_mapBindInParam.end() )
	{
		m_mapBindInParam.erase( itBIn );
	}

	itBout = m_mapBindOutParam.find( m_stmthp );
	if( itBout != m_mapBindOutParam.end() )
	{
		m_mapBindOutParam.erase( itBout );
	}

	return DB_SUCCESS;
}


int CMysqlSqlca:: BindParameter( STBindParamMap &bindParamMap, int nPos, char* format, void *buffer ,int buffer_length )
{
	if( !m_bConnect ) 
		return DB_FAILURE;
	
	/*��ʼ��������*/
	STBindParam stBindParam = {0}; 
	stBindParam.nPos = nPos;
	stBindParam.buffer =(char*)buffer;

	if( 0 == strcmp(format,"%f")  )
	{
		stBindParam.dataType = DATA_TYPE_FLOAT;
		stBindParam.buffer_type = MYSQL_TYPE_DOUBLE;
		stBindParam.buffer_length = sizeof(float);
		if( &bindParamMap == &m_mapBindInParam )
		{
			stBindParam.dvalue = *( (float*)buffer );
			stBindParam.buffer = NULL;
		}
	}
	else if( 0 == strcmp(format,"%lf") )
	{
		stBindParam.dataType = DATA_TYPE_DOUBLE;
		stBindParam.buffer_type = MYSQL_TYPE_DOUBLE;
		stBindParam.buffer_length = sizeof(double);
		if( &bindParamMap == &m_mapBindInParam )
		{
			stBindParam.dvalue = *( (double*)buffer );
			stBindParam.buffer = NULL;
		}
		
	}
	else if( 0 == strcmp(format,"%d") )
	{
		stBindParam.dataType = DATA_TYPE_INT;
		stBindParam.buffer_type = MYSQL_TYPE_LONG;
		stBindParam.buffer_length = sizeof(int);
		if( &bindParamMap == &m_mapBindInParam )
		{
			stBindParam.value = *( (int *)buffer );
			stBindParam.buffer = NULL;
			//printf(" bind int value[%lld] buffer[%p]\n", stBindParam.value, stBindParam.buffer);
		}

	}
	else if( 0 == strcmp(format,"%ld") )
	{
		stBindParam.dataType = DATA_TYPE_LONG;
		stBindParam.buffer_type = MYSQL_TYPE_LONG;
		stBindParam.buffer_length = sizeof(long);
		if( &bindParamMap == &m_mapBindInParam )
		{
			stBindParam.value = *( (int *)buffer );
			stBindParam.buffer = NULL;
			//printf("xulz01 bind int value[%lld] buffer[%p]\n", stBindParam.value, stBindParam.buffer);
		}

	}
	else if( 0 == strcmp(format,"%u") )
	{
		stBindParam.dataType = DATA_TYPE_UINT;
		stBindParam.buffer_type = MYSQL_TYPE_LONG;
		stBindParam.buffer_length = sizeof(unsigned int);
		if( &bindParamMap == &m_mapBindInParam )
		{
			stBindParam.uvalue = *( (unsigned int*)buffer );
			stBindParam.buffer = NULL;
		}

	}
	else if( 0 == strcmp(format,"%lu") )
	{
		stBindParam.dataType = DATA_TYPE_ULONG;
		stBindParam.buffer_type = MYSQL_TYPE_LONG;
		stBindParam.buffer_length = sizeof(unsigned long);
		if( &bindParamMap == &m_mapBindInParam )
		{
			stBindParam.uvalue = *( (unsigned long *)buffer );
			stBindParam.buffer = NULL;
		}
	}
	else if(  0 == strcmp(format,"%lld") )
	{
		stBindParam.dataType = DATA_TYPE_LONGLONG;
		stBindParam.buffer_type = MYSQL_TYPE_LONGLONG;
		stBindParam.buffer_length = sizeof(int64_t);
		if( &bindParamMap == &m_mapBindInParam )
		{
			stBindParam.value = *( (int64_t *)buffer );
			stBindParam.buffer = NULL;
		}
	}
	else if(  0 == strcmp(format,"%llu") )
	{
		stBindParam.dataType = DATA_TYPE_ULONGLONG;
		stBindParam.buffer_type = MYSQL_TYPE_LONGLONG;
		stBindParam.buffer_length = sizeof(uint64_t);
		if( &bindParamMap == &m_mapBindInParam )
		{
			stBindParam.uvalue = *( (uint64_t *)buffer );
			stBindParam.buffer = NULL;
		}
	}
	else if( 0 == strcmp(format,"%s") )
	{
		stBindParam.dataType = DATA_TYPE_STRING;
		stBindParam.buffer_type = MYSQL_TYPE_STRING;
		if( &bindParamMap == &m_mapBindInParam )
			stBindParam.buffer_length = strlen(stBindParam.buffer);
		else
			stBindParam.buffer_length = buffer_length;
	}
	else if( 0 == strcmp(format,"%p") )
	{
		stBindParam.buffer_type = MYSQL_TYPE_STRING;
		stBindParam.buffer_length = buffer_length;
	}
	
	m_stmthp = NULL;
	STBindParamMap::iterator iter = bindParamMap.find( m_stmthp );
	if( iter != bindParamMap.end() )
	{
		STBindParamVec &pvecParam = iter->second ;
		pvecParam.push_back( stBindParam );
	}
	else
	{
		STBindParamVec vecParam;
		vecParam.push_back( stBindParam );
		bindParamMap[m_stmthp] = vecParam;
	}
	
	return DB_SUCCESS;

}

int CMysqlSqlca:: BindIn( int nPos, int nValue )
{
	return BindParameter( m_mapBindInParam, nPos, "%d", &nValue, -1 );
}

int CMysqlSqlca:: BindIn( int nPos, long lValue )
{
	return BindParameter( m_mapBindInParam, nPos, "%ld", &lValue, -1 );
}

int CMysqlSqlca:: BindIn( int nPos, long long llValue )
{
	return BindParameter( m_mapBindInParam, nPos, "%lld", &llValue, -1 );
}

int CMysqlSqlca:: BindIn( int nPos,  unsigned long long lluValue )
{
	return BindParameter( m_mapBindInParam, nPos, "%llu", &lluValue, -1 );
}

int CMysqlSqlca:: BindIn( int nPos, double dValue )
{
	return BindParameter( m_mapBindInParam, nPos, "%lf", &dValue, -1 );
}

int CMysqlSqlca:: BindIn( int nPos, char *strValue )
{
	return BindParameter( m_mapBindInParam, nPos, "%s", strValue, -1 );
}

int CMysqlSqlca:: BindOut( int nPos, char* format, void *buffer, int buffer_length )
{
	return BindParameter( m_mapBindOutParam, nPos, format, buffer, buffer_length );

}

int CMysqlSqlca::BindExecSql( int iters, int mode, int pvskip )
{
	int nRtn = DB_SUCCESS;
	STBindParamVec *pVecBin = NULL;
	STBindParamVec *pVecOut = NULL;
	MYSQL_BIND *pBindIn = NULL;
	MYSQL_BIND	*pBindOut = NULL;
	int 		nSize = 0;

	STBindParamMap ::iterator iter = m_mapBindInParam.find( m_stmthp );
	if( iter != m_mapBindInParam.end() )
	{
		pVecBin = &( iter->second );
	}

	iter = m_mapBindOutParam.find( m_stmthp );
	if ( iter!= m_mapBindOutParam.end() )
	{
		pVecOut = &( iter->second );
	}

	/*------------------------------------���������-----------------------------*/
	
	if( NULL != pVecBin )
		nSize = pVecBin->size();
	else
		nSize = 0;

	if( nSize > 0 )
	{
		 pBindIn = (MYSQL_BIND*)malloc( nSize*sizeof(MYSQL_BIND) );
		 if( NULL== pBindIn )
		 {
			 m_errCode = -1;
			 strcpy( m_errMsg, "�ڴ�������" );

			 return DB_FAILURE;
		 }
		 memset( pBindIn, 0, nSize*sizeof(MYSQL_BIND) );
	}

	int nIndex = 0;
	for( int i = 0; i < nSize; i++ )
	{
		STBindParam &pstBindParam = pVecBin->at( i );
		nIndex = pstBindParam.nPos -1;
		if( pstBindParam.dataType == DATA_TYPE_FLOAT || pstBindParam.dataType == DATA_TYPE_DOUBLE )
		{
			pstBindParam.buffer_type = MYSQL_TYPE_DOUBLE;
			pstBindParam.buffer = (char*)&( pstBindParam.dvalue );
		}
		else if( pstBindParam.dataType == DATA_TYPE_INT || pstBindParam.dataType == DATA_TYPE_LONG || pstBindParam.dataType == DATA_TYPE_LONGLONG )
		{
			pstBindParam.buffer_type = MYSQL_TYPE_LONG;
			pstBindParam.buffer = (char*)&( pstBindParam.value );
		}
		else if( pstBindParam.dataType == DATA_TYPE_UINT || pstBindParam.dataType == DATA_TYPE_ULONG || pstBindParam.dataType == DATA_TYPE_ULONGLONG )
		{
			pstBindParam.buffer_type = MYSQL_TYPE_LONG;
			pstBindParam.buffer = (char*)&( pstBindParam.uvalue );
		}
		else
		{
			pstBindParam.buffer_type = MYSQL_TYPE_STRING;
			//printf("in[%d] value[%s]\n",nIndex, pstBindParam.buffer );
		}
		
		pBindIn[nIndex].buffer_type = pstBindParam.buffer_type;
		pBindIn[nIndex].buffer_length = pstBindParam.buffer_length;
		pBindIn[nIndex].buffer = pstBindParam.buffer ;
		
	//printf("in[%d]: dataType[%d] buffer_type[%d] buffer[%p] buffer_length[%d] \n", \
             	nIndex, pstBindParam.dataType, pBindIn[nIndex].buffer_type, pBindIn[nIndex].buffer,pBindIn[nIndex].buffer_length );  
	}

	/*------------------------------------���������-----------------------------*/

	if( NULL != pVecOut )
		nSize = pVecOut->size();
	else
		nSize = 0;

	if( nSize > 0 )
	{
		pBindOut = (MYSQL_BIND*)malloc( nSize*sizeof(MYSQL_BIND) );
		if( NULL == pBindOut )
		{
			if( pBindIn )  free( pBindIn ); 
			m_errCode = -1;
			strcpy( m_errMsg, "�ڴ�������" );
			return DB_FAILURE;
		}
		memset( pBindOut, 0, nSize*sizeof(MYSQL_BIND) );

	}

	int nDataType = 0;
	for( int i = 0; i < nSize; i++)
	{
		//�����α�--����ֵ
		STBindParam &pstBindParam = pVecOut->at( i );
		pstBindParam.nBatCursorSkip = pvskip;
		pstBindParam.nBatCurRow = 0;
		nIndex = pstBindParam.nPos -1;
		
		pBindOut[nIndex].buffer_type = pstBindParam.buffer_type;
		pBindOut[nIndex].buffer = pstBindParam.buffer;
		pBindOut[nIndex].buffer_length = pstBindParam.buffer_length;
		pBindOut[nIndex].is_null = &pstBindParam.bIsNull;
		
             //printf("out[%d]: buffer_type[%d] buffer[%p] buffer_length[%d] \n", \
             	nIndex, pBindOut[nIndex].buffer_type, pBindOut[nIndex].buffer,pBindOut[nIndex].buffer_length );         

	}

	if( NULL != pBindIn )
	{
		nRtn = mysql_stmt_bind_param( m_stmthp, pBindIn );
		if( CheckErr(nRtn) < 0 )
		{
			if( pBindIn )  free( pBindIn ); 
			if( pBindOut ) free( pBindOut ); 

			return DB_FAILURE;
		}
		
	}

	if( NULL != pBindOut )
	{
		nRtn = mysql_stmt_bind_result( m_stmthp, pBindOut );
		if( CheckErr(nRtn) < 0 )
		{
			if( pBindIn )  free( pBindIn ); 
			if( pBindOut ) free( pBindIn ); 

			return DB_FAILURE;
		}
	
	}

	if( pBindIn )  free( pBindIn ); 
	if( pBindOut ) free( pBindOut ); 

	nRtn = mysql_stmt_execute( m_stmthp );
	if( CheckErr(nRtn) < 0 )
	{
		return DB_FAILURE;
	}
	
	return nRtn;
}

int CMysqlSqlca ::CheckErr( int status )
{
	char errBuf[512] = {0};
	int  errcode = status;

	switch( status )
	{
	case DB_SUCCESS:  /*0*/
		strcpy( errBuf, "" );
		break;

	case MYSQL_NO_DATA:   /*100*/
		strcpy( errBuf, "MYSQL_NO_DATA" );
		break;

	case MYSQL_DATA_TRUNCATED:
		strcpy( errBuf, "���ݱ��ض�" );
		break;

	default:  //1������ 
		{
			errcode = mysql_stmt_errno( m_stmthp );
			strcpy( errBuf, mysql_stmt_error( m_stmthp ) );
			errBuf[511]='\0';
			if( DB_SUCCESS != errcode )
				errcode = DB_FAILURE;
		
		}
		break;

	}

	m_errCode = errcode;
	strcpy( m_errMsg, errBuf );

	/*1405��Ϊ��ֵʱ���Ǵ���,1403 �Ҳ�������*/
	if( DB_SUCCESS != status && MYSQL_NO_DATA != status )
	{
		return DB_FAILURE;
	} 

	return DB_SUCCESS;
}


/*-------------------------------------------------------------------------------
����˵����      ��ȡ��ǰִ�е������ 
����ֵ��        void--�����
				NULL--ʧ��
---------------------------------------------------------------------------------*/
void* CMysqlSqlca::GetCurStmtHandle()
{
	if( NULL != m_stmthp )
		return m_stmthp;
	else
		return NULL;
}


/*-------------------------------------------------------------------------------
����˵����      ��������� 
���������		pStmthandle�����

����ֵ��        ��
---------------------------------------------------------------------------------*/
void CMysqlSqlca::SetCurStmtHandle( void* pStmthandle )
{
	if( NULL != pStmthandle )
		m_stmthp = (MYSQL_STMT*)pStmthandle;
}

int CMysqlSqlca:: FetchData( int nFetchRows, int nOrientation, int nStartRow, void* pStmthandle )
{
	int status = 0;
	int nBatCurRow = 0;
	
	if( !m_bConnect ) return DB_FAILURE;
	
	//������� ʹ�����α���ִ�ж��sql���
	if( NULL != pStmthandle )
		m_stmthp = (MYSQL_STMT*)pStmthandle;

	if( nFetchRows <= 0 )/*Ĭ��ȡһ��*/
	{
		nFetchRows = 1;
	}

	int  nOptType = 0; //�������� 0--���м���,1--�����α� 2--����ṹ��ѯ
	STBindParamVec *pVecBout = NULL;

	if( NULL != m_pResult )
	{
		nOptType = 2;
	}
	else
	{
		STBindParamMap::iterator itBout = m_mapBindOutParam.find( m_stmthp );
		if( itBout != m_mapBindOutParam.end() )
		{
			pVecBout = &(itBout->second);
			if( itBout->second[0].nBatCursorSkip > 0 )
				nOptType = 1;
		}

	}

	if( NULL != pVecBout )
	{
		if( pVecBout->size() && pVecBout->at(0).nBatCursorSkip > 0 )
			nOptType = 1;
	}

	if( 0 == nOptType ) //����ṹ�� ��������
	{
		status = mysql_stmt_fetch( m_stmthp );
		if( CheckErr(status) < 0 )
		{
			return DB_FAILURE;
		}
		//�ַ�������ͳһȥ�ո���(����ṹͨ���󶨱����������)
		if( 0 == status )
		{
			int nSize = pVecBout->size();
			for( int i = 0; i < nSize; i++ )
			{
				STBindParam &pstBindParam = pVecBout->at( i );
				if( pstBindParam.buffer_type == MYSQL_TYPE_LONG && pstBindParam.bIsNull )
				{
					int *pInt = (int*)pstBindParam.buffer;
					*pInt = 0;
				}
				else if( pstBindParam.buffer_type == MYSQL_TYPE_LONGLONG && pstBindParam.bIsNull )
				{
					long long *pInt = (long long*)pstBindParam.buffer;
					*pInt = 0;
				}
				else if( pstBindParam.buffer_type == MYSQL_TYPE_DOUBLE && pstBindParam.bIsNull )
				{
					double *pDouble = (double*)pstBindParam.buffer;
					*pDouble = 0;
				}
				else if( pstBindParam.buffer_type == MYSQL_TYPE_STRING )
				{	
					if( pstBindParam.bIsNull )
					{
						pstBindParam.buffer[0] = '\0';
					}
					else
					{
						AllTrim( pstBindParam.buffer );
						strReplace( pstBindParam.buffer, "\\", '/' );
					}

				}

			}
		}
	
	}
	else
	{
		int nColNum = GetColNum();
		MYSQL_BIND *pBindOut = (MYSQL_BIND*)malloc( nColNum*sizeof(MYSQL_BIND) );
		if( NULL == pBindOut )
		{
			m_errCode = -1;
			sprintf( m_errMsg, "�ڴ������� nColNum=%d,sizeof=%d \n",nColNum,sizeof(MYSQL_BIND) );
			strcpy( m_errMsg, "�ڴ�������" );
			return DB_FAILURE;
		}
		memset( pBindOut, 0, nColNum*sizeof(MYSQL_BIND) );

		int nBatCursorSkip = 0;
		char *pFirstArray = NULL;
		char *pFirstData = NULL;

		for( int j = 0; j < nColNum ; j++)
		{
			//�����α�
			if( 1 == nOptType)
			{
				STBindParam &pstBindParam = pVecBout->at( j );
				int nIndex = pstBindParam.nPos -1;
				if( 0 == nIndex )  
				{
					pFirstArray = pstBindParam.buffer;
					nBatCursorSkip = pstBindParam.nBatCursorSkip;
					pFirstData = (char*)malloc( nBatCursorSkip );
					if( NULL == pFirstData )
					{
						m_errCode = -1 ;
						strcpy( m_errMsg, "�ڴ�������" );
						if( pBindOut ) free( pBindOut );			
						return DB_FAILURE;
					}
					memset( pFirstData, 0, sizeof(nBatCursorSkip) );
				}
			
				pBindOut[nIndex].buffer_type = pstBindParam.buffer_type;
				pBindOut[nIndex].buffer = pstBindParam.buffer;//+i*pstBindParam.nBatCursorSkip;
				pBindOut[nIndex].buffer_length = pstBindParam.buffer_length;
				pBindOut[nIndex].is_null = &pstBindParam.bIsNull;

			}
			else
			{
				STColPropty &stColPropty = m_stSqlPropty.vecColPropty[j];	
				if( 0 == j )  
				{
					pFirstArray = m_pResult;
					nBatCursorSkip = m_stSqlPropty.nTotalColSize;
					pFirstData = (char*)malloc( nBatCursorSkip );
					if( NULL == pFirstData )
					{
						m_errCode = -1;
						strcpy( m_errMsg, "�ڴ�������" );
						if( pBindOut ) free( pBindOut );	
						return DB_FAILURE;
					}
					memset( pFirstData, 0, nBatCursorSkip );
				}
				
				pBindOut[j].buffer_type = MYSQL_TYPE_VAR_STRING;
				pBindOut[j].buffer = m_pResult+m_stSqlPropty.vecColPropty[j].nColOffset;;
				pBindOut[j].buffer_length = stColPropty.nStrLen;

			}

			status = mysql_stmt_bind_result( m_stmthp, pBindOut );
			if( CheckErr(status) < 0 )
			{
				if( pBindOut ) free( pBindOut );
				if( pFirstData ) free( pFirstData );

				return DB_FAILURE;
			}
		}
		
		
		for( int i = 0; i < nFetchRows; i++)
		{
		
			status = mysql_stmt_fetch( m_stmthp );
			if( CheckErr(status) < 0 )
			{
				if( pBindOut ) free( pBindOut );
				if( pFirstData ) free( pFirstData );

				return DB_FAILURE;
			}

			if( 1 == nOptType && MYSQL_NO_DATA!=status )
			{
				nBatCurRow = ++( pVecBout->at(0).nBatCurRow ); //���浱ǰ���α�λ��
			}

			//�����һ������
			if( 0 == i )  
			{
				memcpy( pFirstData, pFirstArray, nBatCursorSkip );
			}
			else
			{
				memcpy( pFirstArray+i*nBatCursorSkip, pFirstArray, nBatCursorSkip );
			}
			

		}
		//��ԭ��һ������
		memcpy( pFirstArray, pFirstData, nBatCursorSkip );
		if( pBindOut ) free( pBindOut );
		if( pFirstData ) free( pFirstData );
	}
	
	if( MYSQL_NO_DATA == status )
	{
		m_errCode = status;
	}

	return nBatCurRow;

}

char* CMysqlSqlca::ConverToMySqlStr(char *src)
{
	char*p =(char*)src;
	while (*p ) 
	{

		if( *p ==':' && *(p+1) =='b' )
		{
			*p = '?';
			while( *(++p) )
			{
				if( *p == ' ' || *p == '=' || *p == ',' || *p == ')' )
				{
					break;
				}
				*p = ' ';
			}
		}

		if( *p )
			p++; 
	}

	return src;
}

int CMysqlSqlca::Disconnect()
{
	Destory_stmthp();
	if( m_pMysql )
	{
		mysql_close( m_pMysql );
		m_pMysql = NULL;
	}
	
	return DB_SUCCESS;
}

time_t CMysqlSqlca::GetLastUseTime()
{
	return m_tmLastUseTime;
}

int CMysqlSqlca::DestroyFreeMem()
{
	Destory_stmthp(); /*�ͷ������*/

	return DB_SUCCESS;

}

int CMysqlSqlca::ExecuteArraySql( char *szSQL, int nCount, int pvskip )
{
	int nRtn = DB_SUCCESS;
	STBindParamVec *pVecBin = NULL;
	MYSQL_BIND *pBindIn = NULL;
	int 		nSize = 0;

	m_nSqlType = GetSQLType( szSQL );
	//����insert
	if( SQLTYPE_INSERT == m_nSqlType )
	{
		return InsertArraySql( szSQL, nCount, pvskip );
	}
	
	nRtn = PrepareSql( szSQL); /*����sql*/
	if( DB_SUCCESS != nRtn )
	{
		Destory_stmthp();
		return DB_FAILURE;
	}

	char *pFirstData= (char*)malloc( pvskip );
	if( NULL == pFirstData)
	{
		m_errCode = -1;
		strcpy( m_errMsg, "�ڴ�������" );
		Destory_stmthp();
		return DB_FAILURE;
	}
	memset( pFirstData, 0, pvskip );

	/*------------------------------------���������-----------------------------*/
	STBindParamMap ::iterator itBin = m_mapBindInParam.find( m_stmthp );
	if( itBin != m_mapBindInParam.end() )
	{
		pVecBin = &(itBin->second);
	}

	if( NULL != pVecBin )
		nSize = pVecBin->size();
	else
		nSize = 0;

	if( nSize > 0 )
	{
		pBindIn = (MYSQL_BIND*)malloc( nSize*sizeof(MYSQL_BIND) );
		if( NULL == pBindIn )
		{
			if( pFirstData )  free( pFirstData );
			m_errCode = -1;
			strcpy( m_errMsg, "�ڴ�������" );
			Destory_stmthp();
			return DB_FAILURE;
		}
		memset( pBindIn, 0, nSize*sizeof(MYSQL_BIND) );
	}

	int nIndex = 0;
	int nDataType = 0;
	char *pFirstArray = NULL;

	for( int j = 0; j < nSize; j++ )
	{
		STBindParam &pstBindParam = pVecBin->at(j);
		nIndex = pstBindParam.nPos -1;
		if( 0 == nIndex )  pFirstArray = pstBindParam.buffer;
		
		pBindIn[nIndex].buffer_type = pstBindParam.buffer_type ;
		pBindIn[nIndex].buffer = pstBindParam.buffer;
		if( pstBindParam.buffer_type = MYSQL_TYPE_STRING )
			pBindIn[nIndex].buffer_length = strlen( pstBindParam.buffer );

		nRtn = mysql_stmt_bind_param( m_stmthp, pBindIn );
		if( CheckErr(nRtn) < 0 )
		{
			if( pFirstData )  free( pFirstData );
			if( pBindIn )  free( pBindIn );
			Destory_stmthp();

			return DB_FAILURE;
		}


	}

	//���������һ������
	memcpy( pFirstData, pFirstArray, pvskip );

	for( int i = 0; i < nCount; i++ )
	{

		if( i > 0 )  
		{
			memcpy( pFirstArray, pFirstArray+i*pvskip, pvskip );
			//���Ϊ�ַ������¼��㳤��
			for( int j = 0; j < nSize; j++)
			{
				if( pBindIn[j].buffer_type == MYSQL_TYPE_STRING )
				{
					pBindIn[j].buffer_length = strlen( (char*)pBindIn[j].buffer );
				}	
			}

		}

		
	//	WriteLevLog(LVL_DEBUG,"begin mysql_stmt_execute");
		nRtn = mysql_stmt_execute( m_stmthp );
		if( CheckErr(nRtn) < 0 )
		{
			if( pFirstData )  free( pFirstData );
			if( pBindIn )  free( pBindIn );
			Destory_stmthp();

			return DB_FAILURE;
		}
	//	WriteLevLog(LVL_DEBUG,"end mysql_stmt_execute");
	}
	//��ԭ��һ������
	memcpy( pFirstArray, pFirstData, pvskip );
	if( pFirstData )  free( pFirstData );
	if( pBindIn )  free( pBindIn );
	Destory_stmthp();

	return DB_SUCCESS;
}
	

int CMysqlSqlca::InsertArraySql( char *szSQL, int nCount, int pvskip )
{
	int nRtn = 0;
	const int BATINSERT = 100;
	int nDivNum = nCount/BATINSERT;
	int nResidual = nCount%BATINSERT;
	
	if( nCount <= BATINSERT )
	{
		nRtn = BatInsertSql( szSQL, nCount, pvskip, true );
	}	
	else 
	{
		char *pFirstArray = NULL;
		char *pLastArray = NULL;
		STBindParamMap ::iterator itBin = m_mapBindInParam.find( m_stmthp );
		if( itBin != m_mapBindInParam.end() )
		{
			pFirstArray = itBin->second[0].buffer;
			
		}

		char *pFirstData = (char*)malloc( BATINSERT*pvskip );
		if( NULL == pFirstData )
		{
			m_errCode = -1;
			strcpy( m_errMsg, "�ڴ�������" );
			Destory_stmthp();

			return DB_FAILURE;
		}
		//����ǰn������
		memcpy( pFirstData, pFirstArray, BATINSERT*pvskip );

		for( int i = 0; i < nDivNum; i++ )
		{
			if( 0 == i ) 
			{
				pLastArray = pFirstArray;
				nRtn=BatInsertSql( szSQL, BATINSERT, pvskip, true );
			}
			else
			{
				pLastArray = pFirstArray+i*BATINSERT*pvskip;
				memcpy( pFirstArray, pLastArray, BATINSERT*pvskip );
				nRtn = BatInsertSql( szSQL, BATINSERT, pvskip, false );
			}
			if( nRtn < 0 )
			{
				Destory_stmthp();
				if( pFirstData ) free( pFirstData );

				return DB_FAILURE;
			}
		}

		

		if( nResidual > 0 )
		{
			//�����������
			memcpy( pFirstArray, pLastArray+BATINSERT*pvskip, nResidual*pvskip );
			nRtn = BatInsertSql( szSQL,nResidual,pvskip,true );
			if( nRtn < 0 )
			{
				Destory_stmthp();
				if( pFirstData ) free( pFirstData );

				return DB_FAILURE;
			}
		}
		
		//��ԭǰn������
		memcpy( pFirstArray, pFirstData, BATINSERT*pvskip );
		if( pFirstData ) free( pFirstData );
		
	}
	Destory_stmthp();
	
	return DB_SUCCESS;
}

int CMysqlSqlca::BatInsertSql( char *szSQL, int nCount, int pvskip, bool bPrepare )
{
	int nRtn = 0;
	MYSQL_BIND *pBindIn = NULL;
	if( !m_bConnect ) return DB_FAILURE;

	if( bPrepare )
	{
		STBindParamVec VecBin;
		int nSize = 0;
		/*------------------------------------���������-----------------------------*/
		STBindParamMap ::iterator itBin = m_mapBindInParam.find( m_stmthp );
		if( itBin != m_mapBindInParam.end() )
		{
			VecBin = itBin->second;
		}

		Destory_stmthp();
		//���°�
		m_mapBindInParam[NULL] = VecBin;

		string sInsert = szSQL;
		//ConverToMySqlStr( (char*)sInsert.c_str() );
		//insert
		int nPos = sInsert.find_first_of("VALUES");
		if( nPos <= 0 ) nPos= sInsert.find_first_of( "values");
		nPos = nPos + 5;
		string sBegin = sInsert.substr( 0, nPos + 1 );
		string sEnd = sInsert.substr( nPos+1, sInsert.length()-nPos-1 );
		//����insert �޸�sql���
		for (int i=0; i < nCount; i++)
		{
			if( i > 0 )
				sBegin += string(",") + sEnd;
			else
				sBegin += sEnd;
		}
		sInsert = sBegin;

		nRtn = PrepareSql( (char*)sInsert.c_str() ); /*����sql*/
		if( DB_SUCCESS != nRtn )
		{
			Destory_stmthp();
			return DB_FAILURE;
		}

		nSize = VecBin.size();
		pBindIn = (MYSQL_BIND*)malloc( nSize*nCount*sizeof(MYSQL_BIND) );
		if( NULL == pBindIn )
		{
			m_errCode = -1;
			strcpy( m_errMsg, "�ڴ�������" );

			return DB_FAILURE;
		}
		memset( pBindIn, 0, nSize*nCount*sizeof(MYSQL_BIND) );

		int nIndex = 0;
		int nDataType = 0;
		for( int i = 0; i < nCount; i++ )
		{
			for( int j = 0; j < nSize; j++ )
			{
				STBindParam &pstBindParam = VecBin[j];
				pBindIn[nIndex].buffer_type = pstBindParam.buffer_type;
				pBindIn[nIndex].buffer = pstBindParam.buffer + i*pvskip;
				if( pstBindParam.buffer_type = MYSQL_TYPE_STRING )
					pBindIn[nIndex].buffer_length = strlen( (char*)pBindIn[nIndex].buffer );

				nIndex++;

			}

		}

		nRtn = mysql_stmt_bind_param( m_stmthp, pBindIn );
		if( CheckErr(nRtn) < 0 )
		{
			if( pBindIn )  free( pBindIn );
			return DB_FAILURE;
		}
	}

	nRtn = mysql_stmt_execute( m_stmthp );
	if( CheckErr(nRtn) < 0 )
	{
		if( pBindIn )  free( pBindIn );
		return DB_FAILURE;
	}

	if( pBindIn )  free( pBindIn );

	return DB_SUCCESS;
}

int CMysqlSqlca::QuerySql( char* szSQL, int nStartRow, int nFetchRows )
{
	int nRtn = 0;
	unsigned int i = 0;

	if( !m_bConnect ) return DB_FAILURE;

	if( nStartRow <= 0 ) nStartRow = 1;
	if( nFetchRows <= 0) nFetchRows = 1;

	if( nFetchRows >DB_MAX_RECORD_ROW )
	{
		m_errCode = -1;
		sprintf( m_errMsg, "��ѯ�������������:%d", DB_MAX_RECORD_ROW);

		return DB_FAILURE;
	}

	char *pExeSQL = NULL;
	/*��ҳ��ѯ����дszSQL*/
	char sTemp1[64] = {0};
	if( nStartRow > 1 )
		sprintf( sTemp1," LIMIT %d,%d ",nStartRow-1, nFetchRows );
	else
		sprintf( sTemp1," LIMIT %d ",nFetchRows );
	
	int nStrLen = strlen(sTemp1) + strlen(szSQL) + 1;
	pExeSQL = (char*)malloc( nStrLen );
	if( NULL == pExeSQL )
	{
		m_errCode = -1;
		strcpy( m_errMsg, "�ڴ�������" );
		return DB_FAILURE;
	}
	memset( pExeSQL, 0, nStrLen );
	snprintf( pExeSQL, nStrLen -1, "%s%s", szSQL, sTemp1 );
	nRtn = PrepareSql( pExeSQL ); /*����sql*/
	free( pExeSQL );

	if( DB_SUCCESS != nRtn ) 
	{
		Destory_stmthp();
		return DB_FAILURE;
	}

	nRtn = BindExecSql( 0, 0 );
	if( DB_SUCCESS != nRtn )
	{
		Destory_stmthp();
		return DB_FAILURE;
	}
	nRtn = mysql_stmt_store_result( m_stmthp );
	if( CheckErr(nRtn) < 0 )
	{
		Destory_stmthp();
		return DB_FAILURE;
	}

	m_nSqlType = SQLTYPE_SELECT;
	nFetchRows = GetAffectRows();  //��ȡ������ʵ������������oci�в���
	//��̬�����ڴ淽ʽ
	nRtn = SetOutputParam( nFetchRows );
	if( DB_SUCCESS != nRtn)
	{
		Destory_stmthp();
		return DB_FAILURE;
	}
	
	///*FetchData ����OCI_NO_DATA ��ʾ����û�����ݣ���ǰ�е����ݻ��Ƿ��ڻ�����*/
	nRtn = FetchData( nFetchRows, 0, 0 ); 
	if( DB_FAILURE == nRtn )
	{
		Destory_stmthp();
		return DB_FAILURE;
	}

	ClearBindParam();

	return nFetchRows;
}

MYSQL_FIELD* CMysqlSqlca:: GetColDesHandle()
{
	MYSQL_FIELD *fd = NULL;

	if( NULL == m_pMysqlRes )
	{
		m_pMysqlRes = mysql_stmt_result_metadata( m_stmthp );
	}

	fd = mysql_fetch_field( m_pMysqlRes );

	return fd;
}

int CMysqlSqlca:: GetColNum()
{
	int nColNmum = 0;

	if( NULL!= m_stmthp )
	{
		nColNmum = mysql_stmt_field_count( m_stmthp );
	}
	
	return nColNmum;
}

int CMysqlSqlca:: GetColType( MYSQL_FIELD *colhd )
{
	int nColType = 0;
	if( NULL != colhd )
	{
		nColType = (int)colhd->type;
	}

	if( nColType == MYSQL_TYPE_NEWDECIMAL )
	{
		nColType = DBTYPE_DOUBLE;
	}
	else if( nColType == MYSQL_TYPE_VAR_STRING )
	{
		nColType = DBTYPE_STRING;
	}
	
	return nColType;
}

int CMysqlSqlca:: GetColSize( MYSQL_FIELD *colhd )
{
	int nLen = 0;

	if( NULL != colhd )
	{
		nLen = colhd->length;
		if( colhd->type == MYSQL_TYPE_VAR_STRING || colhd->type == MYSQL_TYPE_STRING )
		{
			if( colhd->charsetnr == 28 )
				nLen = nLen/2;
			else if( colhd->charsetnr == 33 )
				nLen = nLen/3;
		}
		else if( colhd->type == MYSQL_TYPE_NEWDECIMAL)
		{
			nLen = nLen - 2;
		}

	}

	return nLen;
}

int CMysqlSqlca::GetColScale( MYSQL_FIELD *colhd )
{
	int nColScale = 0;
	if( NULL!= colhd )
	{
		if( colhd->type == MYSQL_TYPE_NEWDECIMAL )
			nColScale = colhd->decimals;
	}

	return nColScale;
}

int CMysqlSqlca::GetColName( MYSQL_FIELD *colhd, char *sColName )
{
	if( NULL!= colhd )
	{
		strcpy( sColName, colhd->name );
		UpperStr( sColName );
	}
	else
		sColName[0] = '\0';

	return DB_SUCCESS;
}

int CMysqlSqlca::SetColPropty()
{
	if( m_stSqlPropty.vecColPropty.empty() )
	{
		//��д���е�����ֵ
		m_stSqlPropty.nColNum = GetColNum();
		for( int i = 0; i < m_stSqlPropty.nColNum; i++ )
		{
			STColPropty stColPropty = {0};	
			int iColIndex = i+1;
			MYSQL_FIELD *colhd = GetColDesHandle();

			stColPropty.nColType = GetColType( colhd );
			stColPropty.nColLen = GetColSize( colhd );
			GetColName( colhd,stColPropty.sColName );
			stColPropty.nColScale = GetColScale( colhd );

			//int nColLen =0;
			if( DBTYPE_DOUBLE == stColPropty.nColType )
			{
				stColPropty.nStrLen = 22;
			}
			else
			{
				stColPropty.nStrLen = stColPropty.nColLen + 1;
			}

			//��¼�ܵ��������Ⱥ��еĳ���
			m_stSqlPropty.nTotalColNameSize += strlen( stColPropty.sColName );
			//��¼ָ��ƫ����
			stColPropty.nColOffset = m_stSqlPropty.nTotalColSize;
			m_stSqlPropty.nTotalColSize += stColPropty.nStrLen; 		
			m_stSqlPropty.vecColPropty.push_back( stColPropty );
		}
		//m_stSqlPropty.vecColPropty.resize(m_stSqlPropty.vecColPropty.size());
	}

	return DB_SUCCESS;
}



int CMysqlSqlca:: SetOutputParam( int nFetchRows )

{  
	int status = 0;

	status = SetColPropty();
	if( status < 0 )
	{
		return DB_FAILURE;
	}

	int nSize = nFetchRows*m_stSqlPropty.nTotalColSize;
	m_pResult = (char*)malloc( nSize );
	if( NULL == m_pResult )
	{
		strcpy( m_errMsg, "�ڴ�������" );
		m_errCode = -1;

		return DB_FAILURE;
	}
	memset( m_pResult, 0, nSize );

	return DB_SUCCESS;

}

 my_ulonglong CMysqlSqlca::GetLastInsertId()
 {
	 my_ulonglong nInsertId = 0;
	 if( NULL != m_stmthp )
	 {
		nInsertId = mysql_stmt_insert_id( m_stmthp );
		//  nInsertId = mysql_insert_id( m_pMysql );
	 }
	 
	 return nInsertId;
 }


 int  CMysqlSqlca:: GetFieldValue( int rowIndex, int colIndex, char* pStrValue,int nStrLen )
 {
	 if(rowIndex < 1 ||colIndex < 1 || !m_bConnect)
	 {
		 return DB_FAILURE;
	 }

	 int nColNum = m_stSqlPropty.vecColPropty.size();
	 if (colIndex > nColNum )
	 {
		 m_errCode = -1;
		 sprintf( m_errMsg, "��ȡ��%d�е�ֵʧ��,�������ݿ�ʵ�ʵ��е�����:%d", colIndex, nColNum );

		 return DB_FAILURE;
	 }

	 colIndex = colIndex-1;
	 rowIndex = rowIndex -1;

	 int nColOffset = m_stSqlPropty.vecColPropty[colIndex].nColOffset;
	 int nColLen = m_stSqlPropty.vecColPropty[colIndex].nStrLen;

	 char* sTempValue = m_pResult+nColOffset+( rowIndex*m_stSqlPropty.nTotalColSize );
	 sTempValue[nColLen-1] = '\0';
	 //���������ṹ��ͳһ�����ո�������ṹͳһ���ո���
	 AllTrim( sTempValue );
	 strReplace( sTempValue, "\\", '/' ); 
	 strcpy( pStrValue, sTempValue );

	 return DB_SUCCESS;
 }


 
/*-------------------------------------------------------------------------------
����˵����      ȡ��ĳ�е�ֵ,�к�ͨ��fetchdataѭ��ȡ��FetchData()
                �ӿ����ʹ��

���������      rowIndex------�кŴ�1��ʼ
				colIndex------�кŴ�1��ʼ
				
���������		pDvalue----����double���͵�ֵ
				
����ֵ��        DB_SUCCESS--�ɹ�
				DB_FAILURE--ʧ��
---------------------------------------------------------------------------------*/
int CMysqlSqlca::GetFieldValue( int rowIndex, int colIndex, double *pDvalue )
{
	if( rowIndex < 1 ||colIndex < 1 || !m_bConnect )
	{
		return DB_FAILURE;
	}
	colIndex = colIndex - 1;
	rowIndex = rowIndex - 1;

	int nColOffset = m_stSqlPropty.vecColPropty[colIndex].nColOffset;
	int nColLen = m_stSqlPropty.vecColPropty[colIndex].nStrLen;

	char* sTempValue = m_pResult+nColOffset+(rowIndex*m_stSqlPropty.nTotalColSize);
	sTempValue[nColLen-1] = '\0';
	*pDvalue= atof(sTempValue);

	return DB_SUCCESS;
}

/*-------------------------------------------------------------------------------
����˵����      ȡ��ĳ�е�ֵ,�к�ͨ��fetchdataѭ��ȡ��FetchData()
                �ӿ����ʹ��

���������      rowIndex------�кŴ�1��ʼ
				colIndex------�кŴ�1��ʼ
				
���������		pLvalue----����int���͵�ֵ
				
����ֵ��        DB_SUCCESS--�ɹ�
				DB_FAILURE--ʧ��
---------------------------------------------------------------------------------*/
int CMysqlSqlca::GetFieldValue( int rowIndex, int colIndex, int *pLvalue )
{
	if( rowIndex < 1 || colIndex < 1 || !m_bConnect )
	{
		return DB_FAILURE;
	}
	colIndex = colIndex - 1;
	rowIndex = rowIndex - 1;

	int nColOffset = m_stSqlPropty.vecColPropty[colIndex].nColOffset;
	int nColLen = m_stSqlPropty.vecColPropty[colIndex].nStrLen;

	char* sTempValue = m_pResult+nColOffset + ( rowIndex*m_stSqlPropty.nTotalColSize );
	sTempValue[nColLen-1]='\0';
	*pLvalue = atol(sTempValue);

	return DB_SUCCESS;
}

int   CMysqlSqlca::Execute(int iters,int mode)
{
	int nRtn = mysql_stmt_execute( m_stmthp );
	if( CheckErr(nRtn) < 0 )
	{
		return DB_FAILURE;
	}

	return DB_SUCCESS;
}
