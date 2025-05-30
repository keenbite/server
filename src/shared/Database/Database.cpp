/* 
 * Project: KeenCore
 * License: GNU General Public License v2.0 or later (GPL-2.0+)
 *
 * This file is part of KeenCore.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Originally based on MaNGOS (Massive Network Game Object Server)
 * Copyright (C) 2005-2025 MaNGOS project <https://getmangos.eu>
 */

#include "DatabaseEnv.h"
#include "Config/Config.h"
#include "Database/SqlOperations.h"
#include "GitRevision.h"

#include <ctime>
#include <iostream>
#include <fstream>
#include <memory>

#define MIN_CONNECTION_POOL_SIZE 1
#define MAX_CONNECTION_POOL_SIZE 16

struct DBVersion
{
    std::string dbname;
    std::string expected_version;
    std::string expected_structure;
    std::string minimal_expected_content; // Minimal because core can starts with some missing contents
    std::string description;
};

const DBVersion databaseVersions[COUNT_DATABASES] = {
    { "World", GitRevision::GetWorldDBVersion(), GitRevision::GetWorldDBStructure(), GitRevision::GetWorldDBContent(), GitRevision::GetWorldDBUpdateDescription() }, // DATABASE_WORLD
    { "Authserver", GitRevision::GetAuthDBVersion(), GitRevision::GetAuthDBStructure(), GitRevision::GetAuthDBContent(), GitRevision::GetAuthDBUpdateDescription() }, // DATABASE_AUTH
    { "Character", GitRevision::GetCharDBVersion(), GitRevision::GetCharDBStructure(), GitRevision::GetCharDBContent(), GitRevision::GetCharDBUpdateDescription() }, // DATABASE_CHARACTER
};

//////////////////////////////////////////////////////////////////////////
SqlPreparedStatement* SqlConnection::CreateStatement(const std::string& fmt)
{
    return new SqlPlainPreparedStatement(fmt, *this);
}

void SqlConnection::FreePreparedStatements()
{
    SqlConnection::Lock guard(this);

    size_t nStmts = m_holder.size();
    for (size_t i = 0; i < nStmts; ++i)
    {
        delete m_holder[i];
    }

    m_holder.clear();
}

SqlPreparedStatement* SqlConnection::GetStmt(uint32 nIndex)
{
    // resize stmt container
    if (m_holder.size() <= nIndex)
    {
        m_holder.resize(nIndex + 1, NULL);
    }

    SqlPreparedStatement* pStmt = NULL;

    // create stmt if needed
    if (m_holder[nIndex] == NULL)
    {
        // obtain SQL request string
        std::string fmt = m_db.GetStmtString(nIndex);
        MANGOS_ASSERT(fmt.length());
        // allocate SQlPreparedStatement object
        pStmt = CreateStatement(fmt);
        // prepare statement
        if (!pStmt->prepare())
        {
            MANGOS_ASSERT(false && "Unable to prepare SQL statement");
            return NULL;
        }

        // save statement in internal registry
        m_holder[nIndex] = pStmt;
    }
    else
    {
        pStmt = m_holder[nIndex];
    }

    return pStmt;
}

bool SqlConnection::ExecuteStmt(int nIndex, const SqlStmtParameters& id)
{
    if (nIndex == -1)
    {
        return false;
    }

    // get prepared statement object
    SqlPreparedStatement* pStmt = GetStmt(nIndex);
    // bind parameters
    pStmt->bind(id);
    // execute statement
    return pStmt->execute();
}

//////////////////////////////////////////////////////////////////////////
Database::~Database()
{
    StopServer();
}

bool Database::Initialize(const char* infoString, int nConns /*= 1*/)
{
    // Enable logging of SQL commands (usually only GM commands)
    // (See method: PExecuteLog)
    m_logSQL = sConfig.GetBoolDefault("LogSQL", false);
    m_logsDir = sConfig.GetStringDefault("LogsDir", "");
    if (!m_logsDir.empty())
    {
        if ((m_logsDir.at(m_logsDir.length() - 1) != '/') && (m_logsDir.at(m_logsDir.length() - 1) != '\\'))
        {
            m_logsDir.append("/");
        }
    }

    m_pingIntervallms = sConfig.GetIntDefault("MaxPingTime", 30) * (MINUTE * 1000);

    // create DB connections

    // setup connection pool size
    if (nConns < MIN_CONNECTION_POOL_SIZE)
    {
        m_nQueryConnPoolSize = MIN_CONNECTION_POOL_SIZE;
    }
    else if (nConns > MAX_CONNECTION_POOL_SIZE)
    {
        m_nQueryConnPoolSize = MAX_CONNECTION_POOL_SIZE;
    }
    else
    {
        m_nQueryConnPoolSize = nConns;
    }

    // create connection pool for sync requests
    for (int i = 0; i < m_nQueryConnPoolSize; ++i)
    {
        SqlConnection* pConn = CreateConnection();
        if (!pConn->Initialize(infoString))
        {
            delete pConn;
            return false;
        }

        m_pQueryConnections.push_back(pConn);
    }

    // create and initialize connection for async requests
    m_pAsyncConn = CreateConnection();
    if (!m_pAsyncConn->Initialize(infoString))
    {
        return false;
    }

    m_pResultQueue = new SqlResultQueue;

    InitDelayThread();
    return true;
}

void Database::StopServer()
{
    HaltDelayThread();

    delete m_pResultQueue;
    delete m_pAsyncConn;

    m_pResultQueue = NULL;
    m_pAsyncConn = NULL;

    for (size_t i = 0; i < m_pQueryConnections.size(); ++i)
    {
        delete m_pQueryConnections[i];
    }

    m_pQueryConnections.clear();
}

SqlDelayThread* Database::CreateDelayThread()
{
    assert(m_pAsyncConn);
    return new SqlDelayThread(this, m_pAsyncConn);
}

void Database::InitDelayThread()
{
    assert(!m_delayThread);

    // New delay thread for delay execute
    m_threadBody = CreateDelayThread();              // will deleted at m_delayThread delete
    m_TransStorage = new ACE_TSS<Database::TransHelper>();
    m_delayThread = new ACE_Based::Thread(m_threadBody);
}

void Database::HaltDelayThread()
{
    if (!m_threadBody || !m_delayThread)
    {
        return;
    }

    m_threadBody->Stop();                                   // Stop event
    m_delayThread->wait();                                  // Wait for flush to DB
    delete m_TransStorage;
    delete m_delayThread;                                   // This also deletes m_threadBody
    m_delayThread = NULL;
    m_threadBody = NULL;
    m_TransStorage=NULL;
}

void Database::ThreadStart()
{
}

void Database::ThreadEnd()
{
}

void Database::ProcessResultQueue()
{
    if (m_pResultQueue)
    {
        m_pResultQueue->Update();
    }
}

void Database::escape_string(std::string& str)
{
    if (str.empty())
    {
        return;
    }

    char* buf = new char[str.size() * 2 + 1];
    // we don't care what connection to use - escape string will be the same
    m_pQueryConnections[0]->escape_string(buf, str.c_str(), str.size());
    str = buf;
    delete[] buf;
}

SqlConnection* Database::getQueryConnection()
{
    int nCount = 0;

    if (m_nQueryCounter == long(1 << 31))
    {
        m_nQueryCounter = 0;
    }
    else
    {
        nCount = ++m_nQueryCounter;
    }

    return m_pQueryConnections[nCount % m_nQueryConnPoolSize];
}

void Database::Ping()
{
    const char* sql = "SELECT 1";

    {
        SqlConnection::Lock guard(m_pAsyncConn);
        delete guard->Query(sql);
    }

    for (int i = 0; i < m_nQueryConnPoolSize; ++i)
    {
        SqlConnection::Lock guard(m_pQueryConnections[i]);
        delete guard->Query(sql);
    }
}

bool Database::PExecuteLog(const char* format, ...)
{
    if (!format)
    {
        return false;
    }

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);

    if (res == -1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s", format);
        return false;
    }

    if (m_logSQL)
    {
        time_t curr;
        tm local;
        time(&curr);                                        // get current time_t value
        local = *(localtime(&curr));                        // dereference and assign
        char fName[128];
        sprintf(fName, "%04d-%02d-%02d_logSQL.sql", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);

        FILE* log_file;
        std::string logsDir_fname = m_logsDir + fName;
        log_file = fopen(logsDir_fname.c_str(), "a");
        if (log_file)
        {
            fprintf(log_file, "%s;\n", szQuery);
            fclose(log_file);
        }
        else
        {
            // The file could not be opened
            sLog.outError("SQL-Logging is disabled - Log file for the SQL commands could not be openend: %s", fName);
        }
    }

    return Execute(szQuery);
}

QueryResult* Database::PQuery(const char* format, ...)
{
    if (!format)
    {
        return NULL;
    }

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);

    if (res == -1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s", format);
        return NULL;
    }

    return Query(szQuery);
}

QueryNamedResult* Database::PQueryNamed(const char* format, ...)
{
    if (!format)
    {
        return NULL;
    }

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);

    if (res == -1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s", format);
        return NULL;
    }

    return QueryNamed(szQuery);
}

bool Database::Execute(const char* sql)
{
    if (!m_pAsyncConn)
    {
        return false;
    }

    SqlTransaction* pTrans = (*m_TransStorage)->get();
    if (pTrans)
    {
        // add SQL request to trans queue
        pTrans->DelayExecute(new SqlPlainRequest(sql));
    }
    else
    {
        // if async execution is not available
        if (!m_bAllowAsyncTransactions)
        {
            return DirectExecute(sql);
        }

        // Simple sql statement
        m_threadBody->Delay(new SqlPlainRequest(sql));
    }

    return true;
}

bool Database::PExecute(const char* format, ...)
{
    if (!format)
    {
        return false;
    }

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);

    if (res == -1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s", format);
        return false;
    }

    return Execute(szQuery);
}

bool Database::DirectPExecute(const char* format, ...)
{
    if (!format)
    {
        return false;
    }

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);

    if (res == -1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s", format);
        return false;
    }

    return DirectExecute(szQuery);
}

bool Database::BeginTransaction()
{
    if (!m_pAsyncConn)
    {
        return false;
    }

    // initiate transaction on current thread
    // currently we do not support queued transactions
    (*m_TransStorage)->init();
    return true;
}

bool Database::CommitTransaction()
{
    if (!m_pAsyncConn)
    {
        return false;
    }

    // check if we have pending transaction
    if (!(*m_TransStorage)->get())
    {
        return false;
    }

    // if async execution is not available
    if (!m_bAllowAsyncTransactions)
    {
        return CommitTransactionDirect();
    }

    // add SqlTransaction to the async queue
    m_threadBody->Delay((*m_TransStorage)->detach());
    return true;
}

bool Database::CommitTransactionDirect()
{
    if (!m_pAsyncConn)
    {
        return false;
    }

    // check if we have pending transaction
    if (!(*m_TransStorage)->get())
    {
        return false;
    }

    // directly execute SqlTransaction
    SqlTransaction* pTrans = (*m_TransStorage)->detach();
    pTrans->Execute(m_pAsyncConn);
    delete pTrans;

    return true;
}

bool Database::RollbackTransaction()
{
    if (!m_pAsyncConn)
    {
        return false;
    }

    if (!(*m_TransStorage)->get())
    {
        return false;
    }

    // remove scheduled transaction
    (*m_TransStorage)->reset();

    return true;
}

void PrintNormalYouHaveDatabaseVersion(std::string current_db_version, std::string current_db_structure, std::string current_db_content, std::string description)
{
    sLog.outString("  [A] You have database Version: %s", current_db_version.c_str());
    sLog.outString("                      Structure: %s", current_db_structure.c_str());
    sLog.outString("                        Content: %s", current_db_content.c_str());
    sLog.outString("                    Description: %s", description.c_str());
}

void PrintErrorYouHaveDatabaseVersion(std::string &current_db_version, std::string current_db_structure, std::string current_db_content, std::string description)
{
    sLog.outErrorDb("  [A] You have database Version: %s", current_db_version.c_str());
    sLog.outErrorDb("                      Structure: %s", current_db_structure.c_str());
    sLog.outErrorDb("                        Content: %s", current_db_content.c_str());
    sLog.outErrorDb("                    Description: %s", description.c_str());
}

void PrintNormalDatabaseVersionReferencedByCore(const DBVersion& core_db_requirements)
{
    sLog.outString("  [B] The core references last database Version: %s", core_db_requirements.expected_version.c_str());
    sLog.outString("                                      Structure: %s", core_db_requirements.expected_structure.c_str());
    sLog.outString("                                        Content: %s", core_db_requirements.minimal_expected_content.c_str());
    sLog.outString("                                    Description: %s", core_db_requirements.description.c_str());
}

void PrintErrorYouNeedDatabaseVersionExpectedByCore(const DBVersion& core_db_requirements)
{
    sLog.outErrorDb("  [B] The core needs database Version: %s", core_db_requirements.expected_version.c_str());
    sLog.outErrorDb("                            Structure: %s", core_db_requirements.expected_structure.c_str());
    sLog.outErrorDb("                              Content: %s", core_db_requirements.minimal_expected_content.c_str());
    sLog.outErrorDb("                          Description: %s", core_db_requirements.description.c_str());
}

bool Database::CheckDatabaseVersion(DatabaseTypes database)
{
    const DBVersion& core_db_requirements = databaseVersions[database];

    // Fetch the database version table information
    QueryResult* result = Query("SELECT `version`, `structure`, `content`, `description` FROM `db_version` ORDER BY `version` DESC, `structure` DESC, `content` DESC LIMIT 1");

    // db_version table does not exist or is empty
    if (!result)
    {
        sLog.outErrorDb("The table `db_version` in your [%s] database is missing or corrupt.", core_db_requirements.dbname.c_str());
        sLog.outErrorDb();
        sLog.outErrorDb("  [A] You have database Version: MaNGOS can not verify your database version or its existence!");
        sLog.outErrorDb();
        PrintErrorYouNeedDatabaseVersionExpectedByCore(core_db_requirements);
        sLog.outErrorDb();
        sLog.outErrorDb("Please verify your database location or your database integrity.");

        // The core loading will no go further :
        return false;
    }

    Field* fields = result->Fetch();
    std::string current_db_version = fields[0].GetCppString();
    std::string current_db_structure = fields[1].GetCppString();
    std::string current_db_content = fields[2].GetCppString();
    std::string description = fields[3].GetCppString();

    delete result;

    // Structure does not match the required version
    if (current_db_version != core_db_requirements.expected_version || current_db_structure != core_db_requirements.expected_structure)
    {
        sLog.outErrorDb("The table `db_version` indicates that your [%s] database does not match the expected structure!", core_db_requirements.dbname.c_str());
        sLog.outErrorDb();
        PrintErrorYouHaveDatabaseVersion(current_db_version, current_db_structure, current_db_content, description);
        sLog.outErrorDb();
        PrintErrorYouNeedDatabaseVersionExpectedByCore(core_db_requirements);
        sLog.outErrorDb();
        sLog.outErrorDb("You must apply all updates after [A] to [B] to use MaNGOS with this database.");
        sLog.outErrorDb("These updates are included in the database/%s/Updates folder.", core_db_requirements.dbname.c_str());
        return false;
    }

    bool db_vs_core_content_version_mismatch = false;

    // DB is not up to date, but structure is correct.
    // The 'content' version in the 'db_version' table can be < from the one required by the core
    // See  enum values for :
    //  WORLD_DB_CONTENT_NR
    //  CHAR_DB_CONTENT_NR
    //  AUTHSERVER_DB_CONTENT_NR
    // for more information.
    if (current_db_content < core_db_requirements.minimal_expected_content)
    {
        // TODO : Should not display with error color but warning (e.g YELLOW) => Create a sLog.outWarningDb() and sLog.outWarning()
        sLog.outErrorDb("You have not updated the core for few DB [%s] updates!", core_db_requirements.dbname.c_str());
        sLog.outErrorDb("Current DB content is %s, core expects %s", current_db_content.c_str(), core_db_requirements.minimal_expected_content.c_str());
        sLog.outErrorDb("It is recommended to run ALL database updates up to the required core version.");
        sLog.outErrorDb("These updates are included in the database/%s/Updates folder.", core_db_requirements.dbname.c_str());
        sLog.outErrorDb("This is ok for now but should not last long.");
        db_vs_core_content_version_mismatch = true;
    }

    // Do not alert if current_db_content > core_db_requirements.minimal_expected_content it can mislead newcomers !

    // In anys cases if there are differences in content : output a recap of the differences :
    if (db_vs_core_content_version_mismatch)
    {
        // TODO : Should not display with error color but warning (e.g YELLOW) => Create a sLog.outWarningDb() and sLog.outWarning()
        sLog.outErrorDb("The table `db_version` indicates that your [%s] database does not match the expected version!", core_db_requirements.dbname.c_str());
        sLog.outErrorDb();
        PrintErrorYouHaveDatabaseVersion(current_db_version, current_db_structure, current_db_content, description);
        sLog.outErrorDb();
        PrintErrorYouNeedDatabaseVersionExpectedByCore(core_db_requirements);
    }
    else
    {
        if (current_db_version == core_db_requirements.expected_version && current_db_structure == core_db_requirements.expected_structure)
        {
            sLog.outString("The table `db_version` indicates that your [%s] database has the same version as the core requirements.", core_db_requirements.dbname.c_str());
            sLog.outString();
        }
        else
        {
            sLog.outString("The table `db_version` indicates that your [%s] database has a higher version than the one referenced by the core."
                "\nYou have probably applied DB updates, and that's a good thing to keep your server up to date.", core_db_requirements.dbname.c_str());
            sLog.outString();
            PrintNormalYouHaveDatabaseVersion(current_db_version, current_db_structure, current_db_content, description);
            sLog.outString();
            PrintNormalDatabaseVersionReferencedByCore(core_db_requirements);
            sLog.outString();
            sLog.outString("You can run the core without any problem like that.");
            sLog.outString();
        }
    }

    return true;
}

bool Database::ExecuteStmt(const SqlStatementID& id, SqlStmtParameters* params)
{
    if (!m_pAsyncConn)
    {
        return false;
    }

    SqlTransaction* pTrans = (*m_TransStorage)->get();
    if (pTrans)
    {
        // add SQL request to trans queue
        pTrans->DelayExecute(new SqlPreparedRequest(id.ID(), params));
    }
    else
    {
        // if async execution is not available
        if (!m_bAllowAsyncTransactions)
        {
            return DirectExecuteStmt(id, params);
        }

        // Simple sql statement
        m_threadBody->Delay(new SqlPreparedRequest(id.ID(), params));
    }

    return true;
}

bool Database::DirectExecuteStmt(const SqlStatementID& id, SqlStmtParameters* params)
{
    MANGOS_ASSERT(params);
    std::shared_ptr<SqlStmtParameters> p(params);
    // execute statement
    SqlConnection::Lock _guard(getAsyncConnection());
    return _guard->ExecuteStmt(id.ID(), *params);
}

SqlStatement Database::CreateStatement(SqlStatementID& index, const char* fmt)
{
    int nId = -1;
    // check if statement ID is initialized
    if (!index.initialized())
    {
        // convert to lower register
        std::string szFmt(fmt);
        // count input parameters
        int nParams = std::count(szFmt.begin(), szFmt.end(), '?');
        // find existing or add a new record in registry
        LOCK_GUARD _guard(m_stmtGuard);
        MANGOS_ASSERT(_guard.locked());
        PreparedStmtRegistry::const_iterator iter = m_stmtRegistry.find(szFmt);
        if (iter == m_stmtRegistry.end())
        {
            nId = ++m_iStmtIndex;
            m_stmtRegistry[szFmt] = nId;
        }
        else
        {
            nId = iter->second;
        }

        // save initialized statement index info
        index.init(nId, nParams);
    }

    return SqlStatement(index, *this);
}

std::string Database::GetStmtString(const int stmtId) const
{
    if (stmtId == -1 || stmtId > m_iStmtIndex)
    {
        return std::string();
    }

    LOCK_GUARD _guard(m_stmtGuard);
    if (_guard.locked())
    {
        PreparedStmtRegistry::const_iterator iter_last = m_stmtRegistry.end();
        for (PreparedStmtRegistry::const_iterator iter = m_stmtRegistry.begin(); iter != iter_last; ++iter)
        {
            if (iter->second == stmtId)
            {
                return iter->first;
            }
        }
    }
    return std::string();
}

// HELPER CLASSES AND FUNCTIONS
Database::TransHelper::~TransHelper()
{
    reset();
}

SqlTransaction* Database::TransHelper::init()
{
    MANGOS_ASSERT(!m_pTrans);   // if we will get a nested transaction request - we MUST fix code!!!
    m_pTrans = new SqlTransaction;
    return m_pTrans;
}

SqlTransaction* Database::TransHelper::detach()
{
    SqlTransaction* pRes = m_pTrans;
    m_pTrans = NULL;
    return pRes;
}

void Database::TransHelper::reset()
{
    delete m_pTrans;
    m_pTrans = NULL;
}
