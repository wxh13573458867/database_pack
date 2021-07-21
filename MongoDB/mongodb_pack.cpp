#include "mongodb_pack.h"
#include <bcon.h>
#include <bson.h>
#include <mongoc.h>

int MongoInit::initCount = 0;
std::mutex MongoInit::DBInit_mutex;
MongoInit * MongoInit::m_MongoInit = NULL;

MongoInit::MongoInit()	
{
	mongoc_init();
}
MongoInit::~MongoInit()
{
	mongoc_cleanup();
}

void MongoInit::AddMongoInit()
{
	DBInit_mutex.lock();
	initCount++;
	if (m_MongoInit == NULL) {
		m_MongoInit = new MongoInit();
	}
	DBInit_mutex.unlock();
	return;
}
void MongoInit::SubMongoInit() {
	DBInit_mutex.lock();
	if (--initCount == 0) {
		delete m_MongoInit;
		m_MongoInit = NULL;
	}
	DBInit_mutex.unlock();
	return ;
}

MongoDB::MongoDB(std::string &url, std::string &database, std::string &muster):\
		db_url(url), db_database(database), db_muster(muster)
{
	MongoInit::AddMongoInit();
	is_Conn = false;	
}

MongoDB::~MongoDB() 
{
	if (is_Conn) {
		mongoc_collection_destroy(collection);
		mongoc_client_destroy(client);
	}
	MongoInit::SubMongoInit();
	return;
}

bool MongoDB::Connect() {
	if (is_Conn) {
		return false;
	}
	this->client = mongoc_client_new(this->db_url.c_str());
	if(!this->client){
		return false;
	}
	this->collection = mongoc_client_get_collection(this->client, this->db_database.c_str(), this->db_muster.c_str());
	if (!this->collection) {
		return false;
	}
	is_Conn = true;
	return true;
}

std::string MongoDB::retError() 
{
	std::string strError = this->db_error.message;
	return strError;
}

bool MongoDB::DBIns(std::map<std::string, std::string>& ins_m) 
{
	this->DB_mutex.lock();
	bool ret = false;

	bson_t *doc = bson_new();
	bson_oid_t oid;
	bson_oid_init(&oid, NULL);
	BSON_APPEND_OID(doc, "_id", &oid);
	for (auto it = ins_m.begin(); it != ins_m.end(); ++it) {
		BSON_APPEND_UTF8(doc, it->first.c_str(), it->second.c_str());
	}

	if (mongoc_collection_insert(this->collection, MONGOC_INSERT_NONE, doc, NULL, &this->db_error)) {
		ret = true;
	}

	bson_destroy(doc);
	this->DB_mutex.unlock();
	return ret;
}

bool MongoDB::DBDel(std::map<std::string, std::string>& del_m, int* rows)
{
	this->DB_mutex.lock();
	bool ret = false;
	int count = 0;

	bson_t* doc = NULL;
	bson_t* query = bson_new();
	for (auto it = del_m.begin(); it != del_m.end(); ++it) {
		BSON_APPEND_UTF8(query, it->first.c_str(), it->second.c_str());
	}
	mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(this->collection, query, NULL, NULL);

	while (mongoc_cursor_next(cursor, (const bson_t**)&doc)) {
		if (mongoc_collection_remove(this->collection, MONGOC_REMOVE_SINGLE_REMOVE, doc, NULL, &this->db_error)) {
			ret = true;
			count++;
		}
		bson_destroy(doc);
	}
	if (rows) {
		*rows = count;
	}

	bson_destroy(query);
	mongoc_cursor_destroy(cursor);
	this->DB_mutex.unlock();
	return ret;
}

bool MongoDB::DBMod(std::map<std::string, std::string>& old_m, std::map<std::string, std::string>& new_m, int *rows)
{
	this->DB_mutex.lock();
	bool ret = false;
	int count = 0;

	bson_t* query = bson_new();
	bson_t* result = NULL;
	bson_t* doc = NULL;
	for(auto it = old_m.begin(); it != old_m.end(); ++it){
		BSON_APPEND_UTF8(query, it->first.c_str(), it->second.c_str());
	}
	mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(this->collection, query, NULL, NULL);

	while (mongoc_cursor_next(cursor, (const bson_t**)&doc)) {
		bool is_Mod = true;
		for (auto it = new_m.begin(); it != new_m.end(); ++it) {
			result = BCON_NEW("$set", "{", it->first.c_str(), BCON_UTF8(it->second.c_str()), "}");
			if (!mongoc_collection_update(this->collection, MONGOC_UPDATE_NONE, doc, result, NULL, &this->db_error)) {
				is_Mod = false;
			}
			bson_destroy(result);
		}
		if (is_Mod) {
			ret = true;
			count++;
		}
		bson_destroy(doc);
	}
	if (rows) {
		*rows = count;
	}

	bson_destroy(query);
	mongoc_cursor_destroy(cursor);
	this->DB_mutex.unlock();
	return ret;
}

bool MongoDB::DBSel(std::map<std::string, std::string>& sel_m, void (*DBSelCallBack)(void*, MongoDB_Out), void* arg, int * rows)
{
	this->DB_mutex.lock();
	bool ret = false;
	int count = 0;

	bson_t* doc = NULL;
	bson_t* query = bson_new();
	for (auto it = sel_m.begin(); it != sel_m.end(); ++it) {
		BSON_APPEND_UTF8(query, it->first.c_str(), it->second.c_str());
	}
	mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(this->collection, query, NULL, NULL);

	std::vector<std::string> out_v;
	char* str_temp = NULL;
	while (mongoc_cursor_next(cursor, (const bson_t**)&doc)) {
		str_temp = bson_as_json(doc, NULL);
		std::string str_next = str_temp;
		out_v.push_back(str_next);

		bson_free(str_temp);
		bson_destroy(doc);
		count++;
		ret = true;
	}	
	if (rows) {
		*rows = count;
	}
	bson_destroy(query);
	mongoc_cursor_destroy(cursor);
	this->DB_mutex.unlock();

	DBSelCallBack(arg, out_v);
	return ret;
}
