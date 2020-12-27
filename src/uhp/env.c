/*
 * 
 * author	: jack
 * email	: xulongzhong2004@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */
  

#include "uhp_in.h"
#include "list.h"

int InitEnvironment( struct HttpserverEnv **pp_env )
{
	struct HttpserverEnv	*p_env = NULL ;

	/* �����ڴ��Դ���������ṹ */
	p_env = (struct HttpserverEnv *)malloc( sizeof(struct HttpserverEnv) ) ;
	if( p_env == NULL )
	{
		return -1;
	}
	memset( p_env , 0x00 , sizeof(struct HttpserverEnv) );
	*pp_env = p_env;
	
	INIT_LIST_HEAD( & (p_env->accepted_session_list.this_node) );
	
	return 0;
}

static int InitLogEnvV( char *service_name , char *module_name , char *event_output , int process_loglevel , char* host_name, char* user_name, char *process_output_format , va_list valist )
{
	char			pathfilename[ UTIL_MAXLEN_FILENAME + 1 ] ;
	char			process_output[ UTIL_MAXLEN_FILENAME + 1 ] ;
	LOG			*g = NULL ;
	int			nret = 0 ;


	/* ������־������� */
	CreateLogsHandleG();
	
	if( event_output && event_output[0] )
	{
		/* ����event��־��� */
		g = CreateLogHandle() ;
		if( g == NULL )
			return -11;
		
		SetLogCustLabel( g , 1 , host_name );
		SetLogCustLabel( g , 2 , user_name );
		SetLogCustLabel( g , 3 , service_name );
		SetLogCustLabel( g , 4 , module_name );
		
		
		memset( pathfilename , 0x00 , sizeof(pathfilename) );
		snprintf( pathfilename , sizeof(pathfilename)-1 , "%s/%s" , secure_getenv("HOME") , event_output );
		/*printf("event pathfilename:%s\n", pathfilename);*/
		SetLogStyles( g , LOG_STYLE_DATETIMEMS|LOG_STYLE_LOGLEVEL|LOG_STYLE_CUSTLABEL1|LOG_STYLE_CUSTLABEL2|LOG_STYLE_CUSTLABEL3|LOG_STYLE_CUSTLABEL4|LOG_STYLE_FORMAT|LOG_STYLE_NEWLINE , NULL );
		nret = SetLogOutput( g , LOG_OUTPUT_FILE , pathfilename , LOG_NO_OUTPUTFUNC ) ;
		
		if( nret )
			return -12;
		
		SetLogLevel( g , LOG_LEVEL_NOTICE );

		nret = AddLogToLogsG( "event" , g ) ;
		if( nret )
			return -13;
	}
	
	if( process_output_format && process_output_format[0] )
	{
		/* ����process��־��� */
		g = CreateLogHandle() ;
		if( g == NULL )
			return -21;
		
		SetLogCustLabel( g , 1 , host_name );
		SetLogCustLabel( g , 2 , user_name );
		SetLogCustLabel( g , 3 , service_name );
		SetLogCustLabel( g , 4 , module_name );
		
		memset( process_output , 0x00 , sizeof(process_output) );
		vsnprintf( process_output , sizeof(process_output)-1 , process_output_format , valist );

		memset( pathfilename , 0x00 , sizeof(pathfilename) );
		snprintf( pathfilename , sizeof(pathfilename)-1 , "%s/%s" , secure_getenv("HOME") , process_output );
		/*printf("process pathfilename:%s\n", pathfilename);*/
		SetLogStyles( g , LOG_STYLE_DATETIMEMS|LOG_STYLE_LOGLEVEL|LOG_STYLE_CUSTLABEL1|LOG_STYLE_CUSTLABEL2|LOG_STYLE_CUSTLABEL3|LOG_STYLE_CUSTLABEL4|LOG_STYLE_PID|LOG_STYLE_TID|LOG_STYLE_SOURCE|LOG_STYLE_FORMAT|LOG_STYLE_NEWLINE , NULL );
		nret = SetLogOutput( g , LOG_OUTPUT_FILE , pathfilename , LOG_NO_OUTPUTFUNC ) ;
		if( nret )
			return -22;
		
		SetLogLevel( g , process_loglevel );
		
		nret = AddLogToLogsG( "process" , g ) ;
		if( nret )
			return -23;
	}
	
	/* ����ERROR��־��� */
	g = CreateLogHandle() ;
	if( g == NULL )
		return -51;
	
	SetLogCustLabel( g , 1 , host_name );
	SetLogCustLabel( g , 2 , user_name );
	SetLogCustLabel( g , 3 , service_name );
	SetLogCustLabel( g , 4 , module_name );
	
	memset( pathfilename , 0x00 , sizeof(pathfilename) );
	snprintf( pathfilename , sizeof(pathfilename)-1 , "%s/log/ERROR.log" , secure_getenv("HOME") );
	SetLogStyles( g , LOG_STYLE_DATETIMEMS|LOG_STYLE_LOGLEVEL|LOG_STYLE_CUSTLABEL1|LOG_STYLE_CUSTLABEL2|LOG_STYLE_CUSTLABEL3|LOG_STYLE_CUSTLABEL4|LOG_STYLE_PID|LOG_STYLE_SOURCE|LOG_STYLE_FORMAT|LOG_STYLE_NEWLINE , NULL );
	nret = SetLogOutput( g , LOG_OUTPUT_FILE , pathfilename , LOG_NO_OUTPUTFUNC ) ;
	if( nret )
		return -52;
	
	SetLogLevel( g , LOG_LEVEL_ERROR );

	nret = AddLogToLogsG( "ERROR" , g ) ;
	if( nret )
		return -53;
		
	return nret;
}

static int InitLogEnv_Format( char *service_name , char *module_name , char *event_output , int process_loglevel , char* host_name, char* user_name, char *process_output_format , ... )
{
	va_list			valist ;
	int			nret = 0 ;
	
	va_start( valist , process_output_format );
	nret = InitLogEnvV( service_name , module_name , event_output , process_loglevel , host_name, user_name, process_output_format , valist ) ;
	va_end( valist );
	
	return nret;
}

int cleanLogEnv()
{
	if( GetLogsHandleG() )
	{
		DestroyLogsHandleG();
		SetLogsHandleG( NULL );
	}
	
	return 0;
}

int InitLogEnv( struct HttpserverEnv *p_env, char* module_name, int loadConfig )
{
	int	nret;
	char	host_name[ HOST_NAME_MAX + 1 ];
	char 	*user_name = NULL;
	char	*p_loglevel = NULL;
	int	rotate_size;
	
	memset( host_name, 0, sizeof(host_name) );
	nret = gethostname( host_name , sizeof(host_name) );
	if( nret == -1 )
		strncpy( host_name, "hostname", sizeof(host_name)-1 );
	
	user_name = secure_getenv( "USER" );
	if( !user_name )
		user_name ="user";
	
	cleanLogEnv();
	
	if( loadConfig == SERVER_BEFORE_LOADCONFIG )
	{
		p_loglevel = "DEBUG";
		rotate_size = 0;
	}
	else
	{
		if( strcmp( module_name, "main") == 0 )
			p_loglevel =  p_env->httpserver_conf.httpserver.log.main_loglevel ;
		else if( strcmp( module_name, "monitor") == 0 )
			p_loglevel =  p_env->httpserver_conf.httpserver.log.monitor_loglevel ;
		else if( strncmp( module_name, "worker", sizeof("worker")-1 ) == 0
			|| strncmp( module_name, "retry_worker", sizeof("retry_worker")-1 ) == 0 
			)
			p_loglevel =  p_env->httpserver_conf.httpserver.log.worker_loglevel ;
		else
			p_loglevel =  p_env->httpserver_conf.httpserver.log.main_loglevel ;
		
		rotate_size = ConvertFileSizeString( p_env->httpserver_conf.httpserver.log.rotate_size );

	}
	
	nret = InitLogEnv_Format( "uhp" , module_name ,  "log/event.log" , ConvertLogLevel( p_loglevel ) ,host_name , user_name, 
		"log/uhp_%s_%s_%s.log" , module_name, host_name , user_name );
	if( nret )
		return -1;
	
	SetLogRotate( rotate_size );
		
	return 0;	
}

int CleanEnvironment( struct HttpserverEnv *p_env )
{
	struct list_head	*node = NULL; 
	struct list_head	*next = NULL;
	struct AcceptedSession	*p_accepted_session = NULL;
	
	int 			nret;
	
	if( p_env->thread_array[RETRY_INDEX].thread_id > 0 )
	{
		pthread_join( p_env->thread_array[RETRY_INDEX].thread_id, NULL );
		p_env->thread_array[RETRY_INDEX].thread_id = -1;
		INFOLOGSG("pid[%ld] ppid[%ld] pthread_join retry_thread_id OK", getpid(),getppid() );
	}
	
	if( p_env->thread_array[ACCEPT_INDEX].thread_id > 0 )
	{
		pthread_join( p_env->thread_array[ACCEPT_INDEX].thread_id, NULL );
		p_env->thread_array[ACCEPT_INDEX].thread_id = -1;
		INFOLOGSG("pid[%ld] ppid[%ld] pthread_join accept_thread_id OK", getpid(),getppid() );
	}
	
	if( p_env->thread_array[SEND_EPOLL_INDEX].thread_id > 0 )
	{
		pthread_join( p_env->thread_array[SEND_EPOLL_INDEX].thread_id, NULL );
		p_env->thread_array[SEND_EPOLL_INDEX].thread_id = -1;
		INFOLOGSG("pid[%ld] ppid[%ld] pthread_join accept_thread_id OK", getpid(),getppid() );
	}
	
	if( p_env->p_threadpool )
	{
		nret = threadpool_destroy( p_env->p_threadpool );
		if( nret == THREADPOOL_HAVE_TASKS )
		{
			FATALLOGSG("thread have tasks exception exit" );
			return nret;
		}
		else if( nret )
		{
			ERRORLOGSG("threadpool_destroy error errno[%d]", errno );
			return nret;
		}
		
		p_env->p_threadpool = NULL;
		INFOLOGSG("pid[%ld] ppid[%ld] threadpool_destroy OK", getpid(),getppid() );
	}
	
	pthread_mutex_lock( &( p_env->session_lock ));
	list_for_each_safe( node , next, & (p_env->accepted_session_list.this_node) )
	{
		p_accepted_session = list_entry( node , struct AcceptedSession , this_node ) ;
		OnClosingSocket( p_env , p_accepted_session, FALSE );
	}
	pthread_mutex_unlock( &( p_env->session_lock ));
	
	cleanLogEnv();
	if( p_env->p_pipe_session )
		free( p_env->p_pipe_session );
	
	if( p_env )
		free( p_env );
	
	return 0;
}


