##	����
������ԭ��������΢�����ڸ��и�ҵ�㷺��Ӧ�ã�httpͨ��Э���Ѿ���Ϊ΢����ͨ�ŵ�����Э�顣���ÿ��Ӧ�ö��Լ������http���񣬴����ظ����裬�����Ѷȴ�ͺ���ά���ɱ��ߵ����⣬���Խ���һ��ͨ�ø�����http�����ܣ���α�д�����������ʵ�ָ�����Ӧ��http���񣬳�Ϊ��ҵĹ㷺����
##	Ŀ��
1. ʵ�ָ�����ͨ��http����ƽ̨������C/C++���������Ա��롣ʹ��linux epoll��·���ã�ʹ��reuseport���ԣ��ں˰汾����3.9���ϣ�����ӽ��̿��ü���ͬһ���˿ڣ�Դͷ�Ͻ���ɸ����̼����ӽ��̼̳е��µľ�Ⱥ����
2. accept��recv��send �̷ֿ߳���һ���������ӡ����ա��������ݵ����ܡ�
3. ֧�ֶ����+���̷߳�ʽ���������������epoll���ȴ�����ȱ�ݡ�
4. ��ʵ���̳߳أ���������ʱ���Զ������̳߳��е�һ�������߳̽���ҵ������������ӹ���ʱ���Զ���չ�߳�ֱ����������̣߳�����ʱ�����ͷŹ�ʣ�߳�ֱ��������С�̡߳�
5. ʵ�ָ��������ݿ���ʲ�������ñ�����ִ��sql�������ν���sql��֧��������ȡ���ݺ��������롣֧�����ݿ����ӳط��ʣ�֧�ֶ�̬��չ���ӳ��ͷš�
6. ��������httpͨ�ű�Ҫ�ֶμ��
7. ��������ʵ��ҵ�����߼�
8. ƽ̨��������api����������������������ƽ̨�����ĵ��á�
9. �Դ��������Ĳ����֧�ֶ༶����ܹ���������ɱ������Դ���У�����˿��Զ�̬�����ͻ������������߿ͻ������ܡ�

## ��ط�������

Uhp��ͨ�õ�http�����ܣ�Ҫʵ����ط���ֻ��Ҫ����������롢���������ɡ�
��׼����ӿڣ�
int Load(); // uhp����ʱ��̬���أ���ʼ�������Դ
int Unload();	// uhp�����˳�ʱж�ص��ã��ͷ���س�ʼ����Դ
int Doworker( struct AcceptedSession *p_session ); //uhp�̳߳ش����������ҵ��

## �������Ĳ��

�ڸ��ӷֲ�ʽϵͳ�У�������Ҫ�Դ��������ݺ���Ϣ����Ψһ��ʶ���������ŵ����Ľ��ڡ�֧�����������Ƶꡢè�۵�Ӱ�Ȳ�Ʒ��ϵͳ�У�
�����ս������������ݷֿ�ֱ����Ҫ��һ��ΨһID����ʶһ�����ݻ���Ϣ�����ݿ������ID��Ȼ�������������ر�һ����綩�������֡��Ż�ȯҲ����Ҫ��ΨһID����ʶ��
��ʱһ���ܹ�����ȫ��ΨһID��ϵͳ�Ƿǳ���Ҫ�ġ�����������Ʋ��÷���ˡ��ͻ��˶������ģʽ��ÿ�����ж���һ��������ͬ���з��ʻ���������������������еĲ������ܡ�
֧�ֵ��ʻ�ȡ��������ȡģʽ����Ӧ��ͬҵ������֧�ֿ���̨�޸Ļ����������ͻ������������������������е����ܡ�

## ����
�Ȱ�װmysql�ͻ��˿�����
```
yum install mysql-community-devel.x86_64
sh makeinstall.sh 
```
�����װ��$HOME/bin��$HOME/shbin��$HOME/include��$HOME/lib��Ŀ¼
## ��װ
* ��ӵ�ǰ�û��Ļ�������
```
PATH=$PATH:$HOME/.local/bin:$HOME/bin:$HOME/shbin
export PATH

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/lib
export LANG=zh_CN.GB18030
ulimit -c unlimited
```
* ���ɳ�ʼ�������ļ�uhp.conf
```
[dep_xlz@localhost etc]$ uhp -a init
WARN : /home/dep_xlz/etc exist !
/home/dep_xlz/etc/uhp.conf created

```

* uhp.conf�����ļ�˵��
```
{
	"httpserver" : 
	{
		"server" : 
		{
			"ip" : "192.168.56.129" ,
			"port" : "10608" ,
			"listenBacklog" : 1024 ,
			"processCount" : 1 ,        //uhp����������
			"restartWhen" : "" ,        //��ʱ����ʱ�� �磺"03:58"����ֹ������������ʱ�������ڴ�й©������
			"maxTaskcount" : 1000000 ,  //�����֧�����������
			"taskTimeoutPercent" : 95 , //ҵ����ʱ�߳�ռ�����̳߳صı������������95�������Զ���������ֹ�����ڽ���״̬
			"perfmsEnable" : 1 ,        //����������־������������ܷ���
			"totalTimeout" : 60 ,       //ҵ����ʱʱ��
			"keepAliveIdleTimeout" : 300 , //�����ӿ���ʱ����ʱ�䣬������ʱ�����˹ر�����
			"keepAliveMaxTime" : 1800 , //��������󱣳�ʵ�ʣ��������ʱ�䣬֪ͨ�ͻ��˶Ͽ����ӣ��������ӡ�
			"showStatusInterval" : 10 , //��ʱ�̹߳�����־���
			"maxChildProcessExitTime" : 10 , //�ػ��˳�ʱ���ȴ��ӽ������ʱ�䣬����ʱ�䱻ǿɱ����
			"maxHttpResponse" : 1024 , //http��Ӧ�������ڴ��С
			"pluginInputPath" : "/home/dep_xlz/lib/libsequence_input.so" , //������·��
			"pluginOutputPath" : "/home/dep_xlz/lib/libseqmysql_output.so" //������·��
		} ,
		"threadpool" : 
		{
			"minThreads" : 10 ,       //�����߳���
			"maxThreads" : 100 ,      //����߳���
			"taskTimeout" : 60 ,      //�̴߳���ʱʱ��
			"threadWaitTimeout" : 2 , //Ĭ��2�룬�̳߳��߳����ȴ�����ʱ�䣬��Ҫ������־������鿴�����߳��Ƿ��ڽ���״̬��
			"threadSeed" : 5          //�������ʱ�����߳����ӣ���������Ǹ����ӱ������������̡߳�
		} ,
		"log" : 
		{
			"rotate_size" : "10MB" ,  //��־ת����С
			"main_loglevel" : "INFO" , 
			"monitor_loglevel" : "INFO" , //�ػ�������־�ȼ�
			"worker_loglevel" : "INFO"    //����������־�ȼ�
		} ,
		"plugin" :   //�����ֶΣ���Ҫ�û����롢���������á������������Ĳ��Ϊ����
		{
			"int1" : 3306 ,         
			"int2" : 10 ,            //�첽��ȡ��С�̳߳ش�С
			"int3" : 100 ,           //�첽��ȡ����̳߳ش�С
			"int4" : 0 ,
			"int5" : 0 ,
			"int6" : 0 ,
			"str301" : "mysqldb" ,   //���ݿ��û���
			"str302" : "mysqldb" ,   //���ݿ�����
			"str501" : "192.168.56.129" , //���ݿ��ַ
			"str502" : "mysqldb" ,        //���ݿ�dbname
			"str801" : "" ,
			"str802" : "" ,
			"str1281" : "" ,
			"str1282" : "" ,
			"str2551" : "" ,
			"str2552" : ""
		}
	}
}

```
* uhp��������
```
[dep_xlz@localhost etc]$ uhp.sh start
server_conf_pathfilename[/home/dep_xlz/etc/uhp.conf]
uhp start ok
dep_xlz   46598      1  0 12:02 ?        00:00:00 uhp -f uhp.conf -a start
dep_xlz   46600  46598  2 12:02 ?        00:00:00 uhp -f uhp.conf -a start
```
* uhp����ֹͣ
```
[dep_xlz@localhost etc]$ uhp.sh stop
dep_xlz   41061      1  0 01:46 ?        00:00:03 uhp -f uhp.conf -a start
dep_xlz   41063  41061  0 01:46 ?        00:01:23 uhp -f uhp.conf -a start
cost time 1 41061 wait closing
cost time 2 41061 wait closing
uhp end ok
```