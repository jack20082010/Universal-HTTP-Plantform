#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>

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
} ;

static long GetFileSize( char *filename )
{
	int		nret ;
	struct stat	stat_buf ;

	nret = stat( filename , & stat_buf ) ;
	if( nret == -1 )
		return -1 ;
	return stat_buf.st_size ;
}

static long gettvdifftime( struct timeval *begin , struct timeval *end )
{

	long cost_time = ( end->tv_sec - begin->tv_sec )*1000 + ( end->tv_usec - begin->tv_usec )/1000;
	     

	return cost_time ;
}

static long GetMillTime( struct timeval *tv_time )
{
	long		lmsec ;

	lmsec  = tv_time->tv_sec *1000 - tv_time->tv_usec/1000.0 ;
	
	return lmsec;

}

int perfmstat( char *filename  )
{
	long		filesize ;
	long		lret ;

	FILE			*fp = NULL ;
	struct Performamce	psd ;

	double			total_time = 0;
	double			deal_time = 0;
	double			recv_time = 0;
	double			publish_time = 0 ;
	double			send_time = 0;

	/* �����ļ���С */
	filesize = GetFileSize( filename ) ;
	if( filesize <= 0 )
	{
		printf( "��ȡ�ļ�[%s]��Сʧ��[%ld]\n" , filename , filesize );
		return -1 ;
	}

	if( filesize % sizeof(struct Performamce) != 0 )
	{
		printf( "�ļ���С[%ld]��Ч����λ��С[%ld]\n" , filesize , sizeof(struct Performamce) );
		return -1 ;
	}

	fp = fopen( filename , "rb" ) ;
	if( fp == NULL )
	{
		printf( "���ļ�[%s]���ڶ�ʧ��errno[%d]\n" , filename , errno );
		return -1 ;
	}

	while( 1 )
	{
		if( feof(fp) )
			break ;

		memset( & psd , 0x00 , sizeof(struct Performamce) );
		lret = fread( & psd , sizeof(struct Performamce) , 1 , fp ) ;
		if( lret <= 0 )
			break ;

		/* �����ܺ�ʱ */
		//total_time = gettvdifftime( & psd.tv_sdkSend , & psd.tv_send_end ) ;
		
		/*�����ܺ�ʱ */
		deal_time = gettvdifftime( & psd.tv_accepted , & psd.tv_send_end ) ;

		/* ���պ�ʱ */
		recv_time = gettvdifftime( & psd.tv_receive_begin , & psd.tv_receive_end ) ;

		/* ������ʱ */
		publish_time = gettvdifftime( & psd.tv_publish_begin , & psd.tv_publish_end ) ;

		/* ���ͺ�ʱ */
		send_time = gettvdifftime( & psd.tv_send_begin , & psd.tv_send_end ) ;


		printf( "tv_sdkSend:[%ld] ms\n", GetMillTime( &psd.tv_sdkSend ) );
		printf( "tv_accepted:[%ld] ms\n", GetMillTime( &psd.tv_accepted ) );
		printf( "tv_receive_begin:[%ld] ms\n", GetMillTime( &psd.tv_receive_begin ) );
		printf( "tv_receive_end:[%ld] ms\n", GetMillTime( &psd.tv_receive_end ) );
		printf( "tv_publish_begin:[%ld] ms\n", GetMillTime( &psd.tv_publish_begin ) );
		printf( "tv_publish_end:[%ld] ms\n", GetMillTime( &psd.tv_publish_end ) );
		printf( "tv_send_begin:[%ld] ms\n", GetMillTime( &psd.tv_send_begin ) );
		printf( "tv_send_end:[%ld] ms\n", GetMillTime( &psd.tv_send_end ) );
		printf( "total_time:%.3lfs | deal_tiem:%.3lfs | recv_tiem:%.3lfs | publish_time:%.3lfs | send_time:%.3lfs \n" , 
			total_time/1000.0 , deal_time/1000.0 , recv_time/1000.0 , publish_time/1000.0 , send_time/1000.0 ) ;

		printf( "--------------------\n" );
	}
	fclose(fp);
	
	return 0 ;
}


void usage()
{
	printf( "USAGE : perfmstat [ �������ݵ����ļ��� ]\n" );
	return ;
}

int main( int argc , char *argv[] )
{

	if( argc == 1 + 1 )
	{
		perfmstat( argv[1] ) ;
	}
	else
	{
		usage() ;
	}

	return 0 ;
}
