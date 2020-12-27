/*
 * author	: jack 
 * email	: xulongzhong2004@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include <openssl/ssl.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include "uhp_in.h"

#define		MAX_SOURCEIP_LEN	30
/*tbmessage����״̬*/
#define TBMESAGE_STATUS_INIT		0
#define TBMESAGE_STATUS_PUBLISH		1

#define SET_ERROR_RESPONSE( p_session , errcode, errmsg ) \
	if( errcode == 0 || errcode == HTTP_OK ) \
	{ \
		sprintf( p_session->http_rsp_body, "{\"errorCode\":\"%d\",", 0 ); \
	} \
	else \
	{ \
		sprintf( p_session->http_rsp_body, "{\"errorCode\":\"%d\",", errcode); \
	} \
	if( errmsg != NULL ) \
	{ \
		strcat( p_session->http_rsp_body , "\"errorMessage\":\""); \
		strcat( p_session->http_rsp_body ,errmsg); \
		strcat( p_session->http_rsp_body ,"\""); \
	}\
	else \
	{ \
		strcat( p_session->http_rsp_body , "\"errorMessage\": null"); \
	} \
	strcat( p_session->http_rsp_body, "}" );
	

static char* ToUpperStr( char *str )
{
	size_t	i;
	size_t	len;
	
	len = strlen( str );
	for( i = 0; i < len; i++ )
	{
		str[i] = toupper( str[i] );
	}
	
	return str;
}

static int NSPIconv( char* fromcode, char* tocode, char *in, char *out, size_t outSize )
{
	int 	nret;
	iconv_t conv;
	size_t inLen;
	
	if( !fromcode || !tocode || !in || !out )
		return -1;
	
	inLen = strlen( in );
	if( strcmp( fromcode, tocode ) == 0 )
	{
		if( outSize <= inLen )
		{
			ERRORLOGSG( "outSize too small");
			return -1;
		}
		strncpy( out, in, inLen );
		
		return 0;
	}
	
	conv = iconv_open( tocode, fromcode );
	if( conv < 0 )
	{
		ERRORLOGSG( "iconv_open failed  errno[%d]", errno );
		return -1;
	}
	
	DEBUGLOGSG( "iconv in[%s] out[%s] fromcode[%s] tocode[%s] errno[%d]", in, out , fromcode, tocode, errno );
	nret = iconv( conv, &in, &inLen, &out, &outSize );
	if( nret < 0 )
	{
		ERRORLOGSG( "iconv failed in[%s] out[%s] fromcode[%s] tocode[%s] errno[%d]", in, out , fromcode, tocode, errno );
		iconv_close( conv );
		return -1;
	}
	
	iconv_close( conv );
	
	return 0;
}

int ConvertReponseBody( struct AcceptedSession *p_accepted_session, char* body_convert, int body_size )
{
	int 	nret;
	char	charset_upper[10+1];
	
	/*�ж���ת��Ϊ��д�����ǲ��ı�ԭֵ*/
	memset( charset_upper, 0, sizeof(charset_upper) ); 
	strncpy( charset_upper, p_accepted_session->charset, sizeof(charset_upper)-1 );
	ToUpperStr( charset_upper );

	nret = NSPIconv("GB18030", charset_upper, p_accepted_session->http_rsp_body, body_convert, body_size );
	if( nret )
	{
		ERRORLOGSG( "NSPIconv failed[%d] charset[%s] http_rsp_body[%s]" , nret, p_accepted_session->charset, p_accepted_session->http_rsp_body );
		SET_ERROR_RESPONSE( p_accepted_session, HTTP_NOT_ACCEPTABLE, HTTP_NOT_ACCEPTABLE_T );
		return -1;
	}
	
	return 0;
}


int ThreadBegin( void *arg, int threadno )
{
	threadinfo_t		*p_threadinfo = (threadinfo_t*)arg;
	struct HttpserverEnv 	*p_env = NULL;
	char			module_name[50];
	
	ResetAllHttpStatus();
	p_env = (struct HttpserverEnv*)threadpool_getReserver1( p_threadinfo->p_pool );
	if( !p_env )
		return -1;
	
	memset( module_name, 0, sizeof(module_name) );
	snprintf( module_name, 15,"Thread_%d", threadno ); 
	prctl( PR_SET_NAME, module_name );
	
	memset( module_name, 0, sizeof(module_name) );
	snprintf( module_name, sizeof(module_name) - 1, "worker_%d", g_process_index+1 );
	/* ������־���� */
	InitLogEnv( p_env, module_name, SERVER_AFTER_LOADCONFIG );
	INFOLOGSG(" InitLogEnv module_name[%s] threadno[%d] ok", module_name, threadno );
	
	return 0;
}

int ThreadRunning( void *arg, int threadno )
{
	threadinfo_t		*p_threadinfo = (threadinfo_t*)arg;
	struct HttpserverEnv 	*p_env = NULL;
	
	p_env = (struct HttpserverEnv*)threadpool_getReserver1( p_threadinfo->p_pool );
	if( !p_env )
		return -1;
	
	if( p_threadinfo->cmd == 'L' ) /*�߳���Ӧ�ó�ʼ��*/
	{
		char	module_name[50];

		memset( module_name, 0, sizeof(module_name) );
		snprintf( module_name, 15,"Thread_%d", threadno ); 
		prctl( PR_SET_NAME, module_name );
		
		memset( module_name, 0, sizeof(module_name) );
		snprintf( module_name, sizeof(module_name) - 1, "worker_%d", g_process_index+1 );
		/* ������־���� */
		InitLogEnv( p_env, module_name, SERVER_AFTER_LOADCONFIG );
		INFOLOGSG(" reload config module_name[%s] threadno[%d] ok", module_name, threadno );
		
		p_threadinfo->cmd = 0;
	}
	
	/*����ʱ�������־*/
	if( p_threadinfo->status == THREADPOOL_THREAD_WAITED )
	{
		struct timeval	now_time;
		
		gettimeofday( &now_time, NULL );
		if( ( now_time.tv_sec - p_env->last_show_status_timestamp ) >= p_env->httpserver_conf.httpserver.server.showStatusInterval )
		{
			INFOLOGSG(" Thread Is Idling  threadno[%d] ", threadno );
		}
	}

	return 0;
}

int ThreadExit( void *arg, int threadno )
{
	threadinfo_t	*p_threadinfo = (threadinfo_t*)arg;
	struct HttpserverEnv *p_env = NULL;
	
	p_env = (struct HttpserverEnv*)threadpool_getReserver1( p_threadinfo->p_pool );
	if( !p_env )
		return -1;
	
	cleanLogEnv();
	p_threadinfo->cmd = 0;
	
	INFOLOGSG("Thread Exit threadno[%d] ", threadno );

	return 0;
}

int ThreadWorker( void *arg, int threadno )
{
	long 			cost_time;
	BOOL			bError = FALSE;
	struct timeval		*ptv_start = NULL; 
	struct timeval		*ptv_end = NULL;
	char			body_convert[MAX_RESPONSE_BODY_LEN];
	
	struct AcceptedSession *p_accepted_session = (struct AcceptedSession*)arg;
	struct HttpserverEnv 	*p_env = (struct HttpserverEnv*)p_accepted_session->p_env;
	
	uint			connection_value;
	int 			status_code;
	int 			nret ;
	
	/*��շ�����Ӧ�壬��ֹ���������������ϴ���Ӧ����*/
	memset( p_accepted_session->http_rsp_body, 0, p_accepted_session->body_len );
	ptv_start = &( p_accepted_session->perfms.tv_publish_begin );
	gettimeofday( ptv_start, NULL );
	
	//��������������ҵ��
	INFOLOGSG("begin doworker_output[%p] threadno[%d]",p_env->p_fn_doworker_output, threadno );
	nret = p_env->p_fn_doworker_output( p_accepted_session );
	if( nret )
	{
		bError = TRUE;
		ERRORLOGSG( "����������ʧ��[%d]" , nret );
		SET_ERROR_RESPONSE( p_accepted_session, nret , "����������ʧ��");
	}
	INFOLOGSG( "������Doworker���óɹ�");
	
	ptv_end = &( p_accepted_session->perfms.tv_publish_end );
	gettimeofday( ptv_end, NULL );
	cost_time =( ptv_end->tv_sec - ptv_start->tv_sec ) * 1000*1000 +( ptv_end->tv_usec - ptv_start->tv_usec) ;
	INFOLOGSG("sendMsg threadno[%d] cost_time[%ld]us p_accepted_session[%p]",  threadno, cost_time, p_accepted_session );
	
	if( bError && p_accepted_session->http_rsp_body[0] != 0 )
	{
		memset( body_convert, 0, sizeof(body_convert) );
		status_code = HTTP_INTERNAL_SERVER_ERROR;
		nret = ConvertReponseBody( p_accepted_session, body_convert, sizeof(body_convert) );
		if( nret )
		{
			ERRORLOGSG( "ConvertReponseBody failed[%d]" , nret );
		}
		else
		{
			strncpy( p_accepted_session->http_rsp_body, body_convert, p_accepted_session->body_len-1 );
		}
		
	}
	else
	{
		status_code = HTTP_OK;
		//SET_ERROR_RESPONSE( p_accepted_session, 0 , "");
		//strncpy( body_convert, p_accepted_session->http_rsp_body, sizeof(body_convert)-1 );
	}

	/*��ʱ�䳬��Ĭ������30���ӣ�����Connection:Close*/
	connection_value = HTTP_OPTIONS_FILL_CONNECTION_NONE;
	if( CheckHttpKeepAlive( p_accepted_session->http ) )
	{
		if( ( ptv_end->tv_sec - p_accepted_session->accept_begin_time ) > p_env->httpserver_conf.httpserver.server.keepAliveMaxTime )
			connection_value = HTTP_OPTIONS_FILL_CONNECTION_CLOSE;
		else 
			connection_value = HTTP_OPTIONS_FILL_CONNECTION_KEEPALIVE;
	}

	nret = FormatHttpResponseStartLine2( status_code , p_accepted_session->http, connection_value, 0,"Content-Type: %s%s"
				HTTP_RETURN_NEWLINE "Content-length: %d" 
				HTTP_RETURN_NEWLINE HTTP_RETURN_NEWLINE "%s" ,
				HTTP_HEADER_CONTENT_TYPE_JSON , 
				p_accepted_session->charset,
				strlen( p_accepted_session->http_rsp_body ) ,
				p_accepted_session->http_rsp_body );
	if( nret )
	{
		ERRORLOGSG( "StrcatfHttpBuffer failed[%d]" , nret );
	} 
	
	if( p_accepted_session->needFree )
	{
		/*�Ѿ�fd�Ѿ���epoll��ɾ�������Բ�����ڲ�������*/
		FreeSession( p_accepted_session );
	}
	else
	{
		nret = AddEpollSendEvent( p_env, p_accepted_session );
	}
	
	INFOLOGSG( "ThreadWorker threadno[%d] end", threadno );
	
	return 0;
}

/*	
static int CheckHeadValid( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session, struct httpserver_httphead  *p_reqhead )
{	 
	return 0;
}
*/

static int OnProcessAddTask( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	taskinfo_t	task ;
	int		nret = 0;
	
	//��������������ҵ��
	INFOLOGSG( "begin p_fn_doworker[%p]_input", p_env->p_fn_doworker_input);
	nret = p_env->p_fn_doworker_input( p_accepted_session );
	if( nret )
	{
		ERRORLOGSG( "����������ʧ��[%d]" , nret );
		return -1;
	}
	INFOLOGSG( "������Doworker���óɹ�");
	
	/*��������ǰ����״̬����ʾ�̻߳�δ������*/
	gettimeofday( &( p_accepted_session->perfms.tv_receive_end ), NULL ) ;
	p_accepted_session->status = SESSION_STATUS_PUBLISH;
	INFOLOGSG( "p_accepted_session[%p] fd[%d] receive end set SESSION_STATUS_PUBLISH" , p_accepted_session, p_accepted_session->netaddr.sock );
	
	memset( &task, 0, sizeof(taskinfo_t) );
	task.fn_callback = ThreadWorker;
	task.arg = p_accepted_session;
	nret = threadpool_addTask( p_env->p_threadpool, &task );
	if( nret < 0 )
	{
		ERRORLOGSG( "threadpool_addTask failed nret[%d]", nret);
		return -1;
	}
	INFOLOGSG( "threadpool_addTask ok nret[%d]", nret);
		
	return HTTP_OK;
}

static int GetCharset( struct AcceptedSession *p_accepted_session )
{
	char 	*p_begin = NULL;
	char 	*p_end = NULL;
	char	*p_content_type = NULL;
	int	value_len;
	size_t	 len;

	p_content_type = QueryHttpHeaderPtr( p_accepted_session->http , HTTP_HEADER_CONTENT_TYPE , & value_len );
	if( !p_content_type || value_len <= 0 )
        {
                ERRORLOGSG( HTTP_HEADER_CONTENT_TYPE"error content_type[%.*s]" , value_len , p_content_type );
		return -1;
        }
        INFOLOGSG("content_type[%.*s]" , value_len , p_content_type );
        
	p_begin = strchr( p_content_type, '=' );
	if( !p_begin )
	{
		//û��ָ������Ĭ������UTF-8
		strncpy( p_accepted_session->charset, "utf-8", sizeof(p_accepted_session->charset)-1 );
		return 0;
	}
		
	/*����windows�ǵĻ��з�*/
	p_end = strstr( p_content_type, "\r\n" );
	if( !p_end )
	{
		p_end = strchr( p_content_type, '\n' );
		if( !p_end )
			return -1;
	}
	
	len = p_end - p_begin -1;
	if( len > ( sizeof(p_accepted_session->charset) -1 ) )
	{
		return -1;	
	}
	else
	{
		strncpy( p_accepted_session->charset, p_begin+1, len );
	}
	
	return 0;
}
static int CheckContentType( struct HttpserverEnv *p_env, struct AcceptedSession *p_accepted_session, char *type , int type_size)
{
	char		*p_content = NULL;
	int		value_len;
	char		nret;

	p_content = QueryHttpHeaderPtr( p_accepted_session->http , HTTP_HEADER_CONTENT_TYPE , & value_len );
	if( ! MEMCMP( p_content , == , type , type_size -1 )  )
	{
		ERRORLOGSG( "content_type[%.*s] Is InCorrect" , value_len , p_content );
		SET_ERROR_RESPONSE( p_accepted_session, HTTP_BAD_REQUEST, HTTP_BAD_REQUEST_T );
		return HTTP_BAD_REQUEST;
	}
	
	return  HTTP_OK;
}
int OnProcess( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	char			*uri = NULL;
	int			uri_len;
	char			*method = NULL;
	int			method_len;

	int			nret = 0;
	
	/* �õ�method ��URI */
	method = GetHttpHeaderPtr_METHOD( p_accepted_session->http , & method_len );
	uri = GetHttpHeaderPtr_URI( p_accepted_session->http , & uri_len );
	INFOLOGSG( "method[%.*s] uri[%.*s]" ,method_len , method ,  uri_len , uri );
	
	if( method_len == sizeof(HTTP_METHOD_POST)-1 && MEMCMP( method , == , HTTP_METHOD_POST , method_len ) )
	{
		nret = GetCharset( p_accepted_session );
		if( nret )
		{
			ERRORLOGSG( "GetCharset error" );
			SET_ERROR_RESPONSE( p_accepted_session, HTTP_BAD_REQUEST, HTTP_BAD_REQUEST_T );
			return HTTP_BAD_REQUEST;
		}
		
		INFOLOGSG( "GetCharset[%s]", p_accepted_session->charset );
			
		nret = OnProcessAddTask( p_env, p_accepted_session );
		if( nret != HTTP_OK )
		{
			ERRORLOGSG( "OnProcessAddTadk failed[%d]" , nret );
			return HTTP_INTERNAL_SERVER_ERROR;
		}
		INFOLOGSG( "OnProcessAddTadk ok" );
	}
	else if( method_len == sizeof(HTTP_METHOD_GET)-1 && MEMCMP( method , == , HTTP_METHOD_GET , method_len ) )
	{
		/* ��ʾ���������ӿͻ��˻Ự */
		if(  uri_len == sizeof(URI_SESSIONS)-1 && MEMCMP( uri , == , URI_SESSIONS , uri_len ) )
		{
			nret = CheckContentType( p_env, p_accepted_session, HTTP_HEADER_CONTENT_TYPE_TEXT, sizeof(HTTP_HEADER_CONTENT_TYPE_TEXT) );
			if( nret != HTTP_OK )
			{
				ERRORLOGSG( "CheckContentType failed[%d]" , nret );
				return nret;
			}
	
			nret = OnProcessShowSessions( p_env , p_accepted_session );
			if( nret != HTTP_OK )
			{
				ERRORLOGSG( "OnProcessShowSessions failed[%d]" , nret );
				return HTTP_INTERNAL_SERVER_ERROR;
			}
			else
			{
				INFOLOGSG( "OnProcessShowSessions ok" );
			}
		}
		/* ��ʾ������߳�״̬��Ϣ */
		else if(  uri_len == sizeof(URI_THREAD_STATUS)-1 && MEMCMP( uri , == , URI_THREAD_STATUS , uri_len ) )
		{
			nret = CheckContentType( p_env, p_accepted_session, HTTP_HEADER_CONTENT_TYPE_TEXT, sizeof(HTTP_HEADER_CONTENT_TYPE_TEXT) );
			if( nret != HTTP_OK )
			{
				ERRORLOGSG( "CheckContentType failed[%d]" , nret );
				return nret;
			}
	
			nret = OnProcessShowThreadStatus( p_env , p_accepted_session );
			if( nret != HTTP_OK )
			{
				ERRORLOGSG( "OnProcessShowThreadStatus failed[%d]" , nret );
				return HTTP_INTERNAL_SERVER_ERROR;
			}
			else
			{
				INFOLOGSG( "OnProcessShowThreadStatus ok" );
			}
		}
		/* ��ʾhttpserver.conf������Ϣ */
		else if( uri_len == sizeof(URI_CONFIG)-1 && MEMCMP( uri , == , URI_CONFIG , uri_len ) )
		{
			nret = CheckContentType( p_env, p_accepted_session, HTTP_HEADER_CONTENT_TYPE_TEXT, sizeof(HTTP_HEADER_CONTENT_TYPE_TEXT) );
			if( nret != HTTP_OK )
			{
				ERRORLOGSG( "CheckContentType failed[%d]" , nret );
				return nret;
			}
	
			nret = OnProcessShowConfig( p_env , p_accepted_session );
			if( nret != HTTP_OK )
			{
				ERRORLOGSG( "OnProcessShowConfig failed[%d]" , nret );
				return HTTP_INTERNAL_SERVER_ERROR;
			}
			else
			{
				INFOLOGSG( "OnProcessShowConfig ok" );
			}
		}
		else
		{	
			nret = OnProcessAddTask( p_env, p_accepted_session );
			if( nret != HTTP_OK )
			{
				ERRORLOGSG( "OnProcessAddTadk failed[%d]" , nret );
				return HTTP_INTERNAL_SERVER_ERROR;
			}
			INFOLOGSG( "OnProcessAddTadk ok" );
		}
	
	}
	else
	{
		ERRORLOGSG( "method unknown method[%.*s] " ,method_len , method  );  
		SET_ERROR_RESPONSE( p_accepted_session, HTTP_BAD_REQUEST, HTTP_BAD_REQUEST_T );
	}
	
	return HTTP_OK;
}


static int AddResponseData( struct HttpserverEnv *p_env, struct AcceptedSession *p_accepted_session, struct HttpBuffer *buf, char *content_type )
{
	struct HttpBuffer	*rsp_buf = NULL;
	
	int			nret = 0;
	
	rsp_buf = GetHttpResponseBuffer( p_accepted_session->http );
	nret = StrcatfHttpBuffer( rsp_buf , "Content-Type: %s" HTTP_RETURN_NEWLINE 
			"Content-length: %d"
			HTTP_RETURN_NEWLINE HTTP_RETURN_NEWLINE "%.*s" , 
			content_type , 
			GetHttpBufferLength(buf) , 
			GetHttpBufferLength(buf) , GetHttpBufferBase(buf,NULL) );
	if( nret )
	{
		ERRORLOGSG( "StrcatfHttpBuffer failed[%d] , errno[%d]" , nret , errno );
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	
	INFOLOGSG( "SEND RESPONSE HTTP [%.*s]" , GetHttpBufferLength(rsp_buf) , GetHttpBufferBase(rsp_buf,NULL) );

	nret = AddEpollSendEvent( p_env, p_accepted_session );
	if( nret )
		return nret;
	
	return 0;
}
int OnProcessShowSessions( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	struct HttpBuffer	*buf = NULL;
	struct AcceptedSession	*p_session = NULL;
	
	int			nret = 0;
		
	buf = AllocHttpBuffer( 4096 );
	if( buf == NULL )
	{
		ERRORLOGSG( "AllocHttpBuffer failed[%d] , errno[%d]" , nret , errno );
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	
	list_for_each_entry( p_session , & (p_env->accepted_session_list.this_node) , struct AcceptedSession , this_node )
	{
		nret = StrcatfHttpBuffer( buf , "%s %d \n" , p_session->netaddr.remote_ip , p_session->netaddr.remote_port );
		if( nret )
		{
			ERRORLOGSG( "StrcatfHttpBuffer failed[%d] , errno[%d]" , nret , errno );
			FreeHttpBuffer( buf );
			return HTTP_INTERNAL_SERVER_ERROR;
		}
	}

	nret = AddResponseData( p_env, p_accepted_session, buf, HTTP_HEADER_CONTENT_TYPE_TEXT );
	FreeHttpBuffer( buf );
	if( nret )
	{
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	return HTTP_OK;
}

int OnProcessShowThreadStatus( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	struct HttpBuffer	*buf = NULL;
	struct ProcStatus 	*p_status = NULL;
	int			i;
	struct tm		tm_restarted;
	char			last_restarted_time[20+1];
	
	int			nret = 0;
	
	buf = AllocHttpBuffer( 4096 );
	if( buf == NULL )
	{
		ERRORLOGSG( "AllocHttpBuffer failed[%d] , errno[%d]" , nret , errno );
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	p_status = (struct ProcStatus*)(p_env->p_shmPtr);
	for( i = 0; i < p_env->httpserver_conf.httpserver.server.processCount; i++, p_status++ )
	{
		localtime_r( &( p_status->last_restarted_time ), &tm_restarted );
		memset( last_restarted_time, 0, sizeof(last_restarted_time) );
		strftime( last_restarted_time, sizeof(last_restarted_time), "%Y-%m-%d %H:%M:%S", &tm_restarted );
		
		nret = StrcatfHttpBuffer( buf , "process[%d] "
						"startTime[%s] "
						"task[%d] "
						"working[%d] "
						"idle[%d] "
						"thread[%d] "
						"timeout[%d] "
						"min[%d] "
						"max[%d] "
						"percent[%d] "
						"session[%d]\n"
						 , i+1
						 , last_restarted_time
						 , p_status->task_count
						 , p_status->working_count
						 , p_status->Idle_count 
						 , p_status->thread_count	
						 , p_status->timeout_count
						 , p_status->min_threads
						 , p_status->max_threads
						 , p_status->taskTimeoutPercent
						 , p_status->session_count );
		if( nret )
		{
			ERRORLOGSG( "StrcatfHttpBuffer failed[%d] , errno[%d]" , nret , errno );
			FreeHttpBuffer( buf );
			return HTTP_INTERNAL_SERVER_ERROR;
		}
	}
	
	nret = AddResponseData( p_env, p_accepted_session, buf, HTTP_HEADER_CONTENT_TYPE_TEXT );
	FreeHttpBuffer( buf );
	if( nret )
	{
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	
	return HTTP_OK;
}

int OnProcessShowConfig( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	struct HttpBuffer	*buf = NULL;
	char			*file_content = NULL;
	int			file_len = 0;
	
	int			nret = 0;
	
	buf = AllocHttpBuffer( 4096 );
	if( buf == NULL )
	{
		ERRORLOGSG( "AllocHttpBuffer failed[%d] , errno[%d]" , nret , errno );
		return HTTP_INTERNAL_SERVER_ERROR;
	}
	
	/* ��ȡhttpserver.conf�������ļ� */
	file_content = StrdupEntireFile( p_env->server_conf_pathfilename , & file_len ) ;
	if( file_content == NULL )
	{
		ERRORLOGSG( "StrdupEntireFile[%s] failed , nret[%d]\n" , p_env->server_conf_pathfilename , errno );
		FreeHttpBuffer( buf );
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	nret = StrcatfHttpBuffer( buf , "%s" , file_content );
	if( nret )
	{
		ERRORLOGSG( "StrcatfHttpBuffer failed[%d] , errno[%d]" , nret , errno );
		FreeHttpBuffer( buf );
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	free(file_content);
	
	nret = AddResponseData( p_env, p_accepted_session, buf, HTTP_HEADER_CONTENT_TYPE_TEXT );
	FreeHttpBuffer( buf );
	if( nret )
	{
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	return HTTP_OK;
}

int OnProcessPipeEvent( struct HttpserverEnv *p_env , struct PipeSession *p_pipe_session )
{
	char		ch ;
	httpserver_conf	httpserver_conf ;
	char 		module_name[50];
	
	int		nret = 0 ;

	nret = (int)read( p_pipe_session->pipe_fds[0] , & ch , 1 );
	if( nret == -1 )
	{
		ERRORLOGSG( "read pipe failed[%d] , errno[%d]" , nret , errno );
		return -2;
	}
	INFOLOGSG( "read pipe ok , ch[%c]" , ch );
	
	memset( & httpserver_conf , 0x00 , sizeof(httpserver_conf) );
	nret = LoadConfig( p_env , &httpserver_conf );
	if( nret )
	{
		ERRORLOGSG( "LoadConfig error" );
		return -1;
	}
	
	memcpy( &p_env->httpserver_conf , &httpserver_conf , sizeof(httpserver_conf) );
	
	/* ������־���� */
	memset( module_name, 0, sizeof(module_name) );
	snprintf( module_name, sizeof(module_name)-1, "worker_%d", g_process_index+1 );
	nret = InitLogEnv( p_env, module_name, SERVER_AFTER_LOADCONFIG );
	if( nret )
	{
		// printf( "InitLogEnv faild[%d]\n" , nret );
		return -1;
	}
		
	if( p_env->p_threadpool )
	{
		nret = threadpool_setThreads( p_env->p_threadpool, p_env->httpserver_conf.httpserver.threadpool.minThreads, p_env->httpserver_conf.httpserver.threadpool.maxThreads );
		if( nret == THREADPOOL_FORBIT_DIMINISH )
		{
			ERRORLOGSG("����߳��ڴ�����ʹ����,����������,���˹���������ʹ������Ч!");
			return -1;
		}	
		threadpool_setTaskTimeout( p_env->p_threadpool, p_env->httpserver_conf.httpserver.threadpool.taskTimeout );
		threadpool_setThreadSeed( p_env->p_threadpool, p_env->httpserver_conf.httpserver.threadpool.threadSeed );
	}
	
	INFOLOGSG("reload_config ok!");
	
	/*�������߳�������־����ɸ��߳����н�������*/
	p_env->thread_array[RETRY_INDEX].cmd = 'L';
	p_env->thread_array[ACCEPT_INDEX].cmd = 'L';
	p_env->thread_array[SEND_EPOLL_INDEX].cmd = 'L';
	
	threadpool_setThreadsCommand( p_env->p_threadpool, 'L');
	INFOLOGSG( "threadpool_setThreads_command ok" );
	
	return 0 ;
}

/*����������־*/
int ExportPerfmsFile( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	FILE			*fp = NULL;
	char			path_file[256];
	size_t			size;
	
	if( p_env->httpserver_conf.httpserver.server.perfmsEnable > 0 )
	{
		memset( path_file, 0, sizeof(path_file) );
		snprintf( path_file, sizeof(path_file)-1, "%s/log/httpserver_perfms.log", secure_getenv("HOME") );
		fp = fopen( path_file, "ab" );
		if( !fp )
		{
			ERRORLOGSG( "fopen failed path_file[%s]", path_file );
			return -1;
		}
		
		size = fwrite( &( p_accepted_session->perfms ), sizeof(struct Performamce), 1, fp );
		if( size != 1 )
		{
			ERRORLOGSG( "fwrite error nret[%u]", size );
			fclose( fp );
			return -1;
		}
		
		fclose( fp );
	}
	
	return 0;
}

void FreeSession( struct AcceptedSession *p_accepted_session )
{
	INFOLOGSG( "close fd[%d] [%s:%d]->[%s:%d] p_accepted_session[%p] status[%d]" , p_accepted_session->netaddr.sock , p_accepted_session->netaddr.remote_ip , p_accepted_session->netaddr.remote_port , p_accepted_session->netaddr.local_ip , p_accepted_session->netaddr.local_port , p_accepted_session,p_accepted_session->status );
	DestroyHttpEnv( p_accepted_session->http );
	close( p_accepted_session->netaddr.sock );
	
	if( p_accepted_session->http_rsp_body )
		free( p_accepted_session->http_rsp_body );
	free( p_accepted_session );
	
	return;
}

int AddEpollSendEvent(struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	struct epoll_event	event ;
	int 			nret ;
	
	/*******ע�������ɾ��Ȼ���ټ���������е�˳���ϵ�����������¼��Ĳ�����������*********/
	nret = epoll_ctl( p_env->epoll_fd_recv , EPOLL_CTL_DEL , p_accepted_session->netaddr.sock , NULL );
	if( nret == -1 )
	{
		ERRORLOGSG( "epoll_ctl_recv[%d] del accepted_session[%d] EPOLLIN failed , errno[%d]" , p_env->epoll_fd_recv , p_accepted_session->netaddr.sock , errno );
		return 1;
	}
	DEBUGLOGSG( "epoll_ctl_recv[%d] del accepted_session[%d] EPOLLIN ok" , p_env->epoll_fd_recv , p_accepted_session->netaddr.sock );
	
	/* ���뵽����epoll�¼� */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLOUT | EPOLLERR ;
	event.data.ptr = p_accepted_session ;
	nret = epoll_ctl( p_env->epoll_fd_send , EPOLL_CTL_ADD , p_accepted_session->netaddr.sock , & event ) ;
	if( nret == -1 )
	{
		ERRORLOGSG( "epoll_ctl_send[%d] add accepted_session[%d] EPOLLIN failed , errno[%d]" , p_env->epoll_fd_recv , p_accepted_session->netaddr.sock , errno );
		return 1;
	}
	DEBUGLOGSG( "epoll_ctl_send[%d] add accepted_session[%d] EPOLLIN ok" , p_env->epoll_fd_send , p_accepted_session->netaddr.sock );
	
	return 0;
}

int AddEpollRecvEvent( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session )
{
	struct epoll_event	event ;
	int 			nret ;
	
	/*******ע�������ɾ��Ȼ���ټ���������е�˳���ϵ�����������¼��Ĳ�����������*********/
	nret = epoll_ctl( p_env->epoll_fd_send , EPOLL_CTL_DEL , p_accepted_session->netaddr.sock , NULL );
	if( nret == -1 )
	{
		ERRORLOGSG( "epoll_ctl_send[%d] del accepted_session[%d] EPOLLIN failed , errno[%d]" , p_env->epoll_fd_send , p_accepted_session->netaddr.sock , errno );
		return 1;
	}	
	DEBUGLOGSG( "epoll_ctl_send[%d] del accepted_session[%d] EPOLLIN ok" , p_env->epoll_fd_send , p_accepted_session->netaddr.sock );
	
	/* ���뵽����epoll�¼� */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = p_accepted_session ;
	nret = epoll_ctl( p_env->epoll_fd_recv , EPOLL_CTL_ADD , p_accepted_session->netaddr.sock , & event ) ;
	if( nret == -1 )
	{
		ERRORLOGSG( "epoll_ctl_recv[%d] add accepted_session[%d] EPOLLIN failed , errno[%d]" , p_env->epoll_fd_recv , p_accepted_session->netaddr.sock , errno );
		return 1;
	}
	DEBUGLOGSG( "epoll_ctl_recv[%d] add accepted_session[%d] EPOLLIN ok" , p_env->epoll_fd_recv , p_accepted_session->netaddr.sock );
	
	return 0;
}