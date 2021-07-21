#ifndef MYSQL_PACK_H
#define MYSQL_PACK_H

#include <string>
#include <mysql/mysql.h>
#include <mutex>

class MySql
{
public:
	MySql(std::string &server, std::string &username, std::string &passwd, std::string &database);
	~MySql();

	//连接数据库
	bool Connect();
	//设置超时时间
	bool SetOvertime(long overtime);
	//查询数据
	bool SqlSel(std::string &cmd, void(SqlSelCallBack)(void *, MYSQL_RES *), void *arg);
	//执行sql语句
	bool SqlMod(std::string &cmd, int *rows = NULL);

private:
	bool is_Conn;
	MYSQL mysql;
	std::string sql_server;
	std::string sql_username;
	std::string sql_passwd;
	std::string sql_database;
	std::mutex sql_mutex;
};

#endif
