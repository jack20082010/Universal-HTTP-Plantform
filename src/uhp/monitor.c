/*
 * author	: jack
 * email	: xulongzhong2004@163.com
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#include <sys/un.h>
#include "uhp_in.h"

signed char			g_exit_flag = 0 ;
static sig_atomic_t		g_SIGTERM_flag = 0 ;
static sig_atomic_t		g_SIGUSR1_flag = 0 ;
int				g_process_index = PROCESS_INIT;

static void sig_set_flag( int sig_no )
{
	/* ���յ���ͬ�ź����ò�ͬ��ȫ�ֱ�־���Ӻ��������д��� */
	if( sig_no == SIGTERM )
	{
		g_SIGTERM_flag = 1 ; /* �˳� */
		INFOLOGSG( "�յ�ϵͳ�˳�����" );
	}
	else if( sig_no == SIGUSR1 )
	{
		g_SIGUSR1_flag = 1 ; /* ������־���� */
	}
	
	INFOLOGSG( "signal g_SIGTERM_flag[%d]" , g_SIGTERM_flag );
	
	return;
}

static int FindProIndex( HttpserverEnv *p_env, pid_t pid )
{
	int			i ;
	int			index = -1;
	struct ProcStatus 	*p_status = NULL;

	p_status = (struct ProcStatus*)p_env->p_shmPtr;
	for( i = 0; i < p_env->httpserver_conf.httpserver.server.processCount; i++, p_status++ )
	{
		if( p_status->pid > 0 && p_status->pid == pid )
		{
			index = i;
			break;
		}
	}

	return index;
}

static int CloseInheritAlivePipe( HttpserverEnv *p_env, int proc_index, BOOL bStart )
{
	int 	i;
	int 	max_index;
	
	if( bStart )
		max_index = proc_index;
	else
		max_index = p_env->httpserver_conf.httpserver.server.processCount;
		
	/*����ʱֻ��Ҫ�ر�indexС�ڵ�ǰ���*/
	for( i = 0; i < max_index; i++ )
	{	
		if( i == proc_index )
			continue;
		
		if( p_env->p_pipe_session[i].pipe_fds[0] > 0 )
		{
			close( p_env->p_pipe_session[i].pipe_fds[0] );
			p_env->p_pipe_session[i].pipe_fds[0] = -1;
		}
		if( p_env->p_pipe_session[i].pipe_fds[1] > 0 )
		{
			close( p_env->p_pipe_session[i].pipe_fds[1] );
			p_env->p_pipe_session[i].pipe_fds[1] = -1;
		}
		
	}
	
	/*�رձ����̹ܵ���д��������*/
	if( p_env->p_pipe_session[proc_index].pipe_fds[1] >0 )
	{
		close( p_env->p_pipe_session[proc_index].pipe_fds[1] );
		p_env->p_pipe_session[proc_index].pipe_fds[1] = -1;
	}
	
	return 0;
}

static int bind_socket( HttpserverEnv *p_env, int proc_index )
{
	int 			socket_domain;
	int			socket_protocol;
	int			nret = 0 ;
	
	
	socket_domain = AF_INET;
	socket_protocol = IPPROTO_TCP;
	p_env->listen_session.netaddr.sock = socket( socket_domain , SOCK_STREAM , socket_protocol ) ;
	if( p_env->listen_session.netaddr.sock == -1 )
	{
		ERRORLOGSG( "socket_domain[%d] socket failed , IPPROTO_TCP[%d] errno[%d] " , socket_domain, socket_protocol, errno );
		return -1;
	}	
	INFOLOGSG( "socket_domain[%d] socket ok[%d] protocol[%d]" , socket_domain, p_env->listen_session.netaddr.sock ,socket_protocol);
	
	/*SetHttpNonblock( p_env->listen_session.netaddr.sock );*/
	SetHttpReuseAddr( p_env->listen_session.netaddr.sock );
	SetHttpNodelay( p_env->listen_session.netaddr.sock , 1 );
	
	/*�ӽ������ö˿����� linux�ں�3.9����֧��*/
	if( proc_index > -1 )
	{
		INFOLOGSG( "child process [%d] SetHttpReusePort", proc_index+1 );
		SetHttpReusePort( p_env->listen_session.netaddr.sock );
	}
	
	/* ���׽��ֵ������˿� */
	nret = QueryNetIpByHostName( p_env->httpserver_conf.httpserver.server.ip , p_env->listen_session.netaddr.ip , sizeof(p_env->listen_session.netaddr.ip) );
	if( nret )
	{
		ERRORLOGSG( "QueryNetIpByHostName ip[%s] failed" , p_env->httpserver_conf.httpserver.server.ip );
		return nret;
	}
	
	nret = QueryNetPortByServiceName( p_env->httpserver_conf.httpserver.server.port , "tcp" , & (p_env->listen_session.netaddr.port) );
	if( nret )
	{
		ERRORLOGSG( "QueryNetPortByServiceName port[%s] failed" , p_env->httpserver_conf.httpserver.server.port );
		return nret;
	}

	if( p_env->listen_session.netaddr.port <= 0 )
	{
		ERRORLOGSG( "port[%d] invalid" , p_env->listen_session.netaddr.port  );
		close( p_env->listen_session.netaddr.sock );
		return -1;
	}
	SETNETADDRESS( p_env->listen_session.netaddr )
	nret = bind( p_env->listen_session.netaddr.sock , (struct sockaddr *) & (p_env->listen_session.netaddr.addr) , sizeof(struct sockaddr) ) ;
	if( nret == -1 )
	{
		ERRORLOGSG( "bind nret[%d] [%s:%d] socket[%d] failed , errno[%d]" , nret, p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock , errno );
		close( p_env->listen_session.netaddr.sock );
		return -1;
	}
	INFOLOGSG( "bind nret[%d] [%s:%d] socket[%d] ok" , nret, p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock );

	
	if( proc_index == -1 )
	{
		close( p_env->listen_session.netaddr.sock );
		p_env->listen_session.netaddr.sock = -1;
	}
	else
	{
		/* ��������״̬�� */
		nret = listen( p_env->listen_session.netaddr.sock , p_env->httpserver_conf.httpserver.server.listenBacklog ) ;
		if( nret == -1 )
		{
			ERRORLOGSG( "listen[%s:%d][%d] failed , errno[%d]" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock , errno );
			close( p_env->listen_session.netaddr.sock );
			return -1;
		}
		else
		{
			INFOLOGSG( "listen[%s:%d][%d] ok" , p_env->listen_session.netaddr.ip , p_env->listen_session.netaddr.port , p_env->listen_session.netaddr.sock );
		}
	}
	
	return 0;
	
}

static int ForkProcess( HttpserverEnv *p_env, int proc_index, BOOL bStart )
{
	pid_t			pid ;
	int			nret = 0 ;
	struct ProcStatus 	*p_status = NULL;
	struct timeval 		tv_now;
	
	/* ��������ܵ� */
	nret = pipe( p_env->p_pipe_session[proc_index].pipe_fds ) ;
	if( nret == -1 )
	{
		ERRORLOGSG( "proc_index[%d] pipe failed , errno[%d]" ,proc_index, errno );
		return -1;
	}
	else
	{
		INFOLOGSG( "proc_index[%d] pipe ok[%d][%d]" , proc_index, p_env->p_pipe_session[proc_index].pipe_fds[0], p_env->p_pipe_session[proc_index].pipe_fds[1] );
	}

	p_status = (struct ProcStatus*)p_env->p_shmPtr;
	/* ������������ */
	pid = fork() ;
	if( pid == -1 )
	{
		ERRORLOGSG( "fork failed , errno[%d]" , errno );
		return -1;
	}
	else if( pid == 0 )
	{
		signal( SIGTERM , SIG_DFL );
		
		g_process_index = proc_index;	
		/*�رռ̳н��̵Ĺܵ�������*/
		CloseInheritAlivePipe( p_env, proc_index, bStart );
		nret = bind_socket( p_env, proc_index );
		if( nret )
		{
			ERRORLOGSG( "bind_socket failed nret[%d] pid[%ld] ppid[%ld] g_process_index[%d]" , nret , getpid(), getppid(), g_process_index );
		}
		else
		{
			nret = worker( p_env ) ;
		}
		
		CleanEnvironment( p_env );
		INFOLOGSG( "worker process exit nret[%d] pid[%ld] ppid[%ld] g_process_index[%d]" , nret , getpid(), getppid(), g_process_index );
		exit(-nret);
	}
	else
	{
		/*!!!!!!ע�⣺���븸���̸�ֵ��frok��һ���첽���̣����ܴ����ӳ������
		����ӽ��̸�ֵ���������ϻ�ȡ���ݣ������pidΪ0���
		��¼�����ӽ���pid,���ں����ӽ����˳�����λindex*/
		memset( &( p_status[proc_index] ), 0, sizeof( struct ProcStatus ) );
		p_status[proc_index].process_index = proc_index;
		p_status[proc_index].pid = pid;
		gettimeofday( &tv_now, NULL );
		p_status[proc_index].last_restarted_time = tv_now.tv_sec;

		close( p_env->p_pipe_session[proc_index].pipe_fds[0] );
		p_env->p_pipe_session[proc_index].pipe_fds[0] = -1;
		SetHttpNonblock( p_env->p_pipe_session[proc_index].pipe_fds[1] );
		
		INFOLOGSG( "parent_pid[%d]  worker_index[%d] child_pid[%d]" , getpid() ,proc_index, pid );
	}
	
	return 0;
}
static int CreateBulletinBoard( HttpserverEnv *p_env )
{
	int	processCount;

	processCount = p_env->httpserver_conf.httpserver.server.processCount;
	p_env->shmid = shmget( 0, processCount * sizeof( struct ProcStatus ) , IPC_CREAT|IPC_EXCL|0666 );
	if( p_env->shmid == -1 )
	{
		ERRORLOGSG("shmget error errno=%d,EEXIST=%d\n", errno, EEXIST );
		return -1;
	}
		
	p_env->p_shmPtr = (char*)shmat( p_env->shmid, 0, 0 );
	if( (long)p_env->p_shmPtr == -1 )
	{
		ERRORLOGSG("shmat error");
		return -1;
	}
	memset( p_env->p_shmPtr, 0, processCount * sizeof( struct ProcStatus ) ); 
	
	return 0;
}

static int ShowProcStatus( HttpserverEnv *p_env )
{
	int 			i;
	struct ProcStatus 	*p_status = NULL;
	struct timeval 		tv_now;
	char			*restartWhen = NULL;    /*����ִ������*/
	int			hourWhen = -1;
	int			minWhen = -1;
	struct tm		tm_time;
	struct tm		tm_restarted; 
	char			cur_date[10+1];
	char			last_restarted_date[10+1];
	char			*p_find = NULL;
	char			tmp[20+1];

	memset( tmp, 0, sizeof(tmp) );
	restartWhen = p_env->httpserver_conf.httpserver.server.restartWhen;
	p_find = strstr( restartWhen, ":");
	if( p_find )	
	{	
		strncpy( tmp, restartWhen, p_find - restartWhen );
		hourWhen = atoi( tmp );
		minWhen = atoi( p_find+1 );	
	}	
	
	/*�������������,��ѡһ����������*/
	gettimeofday( &tv_now, NULL );
	localtime_r( &( tv_now.tv_sec ), &tm_time );
	memset( cur_date, 0, sizeof(cur_date) );
	strftime( cur_date, sizeof(cur_date), "%Y-%m-%d", &tm_time );
	
	p_status = (struct ProcStatus*)(p_env->p_shmPtr);
	for( i = 0; i < p_env->httpserver_conf.httpserver.server.processCount; i++, p_status++ )
	{
		
		localtime_r( &( p_status->last_restarted_time ), &tm_restarted );
		memset( last_restarted_date, 0, sizeof(last_restarted_date) );
		strftime( last_restarted_date, sizeof(last_restarted_date), "%Y-%m-%d", &tm_restarted );
		
		DEBUGLOGSG("worker_%d pid[%ld] cur_date[%s] last_restarted_date[%s] tm_hour[%d] tm_min[%d] hourWhen[%d] minWhen[%d] effect_exit_time[%ld]",
			i+1, p_status->pid, cur_date, last_restarted_date, tm_time.tm_hour, tm_time.tm_min, hourWhen, minWhen, p_status->effect_exit_time );
			
		if( p_status->effect_exit_time > 0  )
		{
			if( ( tv_now.tv_sec - p_status->effect_exit_time ) > p_env->httpserver_conf.httpserver.server.maxChildProcessExitTime )
			{
				ERRORLOGSG(" ����worker_%d pid[%d]������ʱ�����������ǿɱ" , i+1, p_status->pid );
				kill( p_status->pid, SIGKILL );
				/*ʱ�������1��Сʱ��ʹ�ϱ��ж��߼���Ч��Ŀ�ľ��ǿ���ֻ��һ��kill�ź�*/
				p_status->effect_exit_time += 3600; 
			}
		}
		else
		{
			if( strcmp( last_restarted_date, cur_date ) < 0 
				&& tm_time.tm_hour == hourWhen && tm_time.tm_min >= minWhen )
			{
				p_status->effect_exit_time = tv_now.tv_sec; /*��д������־�����ӽ��̽��������˳�*/
				INFOLOGSG("����worker_%d pid[%d] ��ʱ:%s ������Ϊ������־", i+1, p_status->pid, restartWhen );
				break;
			}
		}
		
		
		DEBUGLOGSG( "process[%d] "
			"task[%d] "
			"working[%d] "
			"idle[%d] "
			"thread[%d] "
			"timeout[%d] "
			"min[%d] "
			"max[%d] "
			"percent[%d] "
			"session[%d] "
			"last_restarted_time[%ld] "
			"effect_exit_time[%ld] "
			 , i+1
			 , p_status->task_count
			 , p_status->working_count
			 , p_status->Idle_count 
			 , p_status->thread_count	
			 , p_status->timeout_count
			 , p_status->min_threads
			 , p_status->max_threads
			 , p_status->taskTimeoutPercent
			 , p_status->session_count
			 , p_status->last_restarted_time
			 , p_status->effect_exit_time
			  );	
			  		 
	}
		
	return 0;
}

static int ReloadLog( HttpserverEnv *p_env )
{
	char		ch = 'L' ;
	httpserver_conf	httpserver_conf ;
	int 		i;
	
	int		nret;

	memset( & httpserver_conf , 0x00 , sizeof(httpserver_conf) );
	nret = LoadConfig( p_env , &httpserver_conf );
	if( nret )
	{
		ERRORLOGSG( "LoadConfig error" );
		return -1;
	}
	
	memcpy( &p_env->httpserver_conf , &httpserver_conf , sizeof(httpserver_conf) );
		
	for( i = 0; i < p_env->httpserver_conf.httpserver.server.processCount; i++ )
	{			
		nret = (int)write( p_env->p_pipe_session[i].pipe_fds[1] , & ch , 1 );
		if( nret == -1 )
		{
			ERRORLOGSG( "process_index[%d] write pipe[%d] failed , errno[%d]" , i, p_env->p_pipe_session[i].pipe_fds[1], errno );
			return -1;
		}
		
		INFOLOGSG( "process_index[%d] write pipe[%d] ok", i, p_env->p_pipe_session[i].pipe_fds[1] );

	}
	
	nret = InitLogEnv( p_env, "monitor", SERVER_AFTER_LOADCONFIG );
	if( nret )
	{
		ERRORLOGSG( "InitLogEnv failed[%d]" , nret );
		return -1;
	}

	return 0;
}

static int DealSigterm( HttpserverEnv *p_env )
{
	struct ProcStatus 	*p_status = NULL;
	struct timeval 		tv_now;
	int			i;
	
	gettimeofday( &tv_now, NULL );
	p_status = (struct ProcStatus*)(p_env->p_shmPtr);
	for( i = 0; i < p_env->httpserver_conf.httpserver.server.processCount; i++, p_status++ )
	{	
		if( p_status->effect_exit_time > 0  )
		{
			if( ( tv_now.tv_sec - p_status->effect_exit_time ) > p_env->httpserver_conf.httpserver.server.maxChildProcessExitTime )
			{
				ERRORLOGSG(" ����worker_%d pid[%d]�˳���ʱ�����������ǿɱ" , i+1, p_status->pid );
				kill( p_status->pid, SIGKILL );
				/*ʱ�������1��Сʱ��ʹ�ϱ��ж��߼���Ч��Ŀ�ľ��ǿ���ֻ��һ��kill�ź�*/
				p_status->effect_exit_time += 3600; 
			}
		}
		else
		{
			p_status->effect_exit_time = tv_now.tv_sec; /*��д�˳���־�����ӽ��̽��������˳�*/
			INFOLOGSG("����worker_%d pid[%d] ������Ϊ�˳���־", i+1, p_status->pid  );
			
		}
	}
	
	DEBUGLOGSG("DealSigterm childProcessMaxExitTime[%d]", p_env->httpserver_conf.httpserver.server.maxChildProcessExitTime );
	
	return 0;
}

static int monitor( HttpserverEnv *p_env )
{
	struct sigaction	act ;	
	pid_t			pid ;
	int			status ;
	int			i;
	int			*p_processCount = NULL;
	struct ProcStatus 	*p_status = NULL;
	int			nret = 0 ;

	nret = bind_socket( p_env, -1 );
	if( nret )
	{
		ERRORLOGSG( "monitor bind_socket failed nret[%d]", nret ); 
		return nret;
	}
	
	/* �����ź� */
	signal( SIGCLD , SIG_DFL );
	signal( SIGCHLD , SIG_DFL );
	signal( SIGINT , SIG_IGN );
	signal( SIGPIPE , SIG_IGN );
	
	memset( &act, 0, sizeof(struct sigaction) );
	act.sa_handler = & sig_set_flag ;
	sigemptyset( & (act.sa_mask) );
	act.sa_flags = 0 ;
	sigaction( SIGTERM , & act , NULL );
	act.sa_flags = SA_RESTART ;
	signal( SIGCLD , SIG_DFL );
	sigaction( SIGUSR1 , & act , NULL );
	sigaction( SIGUSR2 , & act , NULL );
	
	p_processCount = &( p_env->httpserver_conf.httpserver.server.processCount );
	if( *p_processCount <= 0 )
		*p_processCount = 1;
	
	if( *p_processCount > SERVER_MAX_PROCESS )
	{
		ERRORLOGSG( "���ý�����[%d]���ܴ���[%d]" , *p_processCount, SERVER_MAX_PROCESS );
		return -1;
	}
	
	/*�����ģʽ���������ڴ湫��壬����ͨ���������鿴*/
	nret = CreateBulletinBoard( p_env );
	if( nret )
	{
		ERRORLOGSG("CreateBulletinBoard failed nret[%d]", nret );
		return -1;
	}
	
	p_env->p_pipe_session = (struct PipeSession*)malloc( sizeof(struct PipeSession)*(*p_processCount) );
	memset( p_env->p_pipe_session, 0, sizeof(struct PipeSession)*(*p_processCount) );

	for( i = 0; i < *p_processCount; i++ )
	{
		nret = ForkProcess( p_env, i , TRUE );
		if( nret )
		{
			close( p_env->listen_session.netaddr.sock );
			return -1;
		}
	}
	
	/*��ʾ�ܵ�״̬��Ϣ*/
	p_status = (struct ProcStatus*)p_env->p_shmPtr;
	for( i = 0; i < *p_processCount; i++,p_status++ )
	{
		INFOLOGSG( "loop_index[%d] pid[%d] pipe_fd0[%d] pipe_fd1[%d]" , i, p_status->pid,
				p_env->p_pipe_session[i].pipe_fds[0], p_env->p_pipe_session[i].pipe_fds[1] );
	}	
	
	while( 1 )
	{
		/* ���������̽����źţ�û���򷵻�0 */
		pid = waitpid( -1 , & status , WNOHANG ) ;
		if( pid < 0 )
		{
			if( errno == ECHILD )
			{
				INFOLOGSG( "û�й���������" );
				break;
			}
			else
			{
				ERRORLOGSG( "���������̽����ź�ʧ��[%d]errno[%d]" , pid , errno );
				return -1;
			}
		}
		else if( pid == 0 )
		{
			/* ������־���� */
			if( g_SIGUSR1_flag )
			{
				g_SIGUSR1_flag = 0 ;
				
				nret = ReloadLog( p_env );
				if( nret )
				{
					ERRORLOGSG( "LoadConfig error" );
					continue;
				}
				
				INFOLOGSG( "ReloadLog Config ok" );
				
			}
			else
			{
				if( g_SIGTERM_flag == 1 )
				{
					/*���յ��˳��źţ����ý����˳���־���ӽ��������˳�*/
					DealSigterm( p_env );
				}
				else
				{
					struct timeval	now_time;
		
					gettimeofday( &now_time, NULL );
					if( ( now_time.tv_sec - p_env->last_show_status_timestamp ) >= p_env->httpserver_conf.httpserver.server.showStatusInterval )
					{
						ShowProcStatus( p_env );
						p_env->last_show_status_timestamp = now_time.tv_sec ;
						
						INFOLOGSG( "child process running ok!" )
					}
				}
				
				sleep(1);
			
			}
		}
		else
		{
			int			proIndex ;
						
			proIndex = FindProIndex( p_env, pid );
			if( proIndex < 0 )
			{
				ERRORLOGSG("FindProIndex failed pid[%d], proIndex[%d]", pid, proIndex );
				break;
			}
			
			/* ��鹤�������Ƿ��������� */
			if( WEXITSTATUS(status) == 0 && WIFSIGNALED(status) == 0 && WTERMSIG(status) == 0 )
			{
				INFOLOGSG( "worker_%d waitpid[%d] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d]" , proIndex+1, pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) );
			}
			else
			{
				ERRORLOGSG( "worker_%d waitpid[%d] WEXITSTATUS[%d] WIFSIGNALED[%d] WTERMSIG[%d]" , proIndex+1, pid , WEXITSTATUS(status) , WIFSIGNALED(status) , WTERMSIG(status) );
			}
			
			close( p_env->p_pipe_session[proIndex].pipe_fds[1] );
			p_env->p_pipe_session[proIndex].pipe_fds[1] = -1;
			
			/*�յ�ֹͣ�ź��˳����������ӽ���,���������ӽ���*/
			if( !g_SIGTERM_flag )
			{
				nret = ForkProcess( p_env, proIndex , FALSE );
				if( nret )
					break;
			}
			
		}
		
	}

	close( p_env->listen_session.netaddr.sock );
	
	return 0;
}

int _monitor( void *pv )
{
	HttpserverEnv	*p_env = (HttpserverEnv *)pv ;
	int			nret = 0 ;
	
	/* ������־���� */
	g_process_index = PROCESS_MONITOR;
	nret = InitLogEnv( p_env, "monitor", SERVER_AFTER_LOADCONFIG );
	if( nret )
	{
		//printf( "Nsporxy_InitLogEnv faild[%d]\n" , nret );
		return nret;
	}
	
	NOTICELOGSG( "--- httpserver.monitor begin --- " );
	
	/* ����monitor */
	nret = monitor( p_env ) ;
	
	/*�ػ������˳��ͷ�pic������Դ*/
	shmdt( p_env->p_shmPtr );
	shmctl( p_env->shmid , IPC_RMID , NULL );
	
	NOTICELOGSG( "--- httpserver.monitor end --- pid[%ld] ppid[%ld]", getpid(), getppid() );
	
	/* ��������˳� */
	CleanEnvironment( p_env );
	exit(-nret);
}
