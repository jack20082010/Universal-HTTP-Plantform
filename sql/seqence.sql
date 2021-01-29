set names GBK;
DROP TABLE  IF EXISTS bs_seq_rule;
CREATE TABLE  bs_seq_rule
(
	id  BIGINT NOT NULL AUTO_INCREMENT  COMMENT '��ֵid' ,
	seq_name VARCHAR(64) NOT NULL COMMENT '��������' ,
	seq_desc VARCHAR(80) COMMENT '��������' ,
	seq_type INT DEFAULT 0 COMMENT 'Ĭ�Ϸ��ϸ񵥵�����' ,
	min_val  BIGINT NOT NULL  COMMENT '��Сֵ' ,
	max_val  BIGINT NOT NULL  COMMENT '���ֵ' ,
	cur_val BIGINT DEFAULT 0 NOT NULL  COMMENT '��ǰֵ' ,
	alert_val BIGINT DEFAULT 0 NOT NULL  COMMENT '�澯ֵ' ,
	step INT DEFAULT 1  COMMENT '����' ,
	alert_diff INT DEFAULT 500  COMMENT 'ʣ������澯',
	CACHE INT DEFAULT 1000 COMMENT '�ڴ滺�����' ,
	client_cache INT DEFAULT 100 COMMENT '�ͻ����ڴ滺�����',
	client_alert_diff INT DEFAULT 50  COMMENT 'ʣ������澯',
	batch_cache INT DEFAULT 10  COMMENT 'cache���г���,������ȡʹ��' ,
	batch_fetch INT DEFAULT 3   COMMENT 'ÿ����ȡ��������cache,������ȡʹ��' ,
	STATUS  INT DEFAULT 0 COMMENT 'Ĭ������' ,
	cycle INT DEFAULT 1 COMMENT 'Ĭ��ѭ��' ,
	create_time DATETIME NOT NULL DEFAULT NOW() COMMENT '����ʱ��' ,
	update_time DATETIME NOT NULL DEFAULT NOW() ON UPDATE NOW() COMMENT '�޸�ʱ��' ,
	remark  VARCHAR(100)  COMMENT '��ע' ,
	PRIMARY KEY (id) 
	
);


ALTER TABLE bs_seq_rule ADD UNIQUE ( seq_name );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test1",1,999999999999999999, 1 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val, step ) VALUES("seq_test2",1,999999999999999999, 1,2 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test3",1,999999999999999999, 1 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test4",1,999999999999999999, 1 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test5",1,999999999999999999, 1 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test6",1,999999999999999999, 1 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test7",1,999999999999999999, 1 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test8",1,999999999999999999, 1 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test9",1,999999999999999999, 1 );
insert into bs_seq_rule( seq_name, min_val, max_val, cur_val ) VALUES("seq_test10",1,999999999999999999, 1 );

DROP TABLE  IF EXISTS bs_seq_test;
CREATE TABLE  bs_seq_test
(
	id  BIGINT NOT NULL   COMMENT 'seqid' ,
	seq_name VARCHAR(32) NOT NULL COMMENT '��������' ,
	create_time DATETIME NOT NULL DEFAULT NOW() COMMENT '����ʱ��' ,
	update_time DATETIME NOT NULL DEFAULT NOW() ON UPDATE NOW() COMMENT '�޸�ʱ��' ,
	remark  VARCHAR(100)  COMMENT '��ע' ,
	PRIMARY KEY (id, seq_name) 
	
);
