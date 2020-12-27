/*
 * author	: jack
 * email	: xulongzhong2004@163.com
 *
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */
#include <sys/prctl.h>
#include "uhp_in.h"

#define			SESSION_TIMEOUT_SEED		1.5	/*��ʱ����*/

void* SendEpollWorker( void *arg )
{
	struct HttpserverEnv 	*p_env = NULL;
	struct epoll_event	events[ MAX_EPOLL_EVENTS ] ;
	int			epoll_nfds = 0 ;
	struct epoll_event	*p_event = NULL ;
	struct AcceptedSession	*p_accepted_session = NULL ;
	
	int			i = 0 ;
	int			nret;
	
	p_env = ( struct HttpserverEnv* )arg;
	p_env->thread_array[SEND_EPOLL_INDEX].cmd = 'I';
	prctl( PR_SET_NAME, "SendEpoll" );
	ResetAllHttpStatus();

	while( ! g_exit_flag )
	{
		if( p_env->thread_array[SEND_EPOLL_INDEX].cmd == 'I' ||  p_env->thread_array[SEND_EPOLL_INDEX].cmd == 'L' ) /*�߳���Ӧ�ó�ʼ��*/
		{
			char	module_name[50];
			
			memset( module_name, 0, sizeof(module_name) );
			snprintf( module_name, sizeof(module_name) - 1, "worker_%d", g_process_index+1 );
			/* ������־���� */
			InitLogEnv( p_env, module_name, SERVER_AFTER_LOADCONFIG );
			
			if( p_env->thread_array[SEND_EPOLL_INDEX].cmd == 'I' )
			{
				INFOLOGSG(" InitLogEnv module_name[%s] ok", module_name );
			}
			else
			{
				INFOLOGSG(" reload config module_name[%s] ok", module_name );
			}
			
			p_env->thread_array[SEND_EPOLL_INDEX].cmd = 0;
		}
		
		if( getppid() == 1 )
		{
			INFOLOGSG("parent process is exit");
			break;
		}
		
		/* �ȴ�epoll�¼�������1�볬ʱ */
		memset( events , 0x00 , sizeof(events) );
		epoll_nfds = epoll_wait( p_env->epoll_fd_send , events , MAX_EPOLL_EVENTS , 1000 ) ;		
		if( epoll_nfds == -1 )
		{
			if( errno == EINTR )
			{
				INFOLOGSG( "epoll_wait_send[%d] interrupted" , p_env->epoll_fd_send );
				continue;
			}
			else
			{
				FATALLOGSG( "epoll_wait_send[%d] failed , errno[%d]" , p_env->epoll_fd_send , ERRNO );
				g_exit_flag = 1 ;
			}
		}
		
		/* ���������¼� */
		INFOLOGSG( "epoll_wait_send epoll_fd[%d] epoll_nfds[%d]" , p_env->epoll_fd_send , epoll_nfds );
		for( i = 0 , p_event = events ; i < epoll_nfds ; i++ , p_event++ )
		{
			INFOLOGSG( "epoll_wait_send current_index[%d] p_event->data.ptr[%p] p_env->listen_session[%p] " , i , p_event->data.ptr , & (p_env->listen_session) );
		
			p_accepted_session = (struct AcceptedSession *)(p_event->data.ptr) ;
			/* ֻ������д�¼��������¼���recv epoll�̴߳��� */
			if( p_event->events & EPOLLOUT )
			{
				nret = OnSendingSocket( p_env , p_accepted_session ) ;
				if( nret < 0 )
				{
					FATALLOGSG( "OnSendingSocket failed[%d]" , nret	);
					g_exit_flag = 1 ;
				}
				else if( nret > 0 )
				{
					INFOLOGSG( "OnSendingSocket return[%d], p_accepted_session[%p]", nret, p_accepted_session );
					OnClosingSocket( p_env , p_accepted_session, TRUE );
				}
				else
				{
					DEBUGLOGSG( "OnSendingSocket ok" );
				}
			}
			/* �����¼� */
			else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) || ( p_event->events & EPOLLRDHUP ) )
			{
				int		errCode;
				
				if( p_event->events & EPOLLERR )
					errCode = EPOLLERR;
				else if( p_event->events & EPOLLHUP )
					errCode = EPOLLHUP;
				else
					errCode = EPOLLRDHUP;
					
				ERRORLOGSG( "accepted_session EPOLLERR[%d] EPOLLHUP[%d] EPOLLRDHUP[%d] errno[%d]", EPOLLERR, EPOLLHUP, EPOLLRDHUP, errno );
				ERRORLOGSG( "p_accepted_session[%p] errCode[%d] status[%d] fd[%d] errno[%d] ���Ա���ȫ�ر�", p_accepted_session, errCode,
				 	p_accepted_session->status, p_accepted_session->netaddr.sock , errno );
				
				OnClosingSocket( p_env , p_accepted_session, TRUE );
				
			}
			/* �����¼� */
			else
			{
				FATALLOGSG( "Unknow accepted session event[0x%X]" , p_event->events );
				g_exit_flag = 1 ;
			}
		}
		
	}
	
	cleanLogEnv();
	INFOLOGSG(" Send epoll thread exit" );
	
	return 0;
}

void* AcceptWorker( void *arg )
{
	struct HttpserverEnv 	*p_env = NULL;
	struct pollfd 		poll_fd;
	
	int			nret;
	
	p_env = ( struct HttpserverEnv* )arg;
	p_env->thread_array[ACCEPT_INDEX].cmd = 'I';
	prctl( PR_SET_NAME, "Accept" );
	
	while( ! g_exit_flag )
	{
		if( p_env->thread_array[ACCEPT_INDEX].cmd == 'I' ||  p_env->thread_array[ACCEPT_INDEX].cmd == 'L' ) /*�߳���Ӧ�ó�ʼ��*/
		{
			char	module_name[50];
			
			memset( module_name, 0, sizeof(module_name) );
			snprintf( module_name, sizeof(module_name) - 1, "worker_%d", g_process_index+1 );
			/* ������־���� */
			InitLogEnv( p_env, module_name, SERVER_AFTER_LOADCONFIG );
			
			if( p_env->thread_array[ACCEPT_INDEX].cmd == 'I' )
			{
				INFOLOGSG(" InitLogEnv module_name[%s] ok", module_name );
			}
			else
			{
				INFOLOGSG(" reload config module_name[%s] ok", module_name );
			}
			
			p_env->thread_array[ACCEPT_INDEX].cmd = 0;
		}
		
		if( getppid() == 1 )
		{
			INFOLOGSG("parent process is exit");
			break;
		}
		
		/*����Ƿ��������ӽ���*/
		poll_fd.fd = p_env->listen_session.netaddr.sock;
		poll_fd.events = POLLIN;
		nret = poll( &poll_fd , 1 , 1000 );
		if( nret < 0 )
		{
			ERRORLOGSG( "poll failed[%d]" , nret );
			g_exit_flag = 1;
			break;
		}
		else if( nret == 0 )
		{
			INFOLOGSG( "accept poll timeout" );
			continue;
		}
		else
		{
			INFOLOGSG( "accept poll ok nret[%d]", nret );
		}

		if( poll_fd.revents & ( POLLERR | POLLNVAL | POLLHUP ) )
		{
			INFOLOGSG("parent process is exit");
			break;
		}
		else if( poll_fd.revents & POLLIN )
		{
			nret = OnAcceptingSocket( p_env, &( p_env->listen_session ) );
			if( nret < 0 )
			{
				g_exit_flag = 1;
				FATALLOGSG( "OnAcceptingSocket failed[%d]" , nret );
				break;
			}
			else if( nret > 0 )
			{
				INFOLOGSG( "OnAcceptingSocket return[%d]" , nret );
			}
			else
			{
				INFOLOGSG( "OnAcceptingSocket ok" );
			}
			
		}
		else 
		{
			DEBUGLOGSG( "revents[%d] POLLIN[%d] POLLOUT[%d] nret[%d]", poll_fd.revents, POLLIN, POLLOUT, nret );
		}
		
	}
	
	cleanLogEnv();
	INFOLOGSG(" AccepteWorker thread exit" );
	
	return 0;
}

static int UpdateWorkingStatus( struct HttpserverEnv *p_env )
{
	
	struct ProcStatus *p_proc = (struct ProcStatus*)(p_env->p_shmPtr);
	
	p_proc[g_process_index].process_index = g_process_index;
	p_proc[g_process_index].task_count = threadpool_getTaskCount( p_env->p_threadpool );
	p_proc[g_process_index].working_count = threadpool_getWorkingCount( p_env->p_threadpool );
	p_proc[g_process_index].Idle_count = threadpool_getIdleCount( p_env->p_threadpool );
	p_proc[g_process_index].thread_count = threadpool_getThreadCount( p_env->p_threadpool );
	
	p_proc[g_process_index].timeout_count = threadpool_getWorkingTimeoutCount( p_env->p_threadpool );
	p_proc[g_process_index].min_threads = threadpool_getMinThreads( p_env->p_threadpool );
	p_proc[g_process_index].max_threads = threadpool_getMaxThreads( p_env->p_threadpool );
	p_proc[g_process_index].taskTimeoutPercent =( 1.0*p_proc[g_process_index].timeout_count / p_proc[g_process_index].max_threads ) * 100 ;
	p_proc[g_process_index].session_count = p_env->session_count;
	
	/*��ǰ����ִ������ʱ�������ڰٷ�֮95��
	��ǰ�̳߳ش��ڽ���״̬����Ҫ��������*/
	if( p_proc[g_process_index].taskTimeoutPercent > p_env->httpserver_conf.httpserver.server.taskTimeoutPercent )
	{
		ERRORLOGSG("��ǰ����ʱ����[%d]�������÷�ֵ[%d],ǿ����������", p_proc[g_process_index].taskTimeoutPercent, p_env->httpserver_conf.httpserver.server.taskTimeoutPercent );
		exit(7);
	}
	
	INFOLOGSG("process[%d] UpdateWorkingStatus ok", g_process_index );
	
	return 0;
}

void* TimerWorker( void *arg )
{
	struct HttpserverEnv *p_env = (struct HttpserverEnv*)arg;

	p_env->thread_array[RETRY_INDEX].cmd = 'I';
	prctl( PR_SET_NAME, "TimerWorker" );
	
	while( ! g_exit_flag )
	{
		if( p_env->thread_array[RETRY_INDEX].cmd == 'I' ||  p_env->thread_array[RETRY_INDEX].cmd == 'L' ) /*�߳���Ӧ�ó�ʼ��*/
		{
			char	module_name[50];
			
			memset( module_name, 0, sizeof(module_name) );
			strncpy( module_name, "timerWorker", sizeof(module_name) - 1 );
			/* ������־���� */
			InitLogEnv( p_env, module_name, SERVER_AFTER_LOADCONFIG );
			
			if( p_env->thread_array[RETRY_INDEX].cmd == 'I' )
			{
				INFOLOGSG(" InitLogEnv module_name[%s] ok", module_name );
			}
			else
			{
				INFOLOGSG(" reload config module_name[%s] ok", module_name );
			}
			
			p_env->thread_array[RETRY_INDEX].cmd = 0;
		}
		
		if( getppid() == 1 )
		{
			INFOLOGSG("parent process is exit");
			break;
		}
		
		UpdateWorkingStatus( p_env);
	
		sleep( 1 );
			
	}
	
	cleanLogEnv();
	INFOLOGSG(" TimerWorker thread exit" );
	
	return 0;
}

int InitPlugin( struct HttpserverEnv *p_env )
{
	int	nret = 0 ;
	char	*error = NULL;
	
	if( p_env->httpserver_conf.httpserver.server.pluginInputPath[0] != 0 )
	{
		p_env->plugin_handle_input = dlopen( p_env->httpserver_conf.httpserver.server.pluginInputPath, RTLD_NOW );
		if( p_env->plugin_handle_input == NULL )
		{
			error = dlerror();
			ERRORLOGSG( "dlopen failed: errno[%d] error[%s] path[%s]", errno, error, p_env->httpserver_conf.httpserver.server.pluginInputPath );
			return -1;
		}
		
		dlerror();
		p_env->p_fn_load_input = (fn_void*)dlsym( p_env->plugin_handle_input, PLUGIN_LOAD );
		error = dlerror();
		if( p_env->p_fn_load_input == NULL || error )
		{
			ERRORLOGSG( "��������λ��������ʧ��[%s] errno[%d] error[%s]", PLUGIN_LOAD, errno, error );
			return -1;
		}
	
		dlerror();
		p_env->p_fn_unload_input = (fn_void*)dlsym( p_env->plugin_handle_input, PLUGIN_UNLOAD );
		error = dlerror();
		if( p_env->p_fn_unload_input == NULL || error )
		{
			ERRORLOGSG( "��������λ��������ʧ��[%s] errno[%d] error[%s]", PLUGIN_UNLOAD, errno, error );
			return -1;
		}
		
		dlerror();
		p_env->p_fn_doworker_input = (fn_doworker*)dlsym( p_env->plugin_handle_input, PLUGIN_DOWORKER );
		error = dlerror();
		if( p_env->p_fn_doworker_input == NULL || error )
		{
			ERRORLOGSG( "��������λ��������ʧ��[%s] errno[%d] error[%s]", PLUGIN_DOWORKER, errno, error );
			return -1;
		}
		
		nret = p_env->p_fn_load_input();
		if( nret )
		{
			ERRORLOGSG( "������װ�س�ʼ��ʧ��[%s] errno[%d] error[%s]", PLUGIN_LOAD, errno, error );
			return -1;
		}
		INFOLOGSG( "������װ�س�ʼ���ɹ�" );
	}
	
	p_env->plugin_handle_output = dlopen( p_env->httpserver_conf.httpserver.server.pluginOutputPath, RTLD_NOW );
	if( p_env->plugin_handle_output == NULL )
	{
		error = dlerror();
		ERRORLOGSG( "dlopen failed: errno[%d] error[%s] path[%s]", errno, error, p_env->httpserver_conf.httpserver.server.pluginOutputPath );
		return -1;
	}
	
	dlerror();
	p_env->p_fn_load_output = (fn_void*)dlsym( p_env->plugin_handle_output, PLUGIN_LOAD );
	error = dlerror();
	if( p_env->p_fn_load_output == NULL || error )
	{
		ERRORLOGSG( "��������λ��������ʧ��[%s] errno[%d] error[%s]", PLUGIN_LOAD, errno, error );
		return -1;
	}

	dlerror();
	p_env->p_fn_unload_output = (fn_void*)dlsym( p_env->plugin_handle_output, PLUGIN_UNLOAD );
	error = dlerror();
	if( p_env->p_fn_unload_output == NULL || error )
	{
		ERRORLOGSG( "��������λ��������ʧ��[%s] errno[%d] error[%s]", PLUGIN_UNLOAD, errno, error );
		return -1;
	}
	
	dlerror();
	p_env->p_fn_doworker_output = (fn_doworker*)dlsym( p_env->plugin_handle_output, PLUGIN_DOWORKER );
	error = dlerror();
	if( p_env->p_fn_doworker_output == NULL || error )
	{
		ERRORLOGSG( "��������λ��������ʧ��[%s] errno[%d] error[%s]", PLUGIN_DOWORKER, errno, error );
		return -1;
	}
	
	nret = p_env->p_fn_load_output();
	if( nret )
	{
		ERRORLOGSG( "������װ�س�ʼ��ʧ��[%s] errno[%d] error[%s]", PLUGIN_LOAD, errno, error );
		return -1;
	}
	INFOLOGSG( "������װ�س�ʼ���ɹ�" );
	
	return 0;
}
int InitWorkerEnv( struct HttpserverEnv *p_env )
{
	int			nret = 0 ;
	char 			module_name[50];
	//char			*error = NULL;
	
	ResetAllHttpStatus();
	/* ������־���� */
	memset( module_name, 0, sizeof(module_name) );
	snprintf( module_name, sizeof(module_name)-1, "worker_%d", g_process_index+1 );
	nret = InitLogEnv( p_env, module_name, SERVER_AFTER_LOADCONFIG );
	if( nret )
	{
		// printf( "InitLogEnv failed[%d]\n" , nret );
		return nret;
	}
	
	nret = InitPlugin( p_env );
	if( nret )
	{
		ERRORLOGSG("InitPlugin failed" );
		return nret;
	}
	
	p_env->p_threadpool = threadpool_create( p_env->httpserver_conf.httpserver.threadpool.minThreads, p_env->httpserver_conf.httpserver.threadpool.maxThreads );
	if( !p_env->p_threadpool )
	{
		ERRORLOGSG("threadpool_create error ");
		return -1;
	}
	threadpool_setReserver1( p_env->p_threadpool, p_env );
	threadpool_setBeginCallback( p_env->p_threadpool, &ThreadBegin );
	threadpool_setRunningCallback( p_env->p_threadpool, &ThreadRunning );
	threadpool_setExitCallback( p_env->p_threadpool, &ThreadExit );	
	threadpool_setTaskTimeout( p_env->p_threadpool, p_env->httpserver_conf.httpserver.threadpool.taskTimeout );
	threadpool_setThreadSeed( p_env->p_threadpool, p_env->httpserver_conf.httpserver.threadpool.threadSeed );
	
	nret = threadpool_start( p_env->p_threadpool );
	if( nret )
	{
		ERRORLOGSG("threadpool_start error ");
		return -1;
	}

	nret = pthread_create( &(p_env->thread_array[RETRY_INDEX].thread_id), NULL, TimerWorker, p_env );
	if( nret )
	{
		ERRORLOGSG( "pthread_create failed , errno[%d]" , errno );
		return -1;
	}
	INFOLOGSG( "pthread_create retry_thread_id[%ld] ok" , p_env->thread_array[RETRY_INDEX].thread_id );

	/* ����recv epoll */
	p_env->epoll_fd_recv = epoll_create( 1024 ) ;
	if( p_env->epoll_fd_recv == -1 )
	{
		ERRORLOGSG( "recv epoll_create failed , errno[%d]" , errno );
		return -1;
	}
	
	/* ����send epoll */
	p_env->epoll_fd_send = epoll_create( 1024 ) ;
	if( p_env->epoll_fd_send == -1 )
	{
		ERRORLOGSG( "send epoll_create failed , errno[%d]" , errno );
		return -1;
	}
	
	nret = pthread_create( &(p_env->thread_array[ACCEPT_INDEX].thread_id), NULL, AcceptWorker, p_env );
	if( nret )
	{
		ERRORLOGSG( "pthread_create failed , errno[%d]" , errno );
		return -1;
	}
	INFOLOGSG( "pthread_create accept_thread_id[%ld] ok" , p_env->thread_array[ACCEPT_INDEX].thread_id );

	nret = pthread_create( &(p_env->thread_array[SEND_EPOLL_INDEX].thread_id), NULL, SendEpollWorker, p_env );
	if( nret )
	{
		ERRORLOGSG( "pthread_create failed , errno[%d]" , errno );
		g_exit_flag = 1 ;
		return -1;
	}
	INFOLOGSG( "pthread_create SendEpollWorker thread_id[%ld] ok" , p_env->thread_array[SEND_EPOLL_INDEX].thread_id );
	
	return 0;
	
}

static int TravelSessions( struct HttpserverEnv *p_env )
{
	struct AcceptedSession	*p_session = NULL;
	struct timeval		now_time;
	struct list_head	*node = NULL; 
	struct list_head	*next = NULL;
	
	gettimeofday( &now_time, NULL );
	/*����1������ѯsession,��ֹcpu�ߺ�*/
	if( ( now_time.tv_sec - p_env->last_loop_session_timestamp ) >= 1 )
	{
		pthread_mutex_lock( &( p_env->session_lock ));
		list_for_each_safe( node , next, & (p_env->accepted_session_list.this_node) )
		{
			p_session = list_entry( node , struct AcceptedSession , this_node ) ;
			DEBUGLOGSG("traversal sessions ip: %s %d now_time[%ld] request_begin_time[%ld]", p_session->netaddr.remote_ip, 
				p_session->netaddr.remote_port,now_time.tv_sec, p_session->request_begin_time ); 
			
			if( p_session->status == SESSION_STATUS_PUBLISH )
				continue;
				
			if( CheckHttpKeepAlive( p_session->http ) ) 
			{
				/*������ʱ,����ʱ�䳬��Ĭ��300�룬�Ͽ�����*/
				if( ( now_time.tv_sec - p_session->request_begin_time ) > p_env->httpserver_conf.httpserver.server.keepAliveIdleTimeout )
				{
					INFOLOGSG( "keep_alive idling timeout close session fd[%d] p_session[%p]", p_session->netaddr.sock, p_session );
					OnClosingSocket( p_env, p_session, FALSE );
				}		
			}
			else 
			{
				/*������ʱ,Ϊ���ý���ʱ���1.5�����Ͽ�����*/
				if( ( now_time.tv_sec - p_session->request_begin_time ) > p_env->httpserver_conf.httpserver.server.totalTimeout*SESSION_TIMEOUT_SEED )
				{
					INFOLOGSG( "idling timeout close session fd[%d] p_session[%p]", p_session->netaddr.sock, p_session );
					OnClosingSocket( p_env, p_session, FALSE );
				}
				
			}
		}	
		pthread_mutex_unlock( &( p_env->session_lock ));
		
		p_env->last_loop_session_timestamp = now_time.tv_sec;
		
		DEBUGLOGSG("timer traversal sessions ok" );
	}
	
	return 0;
}

static int IsRun( struct HttpserverEnv *p_env )
{
	int			empty  = 0;
	int			task_count;
	struct AcceptedSession	*p_session = NULL;
	struct list_head	*node = NULL; 
	struct list_head	*next = NULL;

	/*�����ʱ����е�session*/
	TravelSessions( p_env );
	
	if( !g_exit_flag )
		return 1;
	
	/*���յ�ϵͳ�˳�����,�رջ�û�п�ʼҵ�����Ự��ʹsdk����ת��*/
	pthread_mutex_lock( &( p_env->session_lock ));
	list_for_each_safe( node , next, & (p_env->accepted_session_list.this_node) )
	{
		p_session = list_entry( node , struct AcceptedSession , this_node ) ;
		INFOLOGSG(" process will exit ip: %s %d", p_session->netaddr.remote_ip, p_session->netaddr.remote_port );
		if( p_session->status < SESSION_STATUS_PUBLISH || p_session->status == SESSION_STATUS_DIE )
		{
			INFOLOGSG( "process will exit close session fd[%d], p_session[%p]", p_session->netaddr.sock, p_session );
			OnClosingSocket( p_env, p_session, FALSE );
		}
	}
	empty = list_empty( & ( p_env->accepted_session_list.this_node ) );
	pthread_mutex_unlock( &( p_env->session_lock ));
	
	task_count = threadpool_getTaskCount( p_env->p_threadpool );
	INFOLOGSG("IsRun g_exit_flag[%d] empty[%d] task_count[%d]", g_exit_flag, empty, task_count );
	
	return !empty;

}

static int ClosePipeAndDestroyThreadpool( struct HttpserverEnv *p_env )
{
	int		nret;
	
	/*���յ��˳�����رռ���*/
	close( p_env->listen_session.netaddr.sock );
	p_env->listen_session.netaddr.sock = -1;
	
	epoll_ctl( p_env->epoll_fd_recv , EPOLL_CTL_DEL , p_env->p_pipe_session[g_process_index].pipe_fds[0] , NULL );
	FATALLOGSG( "epoll_ctl_recv[%d] del pipe_session[%d] errno[%d]" , p_env->epoll_fd_recv , p_env->p_pipe_session[g_process_index].pipe_fds[0] , errno );
	close( p_env->p_pipe_session[g_process_index].pipe_fds[0] );
	p_env->p_pipe_session[g_process_index].pipe_fds[0] = -1;
		
	/*���������˳�,�ȴ�����������ɣ��߳̽���*/
	INFOLOGSG("pid[%ld] begin threadpool_destroy", getpid() );
	nret = threadpool_destroy( p_env->p_threadpool );
	if( nret == THREADPOOL_HAVE_TASKS )
	{
		FATALLOGSG("thread have tasks exception exit" );
		return -1;
	}
	else if( nret )
	{
		ERRORLOGSG("threadpool_destroy error errno[%d]", errno );
		return -1;
	}
	
	p_env->p_threadpool = NULL;
	INFOLOGSG("pid[%ld] threadpool_destroy OK", getpid() );
	
	return 0;
}

int worker( struct HttpserverEnv *p_env )
{
	struct epoll_event	event ;
	struct epoll_event	events[ MAX_EPOLL_EVENTS ] ;
	int			epoll_nfds = 0 ;
	struct epoll_event	*p_event = NULL ;
	struct PipeSession	*p_pipe_session = NULL ;
	struct AcceptedSession	*p_accepted_session = NULL ;
	struct ProcStatus 	*p_status = NULL;
	
	int			i = 0 ;
	int			nret = 0 ;
	
	nret = InitWorkerEnv( p_env );
	if( nret )
		return -1;
	INFOLOGSG("InitWorkerEnv ok");
	
	/* ��������ܵ��ɶ��¼���epoll */
	memset( & event , 0x00 , sizeof(struct epoll_event) );
	event.events = EPOLLIN | EPOLLERR ;
	event.data.ptr = & ( p_env->p_pipe_session[g_process_index] ) ;
	nret = epoll_ctl( p_env->epoll_fd_recv , EPOLL_CTL_ADD , p_env->p_pipe_session[g_process_index].pipe_fds[0] , &event );
	if( nret == -1 )
	{
		ERRORLOGSG( "EPOLL_CTL_ADD pipe_session EPOLLIN failed g_process_index[%d] pipe [%d][%d]  errno[%d]", g_process_index,
			p_env->p_pipe_session[g_process_index].pipe_fds[0], p_env->p_pipe_session[g_process_index].pipe_fds[1], errno );
		close( p_env->epoll_fd_recv );
		p_env->epoll_fd_recv = -1;
		g_exit_flag = 1;
		return -1;
	}
	else
	{
		INFOLOGSG( "g_process_index[%d] epoll_ctl[%d] add pipe_session[%d] EPOLLIN", g_process_index, p_env->epoll_fd_recv , p_env->p_pipe_session[g_process_index].pipe_fds[0] );
	}
	
	/*���յ��˳��ź�,�������пͻ����Ѿ��Ͽ����ӣ������̲Ž���*/
	while( IsRun( p_env ) )
	{
		errno = 0;
		
		p_status = (struct ProcStatus*)p_env->p_shmPtr;
		if( p_status[g_process_index].effect_exit_time > 0  )
		{	
			INFOLOGSG( "worker_%d pid[%d] effect_exit_time[%ld] ��⵽�˳���־�����̽��˳�", 
				g_process_index+1, p_status[g_process_index].pid, p_status[g_process_index].effect_exit_time );
			g_exit_flag = 1;
		}

		/* �ȴ�epoll�¼�������1�볬ʱ */
		memset( events , 0x00 , sizeof(events) );
		epoll_nfds = epoll_wait( p_env->epoll_fd_recv , events , MAX_EPOLL_EVENTS , 1000 ) ;		
		if( epoll_nfds == -1 )
		{
			if( errno == EINTR )
			{
				INFOLOGSG( "epoll_wait[%d] interrupted" , p_env->epoll_fd_recv );
				continue;
			}
			else
			{
				FATALLOGSG( "epoll_wait[%d] failed , errno[%d]" , p_env->epoll_fd_recv , ERRNO );
				g_exit_flag = 1;
			}
		}
		
		/* ���������¼� */
		INFOLOGSG( "epoll_wait_recv epoll_fd[%d] epoll_nfds[%d]" , p_env->epoll_fd_recv , epoll_nfds );
		for( i = 0 , p_event = events ; i < epoll_nfds ; i++ , p_event++ )
		{
			INFOLOGSG( "epoll_wait_recv current_index[%d] p_event->data.ptr[%p] p_env->listen_session[%p] " , i , p_event->data.ptr , & (p_env->listen_session) );

			/* ����ܵ��¼� */
			if( p_event->data.ptr == &( p_env->p_pipe_session[g_process_index] ) )
			{
				INFOLOGSG( "pipe_session" )

				p_pipe_session = (struct PipeSession *)(p_event->data.ptr) ;

				/* �ɶ��¼� */
				if( ( p_event->events & EPOLLIN ) )
				{
					nret = OnReceivePipe( p_env , p_pipe_session );
					if( nret < 0 )
					{
						FATALLOGSG( "OnProcessPipe failed[%d]", nret );
						nret = ClosePipeAndDestroyThreadpool( p_env );
						if( nret < 0 )
						{
							FATALLOGSG( "ClosePipeAndDestroyThreadpool failed[%d]", nret );
						}
						g_exit_flag = 1 ;
					}
					else if( nret > 0 )
					{
						ERRORLOGSG( "OnProcessPipe return[%d]" , nret );
						continue ;
					}
					else
					{
						DEBUGLOGSG( "OnProcessPipe ok" );
					}
				}
				/* �����¼� */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) )
				{			
					nret = ClosePipeAndDestroyThreadpool( p_env );
					if( nret < 0 )
					{
						FATALLOGSG( "ClosePipe failed[%d]", nret );
					}
					g_exit_flag = 1 ;
		
				}
				/* �����¼� */
				else
				{
					FATALLOGSG( "Unknow pipe session event[0x%X]" , p_event->events );
					g_exit_flag = 1 ;
				}
			}
			
			/* �����¼������ͻ������ӻỰ�¼� */
			else 
			{
				INFOLOGSG( "accepted_session" )
				p_accepted_session = (struct AcceptedSession *)(p_event->data.ptr) ;
				
				/* �ɶ��¼� */
				if( p_event->events & EPOLLIN )
				{
					nret = OnReceivingSocket( p_env , p_accepted_session ) ;
					if( nret < 0 )
					{
						FATALLOGSG( "OnReceivingSocket failed[%d]" , nret );
						g_exit_flag = 1 ;
					}
					else if( nret > 0 )
					{
						INFOLOGSG( "OnReceivingSocket return[%d] p_accepted_session[%p]" , nret, p_accepted_session );
						OnClosingSocket( p_env , p_accepted_session, TRUE );
					}
					else
					{
						DEBUGLOGSG( "OnReceivingSocket ok" );
					}
				}
				/* �����¼� */
				else if( ( p_event->events & EPOLLERR ) || ( p_event->events & EPOLLHUP ) || ( p_event->events & EPOLLRDHUP ) )
				{
					int		errCode;
					
					if( p_event->events & EPOLLERR )
						errCode = EPOLLERR;
					else if( p_event->events & EPOLLHUP )
						errCode = EPOLLHUP;
					else
						errCode = EPOLLRDHUP;
						
					ERRORLOGSG( "accepted_session EPOLLERR[%d] EPOLLHUP[%d] EPOLLRDHUP[%d] errno[%d]", EPOLLERR, EPOLLHUP, EPOLLRDHUP, errno );
					
					/*û�б��̳߳ص��ȵ�״̬���԰�ȫ�رգ�����״̬��Ҫ�ȴ��߳�ִ����ϲ��ܹر�*/
					if( p_accepted_session->status != SESSION_STATUS_PUBLISH )
					{
						ERRORLOGSG( "p_accepted_session[%p] errCode[%d] status[%d] fd[%d] errno[%d] ���Ա���ȫ�ر�", p_accepted_session, errCode, p_accepted_session->status, p_accepted_session->netaddr.sock , errno );
						OnClosingSocket( p_env , p_accepted_session, TRUE );
					}
					else
					{
						ERRORLOGSG( "accepted_session status[%d] fd[%d] EPOLLERR errno[%d]" , p_accepted_session->status, p_accepted_session->netaddr.sock , errno );
					}
					
					
				}
				/* �����¼� */
				else
				{
					FATALLOGSG( "Unknow accepted session event[0x%X]" , p_event->events );
					g_exit_flag = 1 ;
				}
			}
			
		}
	}
	
	/* �ر������׽��� */
	if( p_env->listen_session.netaddr.sock > 0 )
	{
		INFOLOGSG( "close listen[%d]" , p_env->listen_session.netaddr.sock );
		close( p_env->listen_session.netaddr.sock );
		p_env->listen_session.netaddr.sock = -1;
	}
	
	/* �ر�epoll */
	if( p_env->epoll_fd_recv > 0 )
	{
		INFOLOGSG( "close epoll_fd_recv[%d]" , p_env->epoll_fd_recv );
		close( p_env->epoll_fd_recv );
		p_env->epoll_fd_recv = -1;
	}
	if( p_env->epoll_fd_send > 0 )
	{
		INFOLOGSG( "close epoll_fd_send[%d]" , p_env->epoll_fd_send );
		close( p_env->epoll_fd_send );
		p_env->epoll_fd_send = -1;
	}
	shmdt( p_env->p_shmPtr );
	
	if( p_env->plugin_handle_input )
	{
		nret = p_env->p_fn_unload_input();
		if( nret )
		{
			ERRORLOGSG( "������ж��ʧ��nret[%d]", nret );
		}
		else
		{
			INFOLOGSG( "������ж�سɹ�" );
		}
		
		dlclose( p_env->plugin_handle_output );
		p_env->plugin_handle_output = NULL;
	}
	
	if( p_env->plugin_handle_output )
	{
		nret = p_env->p_fn_unload_output();
		if( nret )
		{
			ERRORLOGSG( "������ж��ʧ��nret[%d]", nret );
		}
		else
		{
			INFOLOGSG( "������ж�سɹ�" );
		}
		
		dlclose( p_env->plugin_handle_output );
		p_env->plugin_handle_output = NULL;
	}

	return 0;
}