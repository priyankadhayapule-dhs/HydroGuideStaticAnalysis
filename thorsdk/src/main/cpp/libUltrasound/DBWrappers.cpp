#define LOG_TAG "DBWrappers"
#include <DBWrappers.h>
#include <ThorUtils.h>

Sqlite3::Sqlite3() : db(nullptr) {}
Sqlite3::~Sqlite3() { close(); }

Sqlite3::operator sqlite3* ()
{
	return db;
}

sqlite3** Sqlite3::operator&()
{
	// Assume that the caller is going to rewrite this db
	LOGI("Auto closing existing sqlite3 database at %p", db);
	close();
	return &db;
}

void Sqlite3::close()
{
	sqlite3_close(db);
	db = nullptr;
}

Sqlite3Stmt::Sqlite3Stmt() : stmt(nullptr) {}
Sqlite3Stmt::~Sqlite3Stmt() { finalize(); }

Sqlite3Stmt::operator sqlite3_stmt* ()
{
	return stmt;
}

sqlite3_stmt** Sqlite3Stmt::operator&()
{
	// Assume that the caller is going to rewrite this stmt
	LOGI("Auto finalizing existing sqlite3 statement at %p", stmt);
	finalize();
	return &stmt;
}

void Sqlite3Stmt::finalize()
{
	sqlite3_finalize(stmt);
	stmt = nullptr;
}