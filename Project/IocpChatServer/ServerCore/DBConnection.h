#pragma once
#include <sql.h>
#include <sqlext.h>

/*----------------
	DBConnection
-----------------*/

class DBConnection
{
public:
	bool			Connect(SQLHENV henv, const WCHAR* connectionString);
	void			Clear();

	// PreparedStatement를 위한 함수들
	bool			Prepare(const WCHAR* query);
	bool			Execute(); // 인자 없는 Execute 추가

	// 직접 쿼리 실행을 위한 기존 함수
	bool			Execute(const WCHAR* query);

	bool			Fetch();
	int32			GetRowCount();
	void			Unbind();

public:
	// 마지막 인자에 기본값 nullptr 추가하여 호출을 간편하게 만듦
	bool			BindParam(SQLUSMALLINT paramIndex, SQLSMALLINT cType, SQLSMALLINT sqlType, SQLULEN len, SQLPOINTER ptr, SQLLEN* index = nullptr);
	bool			BindCol(SQLUSMALLINT columnIndex, SQLSMALLINT cType, SQLULEN len, SQLPOINTER value, SQLLEN* index = nullptr);
	void			HandleError(SQLRETURN ret);

private:
	SQLHDBC			_connection = SQL_NULL_HANDLE;
	SQLHSTMT		_statement = SQL_NULL_HANDLE;
};