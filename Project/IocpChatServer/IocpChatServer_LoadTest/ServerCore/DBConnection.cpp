#include "pch.h"
#include "DBConnection.h"
#include <iostream>
#include <locale>
#include "Metrics.h" 


bool DBConnection::Connect(SQLHENV henv, const WCHAR* connectionString)
{
	// DB 연결 핸들 할당
	if (::SQLAllocHandle(SQL_HANDLE_DBC, henv, &_connection) != SQL_SUCCESS)
		return false;

	WCHAR stringBuffer[MAX_PATH] = { 0 };
	::wcscpy_s(stringBuffer, connectionString);

	WCHAR resultString[MAX_PATH] = { 0 };
	SQLSMALLINT resultStringLen = 0;

	// 실제 DB 연결 시도
	SQLRETURN ret = ::SQLDriverConnectW(
		_connection,
		NULL,
		reinterpret_cast<SQLWCHAR*>(stringBuffer),
		_countof(stringBuffer),
		OUT reinterpret_cast<SQLWCHAR*>(resultString),
		_countof(resultString),
		OUT & resultStringLen,
		SQL_DRIVER_NOPROMPT
	);

	// statement 핸들 할당
	if (::SQLAllocHandle(SQL_HANDLE_STMT, _connection, &_statement) != SQL_SUCCESS)
		return false;

	return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

void DBConnection::Clear()
{
	// 핸들 정리
	if (_statement != SQL_NULL_HANDLE)
	{
		::SQLFreeHandle(SQL_HANDLE_STMT, _statement);
		_statement = SQL_NULL_HANDLE;
	}

	if (_connection != SQL_NULL_HANDLE)
	{
		::SQLFreeHandle(SQL_HANDLE_DBC, _connection);
		_connection = SQL_NULL_HANDLE;
	}
}

bool DBConnection::Prepare(const WCHAR* query)
{
	// 쿼리 준비 (프리컴파일)
	SQLRETURN ret = ::SQLPrepareW(_statement, (SQLWCHAR*)query, SQL_NTSL);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		HandleError(ret);
		// GMetrics.db_prepare_fail.fetch_add(1, std::memory_order_relaxed); // Caller (DbLogger) will count this
		return false;
	}
	return true;
}

bool DBConnection::Execute()
{
	// 준비된 쿼리 실행
	SQLRETURN ret = ::SQLExecute(_statement);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		HandleError(ret);
		// GMetrics.db_exec_fail.fetch_add(1, std::memory_order_relaxed); // Caller (DbLogger) will count this
		return false;
	}

	// GMetrics.db_exec_ok.fetch_add(1, std::memory_order_relaxed); // Caller (DbLogger) will count this
	return true;
}

bool DBConnection::Execute(const WCHAR* query)
{
	// 쿼리 바로 실행 (Prepare 안하고 바로)
	SQLRETURN ret = ::SQLExecDirectW(_statement, (SQLWCHAR*)query, SQL_NTSL);
	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
	{
		GMetrics.db_exec_ok.fetch_add(1, std::memory_order_relaxed);
		return true;
	}

	HandleError(ret);
	GMetrics.db_exec_fail.fetch_add(1, std::memory_order_relaxed);
	return false;
}

bool DBConnection::Fetch()
{
	// 결과셋에서 다음 행 불러오기
	SQLRETURN ret = ::SQLFetch(_statement);

	switch (ret)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		GMetrics.db_fetch_ok.fetch_add(1, std::memory_order_relaxed);
		return true;
	case SQL_NO_DATA:
		GMetrics.db_fetch_no_data.fetch_add(1, std::memory_order_relaxed);
		return false;
	case SQL_ERROR:
		HandleError(ret);
		GMetrics.db_fetch_fail.fetch_add(1, std::memory_order_relaxed);
		return false;
	default:
		// 아직 실행 중인 경우(SQL_STILL_EXECUTING 등)
		return true;
	}
}

int32 DBConnection::GetRowCount()
{
	// 실행된 쿼리 결과 행 수 가져오기
	SQLLEN count = 0;
	SQLRETURN ret = ::SQLRowCount(_statement, OUT & count);

	if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
		return static_cast<int32>(count);

	return -1;
}

void DBConnection::Unbind()
{
	// 실행 이후 바인딩/커서 정리
	::SQLFreeStmt(_statement, SQL_CLOSE);
	::SQLFreeStmt(_statement, SQL_RESET_PARAMS);
	::SQLFreeStmt(_statement, SQL_UNBIND);

	// GMetrics.db_unbind_calls.fetch_add(1, std::memory_order_relaxed); // Caller (DbLogger) will count this
}

bool DBConnection::BindParam(SQLUSMALLINT paramIndex, SQLSMALLINT cType, SQLSMALLINT sqlType, SQLULEN len, SQLPOINTER ptr, SQLLEN* index)
{
	// 쿼리 파라미터 바인딩
	SQLRETURN ret = ::SQLBindParameter(_statement, paramIndex, SQL_PARAM_INPUT, cType, sqlType, len, 0, ptr, 0, index);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		HandleError(ret);
		return false;
	}

	return true;
}

bool DBConnection::BindCol(SQLUSMALLINT columnIndex, SQLSMALLINT cType, SQLULEN len, SQLPOINTER value, SQLLEN* index)
{
	// 결과 컬럼 바인딩
	SQLRETURN ret = ::SQLBindCol(_statement, columnIndex, cType, value, len, index);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		HandleError(ret);
		return false;
	}

	return true;
}

void DBConnection::HandleError(SQLRETURN ret)
{
	if (ret == SQL_SUCCESS)
		return;

	SQLSMALLINT index = 1;
	SQLWCHAR sqlState[MAX_PATH] = { 0 };
	SQLINTEGER nativeErr = 0;
	SQLWCHAR errMsg[MAX_PATH] = { 0 };
	SQLSMALLINT msgLen = 0;
	SQLRETURN errorRet = 0;

	while (true)
	{
		errorRet = ::SQLGetDiagRecW(
			SQL_HANDLE_STMT,
			_statement,
			index,
			sqlState,
			OUT & nativeErr,
			errMsg,
			_countof(errMsg),
			OUT & msgLen
		);

		if (errorRet == SQL_NO_DATA)
			break;

		if (errorRet != SQL_SUCCESS && errorRet != SQL_SUCCESS_WITH_INFO)
			break;

		// 에러 메시지 출력 (간단 로그)
		std::wcout.imbue(std::locale("kor"));
		std::wcout << L"DB Error: " << errMsg << L" (SQLState: " << sqlState << L", NativeErr: " << nativeErr << L")" << std::endl;

		index++;
	}
}

// =================================================================
// GIGACHAD ADDITIONS (FOR BATCHING)
// =================================================================

/**
 * \brief Set Auto-Commit mode ON(true) or OFF(false).
 * Turning OFF Auto-Commit is the start of a transaction.
 */
bool DBConnection::SetAutoCommit(bool autoCommit)
{
	SQLPOINTER mode = autoCommit ? (SQLPOINTER)SQL_AUTOCOMMIT_ON : (SQLPOINTER)SQL_AUTOCOMMIT_OFF;

	// Set auto-commit attribute on the *Connection* handle
	SQLRETURN ret = ::SQLSetConnectAttr(_connection, SQL_ATTR_AUTOCOMMIT, mode, SQL_IS_UINTEGER);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		// This HandleError uses the Statement handle, but the error is on the Connection
		// We need a way to check connection errors, but for now, just log and fail.
		std::wcout << L"DB Error: Failed to set AutoCommit mode." << std::endl;
		return false;
	}
	return true;
}

/**
 * \brief Commits the current transaction.
 */
bool DBConnection::Commit()
{
	// Commit the transaction on the *Connection* handle
	SQLRETURN ret = ::SQLEndTran(SQL_HANDLE_DBC, _connection, SQL_COMMIT);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		std::wcout << L"DB Error: Failed to Commit transaction." << std::endl;
		return false;
	}
	return true;
}

/**
 * \brief Rolls back the current transaction.
 */
bool DBConnection::Rollback()
{
	// Rollback the transaction on the *Connection* handle
	SQLRETURN ret = ::SQLEndTran(SQL_HANDLE_DBC, _connection, SQL_ROLLBACK);
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		std::wcout << L"DB Error: Failed to Rollback transaction." << std::endl;
		return false;
	}
	return true;
}