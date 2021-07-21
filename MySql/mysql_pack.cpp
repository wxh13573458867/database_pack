#include "mysql_pack.h"
#include <stdio.h>
#include <string.h>

MySql::MySql(std::string &server, std::string &username, std::string &passwd, std::string &database):
	sql_server(server), 
	sql_username(username), 
	sql_passwd(passwd), 
	sql_database(database){
	is_Conn = false;
}

MySql::~MySql(){
	if(is_Conn){
		mysql_close(&(this->mysql));
	}
}

//连接数据库
bool MySql::Connect(){
	if(is_Conn){
		return false;
	}

	mysql_init(&(this->mysql));

	if(mysql_real_connect(&(this->mysql),this->sql_server.c_str(), this->sql_username.c_str(), this->sql_passwd.c_str(), this->sql_database.c_str(), 0, NULL, 0) == NULL){
		return false;
	}else{
		char cmd[256] = {0};
		strcpy(cmd, "SET NAMES UTF8");
		int result = mysql_real_query(&(this->mysql), cmd, strlen(cmd));
		bool ret = (result == 0 ? true : false);
		if(!ret){
			mysql_close(&(this->mysql));
			return false;
		}
	}
	is_Conn = true;
	return true;
}

//设置超时时间
bool MySql::SetOvertime(long overtime){
	char cmd[256] = {0};
	sprintf(cmd, "SET GLOBAL INTERACTIVE_TIMEOUT = %ld;", overtime);
	int result = mysql_real_query(&(this->mysql), cmd, strlen(cmd));
	bool ret = (result == 0 ? true : false);
	return ret; 
}

//查询数据
bool MySql::SqlSel(std::string &cmd, void(SqlSelCallBack)(void *, MYSQL_RES *), void *arg)
{
	(this->sql_mutex).lock();
	int result = mysql_real_query(&(this->mysql), cmd.c_str(), cmd.size());
	bool ret = (result == 0 ? true : false);
	if(!ret){
		(this->sql_mutex).unlock();
		return false;
	}

	MYSQL_RES *mysql_res = NULL;
	if((mysql_res = mysql_store_result(&(this->mysql))) == NULL){
		(this->sql_mutex).unlock();
		return false;
	}
	SqlSelCallBack(arg, mysql_res);
	mysql_free_result(mysql_res);

	(this->sql_mutex).unlock();
	return true;
}
//执行sql语句
bool MySql::SqlMod(std::string &cmd, int *rows)
{
	(this->sql_mutex).lock();
	int result = mysql_real_query(&(this->mysql), cmd.c_str(), cmd.size());
	bool ret = (result == 0 ? true : false);
	if(!ret){
		(this->sql_mutex).unlock();
		return false;
	}
	
	if(rows != NULL){
		*rows = mysql_affected_rows(&(this->mysql));
	}

	(this->sql_mutex).unlock();
	return true;
}
