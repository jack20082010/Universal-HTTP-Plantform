/*
 * author	: jack
 * email	: xulongzhong2004@163.com
 * Licensed under the LGPL v2.1, see the file LICENSE in base directory.
 */

#ifndef _H_UHP_IN_
#define _H_UHP_IN_

#include <dirent.h>
#include <unistd.h>
#include <iconv.h>
#include <stdint.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <dlfcn.h>

#include "list.h"
#include "util.h"
#include "fasterjson.h"
#include "fasterhttp.h"

#include "threadpool.h"
#include "seqerr.h"
#include "IDL_httpserver_conf.dsc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _TYPEDEF_BOOL_
#define _TYPEDEF_BOOL_
typedef int BOOL;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef BOOLNULL
#define BOOLNULL -1
#endif

#define HTTP_HEADER_CONTENT_TYPE_TEXT           "application/text;charset=gb18030"
#define	HTTP_HEADER_CONTENT_TYPE_JSON		"application/json"
#define SERVER_CONF_FILENAME			"uhp.conf"
#define SERVER_BEFORE_LOADCONFIG		1
#define SERVER_AFTER_LOADCONFIG			0
#define SERVER_MAX_PROCESS			1024
#define SERVER_MAX_MESSAGE_SIZE			1024*1024*4
#define SERVER_MAX_TOPIC_LEN			255

#define	SERVER_FULL_FUSING_TIME			10	/*���л���ȫ�۶�ʱ��*/
#define SERVER_FUSING_TIME			60	/*��̨�����۶�ʱ��*/
#define MAX_HOST_LEN	 			100

extern char		*g_ibp_httpserver_kernel_version ;
extern signed char 	g_exit_flag;
extern int 		g_process_index;

#define PROCESS_INIT			-2	/*���̳�ʼֵ*/
#define PROCESS_MONITOR			-1	/*monitor�ػ����̣�����Ϊ�ӽ���*/

/* ÿ��������epoll�¼��� */
#define MAX_EPOLL_EVENTS		50000
#define MAX_RESPONSE_BODY_LEN		300

#define RETRY_INDEX			0
#define ACCEPT_INDEX			1
#define SEND_EPOLL_INDEX		2

/* proxy��sdk֮�������URI */
#define URI_PRODUCER			"/sequence"  
#define URI_SESSIONS			"/sessions"  
#define URI_THREAD_STATUS		"/thread_status"  
#define URI_CONFIG			"/config"


/*�Ự״̬���̿���*/
#define SESSION_STATUS_RECEIVE		1
#define SESSION_STATUS_PUBLISH		2
#define SESSION_STATUS_SEND		3
#define SESSION_STATUS_DIE		4

/* �����Ự�ṹ	*/
struct ListenSession
{
	struct NetAddress	netaddr	;
} ;

/* �ܵ��Ự�ṹ	*/
struct PipeSession
{
	int			pipe_fds[ 2 ] ;
} ;

/*���ܵ����ṹ*/
struct Performamce
{
	struct timeval	tv_sdkSend;
	struct timeval	tv_accepted;
	struct timeval	tv_receive_begin;
	struct timeval	tv_receive_end;
	struct timeval	tv_publish_begin;
	struct timeval	tv_publish_end;
	struct timeval	tv_send_begin;
	struct timeval	tv_send_end;
};


/* �ͻ������ӻỰ�ṹ */
struct AcceptedSession
{
	struct NetAddress	netaddr;
	struct HttpEnv		*http ;
	struct list_head	this_node ;
	struct Performamce	perfms;
	int			status;
	int			needFree;	/*��Ҫ�ӳ�free*/
	long			request_begin_time;
	long			accept_begin_time;
	char			*http_rsp_body; /*http������Ӧ��*/
	int			body_len; /*http������Ӧ��*/
	void			*p_env;
	char			charset[10+1];
} ;

/* �����в����ṹ */
struct CommandParameter
{
	char			*_action ;
	char			*_show ;
	char			*_rotate_size ;
	char			*_main_loglevel	;
	char			*_monitor_loglevel ;
	char			*_worker_loglevel ;
	char			*_sqlcmd;
	char			*_sqlfile;
	
} ;

struct ThreadInfo
{
	pthread_t	thread_id;
	char		cmd;		/*���������߳���־���ص�*/
};

struct HostInfo
{
	char	ip[15];
	int	port;
	long	activeTimestamp;
};

#define PLUGIN_LOAD				"Load"
#define PLUGIN_UNLOAD				"Unload"
#define PLUGIN_DOWORKER				"Doworker"
typedef int fn_doworker( struct AcceptedSession* );
typedef int fn_void( );

/* proxy������������ṹ */
struct HttpserverEnv
{
	struct CommandParameter		cmd_param ;
	
	char				*server_conf_filename ; /* proxy.conf�ļ��� */
	char				server_conf_pathfilename[ UTIL_MAXLEN_FILENAME + 1 ] ; /* proxy.conf·���ļ��� */
	
	httpserver_conf			httpserver_conf ; /* httpserver������ */
	int				epoll_fd_recv ; /* �ڲ�IO��·����epoll������ */
	int				epoll_fd_send ; /* �ڲ�IO��·����epoll������ */
	char				*p_shmPtr;   /*�����ڴ��ַ*/
	int				shmid;		/*�����ڴ��ʶ*/

	pthread_mutex_t			session_lock;
	struct ListenSession		listen_session ; /* �����Ự */
	struct PipeSession		*p_pipe_session ; /* �ܵ��Ự */
	struct AcceptedSession		accepted_session_list ;	/* �ͻ������ӻỰ */
	size_t				session_count;
	threadpool_t			*p_threadpool;
	char				lastDeletedDate[10+1] ;
	struct ThreadInfo		thread_array[3]; /*0 RETRY_INDEX--�����̣߳�1 ACCEPT_INDEX--accept�߳� 2 SEND_EPOLL_INDEX--send epoll�߳�*/
	long				last_loop_session_timestamp; /*���һ����ѯ����sessionʱ���*/
	long				last_show_status_timestamp;  /*������ʾ״̬��ѯʱ��*/
	struct NetAddress		netaddr;
	
	void				*plugin_handle_input;		 /*���������*/
	fn_doworker			*p_fn_doworker_input;	
	fn_void				*p_fn_load_input;
	fn_void				*p_fn_unload_input;
	
	void				*plugin_handle_output;		 /*���������*/
	fn_doworker			*p_fn_doworker_output;	
	fn_void				*p_fn_load_output;
	fn_void				*p_fn_unload_output;		
	
	void				*p_reserver1;		/*�����ֶ�*/
	void				*p_reserver2;
	void				*p_reserver3;
	void				*p_reserver4;
	void				*p_reserver5;
	void				*p_reserver6;
	
} ;

struct ProcStatus
{
	int		process_index;
	pid_t		pid;
	int 		task_count;
	int 		working_count;
	int 		Idle_count;
	int 		timeout_count;
	int		thread_count;
	int		max_threads;
	int		min_threads;
	int		taskTimeoutPercent;
	int		session_count;
	time_t		last_restarted_time;   /*�������һ������ʱ��*/
	time_t		effect_exit_time;	/*���ý����˳���־ʱ��*/
	
};
	
/*
 * ���ù���ģ��
 */

int LoadConfig( struct HttpserverEnv *p_env, httpserver_conf *p_conf );

/*
 * ��������ģ��
 */

int InitEnvironment( struct HttpserverEnv **pp_env );
int CleanEnvironment( struct HttpserverEnv *p_env );
int InitLogEnv( struct HttpserverEnv *p_env, char* module_name, int before_loadConfig );
int cleanLogEnv();
int SetHttpserverEnv( struct HttpserverEnv* p_env );


/*
 * �������
 */

int _monitor( void *pv );

/*
 * ��������
 */

int worker( struct HttpserverEnv *p_env );

/*
 * ͨѶ��
 */

int OnAcceptingSocket( struct HttpserverEnv *p_env , struct ListenSession *p_listen_session );
void OnClosingSocket( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session, BOOL with_lock );
int OnReceivingSocket( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int OnSendingSocket( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int OnReceivePipe( struct HttpserverEnv *p_env , struct PipeSession *p_pipe_session );

/*
 * Ӧ�ò�
 */
int RetryProducer( struct HttpserverEnv *p_env );
int InitWorkerEnv( struct HttpserverEnv *p_env );
int InitConfigFiles( struct HttpserverEnv *p_env );
int OnProcess( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int OnProcessPipeEvent( struct HttpserverEnv *p_env , struct PipeSession *p_pipe_session );
int ThreadBegin( void *arg, int threadno );
int ThreadRunning( void *arg, int threadno );
int ThreadExit( void *arg, int threadno );
//elogLevel convert_sdkLogLevel( struct HttpserverEnv *p_env );
int OnProcessShowSessions( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int OnProcessShowThreadStatus( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int OnProcessShowConfig( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int ExportPerfmsFile( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int OnProcessGetTopicList( struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int ConvertReponseBody( struct AcceptedSession *p_accepted_session, char* body_convert, int body_size );

void FreeSession( struct AcceptedSession *p_accepted_session );
int AddEpollSendEvent(struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
int AddEpollRecvEvent(struct HttpserverEnv *p_env , struct AcceptedSession *p_accepted_session );
/*
 * �ͻ���
 */

int SetMqServers( struct HttpserverEnv *p_env );
int GetRemoteTopicList( struct HttpserverEnv *p_env );

int ShowSessions( struct HttpserverEnv *p_env );
int ShowThreadStatus( struct HttpserverEnv *p_env );
int ShowConfig( struct HttpserverEnv *p_env );
int ShowTopicList( struct HttpserverEnv *p_env );

#ifdef __cplusplus
}
#endif

#endif
