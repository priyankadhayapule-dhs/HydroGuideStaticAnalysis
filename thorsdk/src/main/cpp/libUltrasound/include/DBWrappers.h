#pragma once

#include <memory>
#include <sqlite3.h>

// A small wrapper around the sqlite* c interface to ensure that objects are cleaned up
// Tried using std::unique_ptr, but that didn't work as cleanly because sqlite3 interface takes lots
// of pointer-to-pointer types, which does not mesh well with std::unique_ptr interface
class Sqlite3
{
public:
	Sqlite3();
	~Sqlite3();

	// allow implicit casting to a sqlite3*
	// This also covers usage in an if statement (no need for an operator bool)
	operator sqlite3*();

	// Allow getting pointer-to-pointer type
	// This auto closes any existing db, as it assumes
	// that the db will be overwritten
	sqlite3** operator&();

	// explicit close
	void close();

private:
	sqlite3 *db;
};

class Sqlite3Stmt
{
	public:
	Sqlite3Stmt();
	~Sqlite3Stmt();

	// allow implicit casting to a sqlite3_stmt*
	// This also covers usage in an if statement (no need for an operator bool)
	operator sqlite3_stmt*();

	// Allow getting pointer-to-pointer type
	// This auto finalizes any existing stmt, as it assumes
	// that the stmt will be overwritten
	sqlite3_stmt** operator&();

	// explicit finalize
	void finalize();

private:
	sqlite3_stmt *stmt;
};