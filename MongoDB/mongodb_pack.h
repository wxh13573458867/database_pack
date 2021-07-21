#ifndef MONGODB_PACK_H
#define MONGODB_PACK_H

#include <bson.h>
#include <bcon.h>
#include <mongoc.h>
#include <map>
#include <vector>
#include <mutex>
#include <string>

typedef  std::vector<std::string>& MongoDB_Out;

class MongoInit
{
private:
	MongoInit();
	~MongoInit();
public:
	static MongoInit* m_MongoInit;	//MongoDB初始化对象
	static std::mutex DBInit_mutex;	//访问锁
	static int initCount;			//连接计数,用于清理
	static void AddMongoInit();		//连接数+1,对象为NULL时创建
	static void SubMongoInit();		//连接数-1,连接数为0时清理
};

class MongoDB
{
public:
	MongoDB(std::string &url, std::string &database, std::string &muster);	
	~MongoDB();	
	
	//连接数据库
	bool Connect();
	//返回错误信息
	std::string retError();
	//插入数据
	bool DBIns(std::map<std::string, std::string> &ins_m);
	//删除数据
	bool DBDel(std::map<std::string, std::string> &del_m, int* rows = NULL);
	//修改数据
	bool DBMod(std::map<std::string, std::string> &old_m, std::map<std::string, std::string>& new_m, int* rows = NULL);
	//查询数据
	bool DBSel(std::map<std::string, std::string>& sel_m, void (*DBSelCallBack)(void *, MongoDB_Out), void* arg, int* rows = NULL);
	
	
private:
	mongoc_client_t *client;
	mongoc_collection_t *collection;
	bson_error_t db_error;
	std::mutex DB_mutex;	//访问锁
	std::string db_url;
	std::string db_database;
	std::string db_muster;
	bool is_Conn;
};

#endif
