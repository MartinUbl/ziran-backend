#include "database.h"
#include "controller.h"

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std::string_literals;

CDatabase_Handler::CDatabase_Handler()
{
	//
}

CDatabase_Handler::~CDatabase_Handler()
{
	// close DB/statements if needed
	Close();
}

bool CDatabase_Handler::Close()
{
	auto stmt_close = [](MYSQL_STMT*& stmt) {
		if (stmt)
		{
			mysql_stmt_close(stmt);
			stmt = nullptr;
		}
	};

	// terminate statements at first
	stmt_close(mStmt_Get_Job_By_Name);
	stmt_close(mStmt_Set_Job_Status);
	stmt_close(mStmt_Set_Job_Output);
	stmt_close(mStmt_Add_Job_Report);

	// terminate connection
	if (mConnection)
	{
		mysql_close(mConnection);
		mConnection = nullptr;
		return true;
	}

	return false;
}

bool CDatabase_Handler::Connect(const std::string& host, uint16_t port, const std::string& username, const std::string& password, const std::string& dbname, std::vector<std::string>& log)
{
	log.push_back("MySQL client version: "s + mysql_get_client_info());

	mConnection = mysql_init(nullptr);

	if (!mConnection)
	{
		log.push_back("Unable to create connection handle");
		return false;
	}

	// connect - this may block
	if (!mysql_real_connect(mConnection, host.c_str(), username.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0))
	{
		log.push_back(mysql_error(mConnection));
		mysql_close(mConnection);
		return false;
	}

	return true;
}

bool CDatabase_Handler::Init_Statements(std::vector<std::string>& log)
{
	// generic function for preparing statement
	auto prepare = [&](const std::string& statement) -> MYSQL_STMT* {

		MYSQL_STMT* stmt = mysql_stmt_init(mConnection);
		if (!stmt)
		{
			log.push_back("Could not create prepared statement");
			return nullptr;
		}

		int result = mysql_stmt_prepare(stmt, statement.c_str(), static_cast<unsigned long>(statement.size()));
		if (result != 0)
		{
			log.push_back("Could not prepare statement: " + statement);
			return nullptr;
		}

		return stmt;
	};

	// internal "template" of prepared statement
	struct TPrepared_Statement_Template
	{
		MYSQL_STMT*& stmt;
		const std::string query;
	};

	// vector of statements to be prepared
	const std::vector<TPrepared_Statement_Template> Prepared_Statements = {
		{ mStmt_Get_Job_By_Name, "SELECT id, name, status, output, pipeline_id FROM job WHERE name = ?" },
		{ mStmt_Set_Job_Status, "UPDATE job SET status = ? WHERE id = ?" },
		{ mStmt_Set_Job_Output, "UPDATE job SET output = ? WHERE id = ?"},
		{ mStmt_Add_Job_Report, "INSERT INTO job_report (job_id, log_type, log_identifier, value) VALUES (?, ?, ?, ?)" }
	};

	// prepare all
	for (auto& b : Prepared_Statements)
		b.stmt = prepare(b.query);

	// and check if everything went OK
	for (auto& b : Prepared_Statements)
	{
		if (!b.stmt)
			return false;
	}

	return true;
}

bool CDatabase_Handler::Load_DB_Plugins(std::vector<std::string>& log)
{
	// one-shot query to load plugins
	if (mysql_query(mConnection, "SELECT id, guid FROM plugin"))
	{
		log.push_back("Could not execute query");
		log.push_back(mysql_error(mConnection));
		return false;
	}

	MYSQL_RES* result = mysql_store_result(mConnection);

	if (result == NULL)
	{
		log.push_back("Could not store query result");
		return false;
	}

	size_t pluginCnt = 0;

	bool ok = true;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		int id = std::stoi(row[0]);
		std::string guidStr = row[1];

		GUID plugin_guid = String_To_GUID(guidStr, ok);
		if (ok)
		{
			mDatabase_Plugins[id] = plugin_guid;
			pluginCnt++;
		}
		else
			log.push_back("Invalid GUID in database: " + guidStr);
	}

	mysql_free_result(result);

	log.push_back("Loaded " + std::to_string(pluginCnt) + " plugins");

	return true;
}

bool CDatabase_Handler::Load_DB_Global_Config(std::vector<std::string>& log, CDefault_Environment& env)
{
	// one-shot query to load global config items
	if (mysql_query(mConnection, "SELECT name, value FROM global_config"))
	{
		log.push_back("Could not execute query");
		log.push_back(mysql_error(mConnection));
		return false;
	}

	MYSQL_RES* result = mysql_store_result(mConnection);

	if (result == NULL)
	{
		log.push_back("Could not store query result");
		return false;
	}

	size_t cfgCnt = 0;

	bool ok = true;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		std::string key = row[0];
		std::string val = row[1];

		if (!key.empty() && !val.empty())
		{
			env.Set_String(key, val);
			cfgCnt++;
		}
		else
			log.push_back("Invalid global config entry in database: " + key);
	}

	mysql_free_result(result);

	log.push_back("Loaded " + std::to_string(cfgCnt) + " global configurations");

	return true;
}

bool CDatabase_Handler::Load_DB_Inputs(std::vector<std::string>& log, CDefault_Environment& env)
{
	// one-shot query to load inputs
	if (mysql_query(mConnection, "SELECT name, content FROM inputs"))
	{
		log.push_back("Could not execute query");
		log.push_back(mysql_error(mConnection));
		return false;
	}

	MYSQL_RES* result = mysql_store_result(mConnection);

	if (result == NULL)
	{
		log.push_back("Could not store query result");
		return false;
	}

	size_t cfgCnt = 0;

	bool ok = true;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		std::string key = row[0];
		std::string val = row[1];

		if (!key.empty() && !val.empty())
		{
			env.Set_Input(key, val);
			cfgCnt++;
		}
		else
			log.push_back("Invalid input entry in database: " + key);
	}

	mysql_free_result(result);

	log.push_back("Loaded " + std::to_string(cfgCnt) + " input data");

	return true;
}

bool CDatabase_Handler::Load_DB_Pipelines(std::vector<std::string>& log)
{
	if (mysql_query(mConnection, "SELECT id, name FROM pipeline"))
	{
		log.push_back("Could not execute query");
		return false;
	}

	MYSQL_RES* result = mysql_store_result(mConnection);

	if (result == NULL)
	{
		log.push_back("Could not store query result");
		return false;
	}

	size_t pipelineCnt = 0;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		int id = std::stoi(row[0]);
		std::string nameStr = row[1];

		mDatabase_Pipelines[id] = nameStr;
		pipelineCnt++;
	}

	mysql_free_result(result);

	log.push_back("Loaded " + std::to_string(pipelineCnt) + " pipelines");

	return true;
}

bool CDatabase_Handler::Load_DB_Pipeline_Items(std::vector<std::string>& log)
{
	if (mysql_query(mConnection, "SELECT id, pipeline_id, plugin_id, position, parameters FROM pipeline_item"))
	{
		log.push_back("Could not execute query");
		log.push_back("Error: "s + mysql_error(mConnection));
		return false;
	}

	MYSQL_RES* result = mysql_store_result(mConnection);

	if (result == NULL)
	{
		log.push_back("Could not store query result");
		return false;
	}

	size_t pipelineItemCnt = 0;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)))
	{
		TPipeline_Item_Record rec;

		rec.id = std::stoi(row[0]);
		rec.pipeline_id = std::stoi(row[1]);
		rec.plugin_id = std::stoi(row[2]);
		rec.order = std::stoi(row[3]);
		rec.parameters = Parse_Parameters(row[4] ? row[4] : "", log);

		mDatabase_Pipeline_Items[rec.pipeline_id].push_back(rec);
		pipelineItemCnt++;
	}

	mysql_free_result(result);

	// sort items by their order
	for (auto& r : mDatabase_Pipeline_Items)
	{
		auto& vec = r.second;

		std::sort(vec.begin(), vec.end(), [](const TPipeline_Item_Record& a, const TPipeline_Item_Record& b) { return a.order < b.order; });
	}

	log.push_back("Loaded " + std::to_string(pipelineItemCnt) + " pipeline items");

	return true;
}

TJob_Record CDatabase_Handler::Get_Job_By_Name(const std::string& job_name)
{
	MYSQL_BIND param_bind;
	MYSQL_BIND bind[5];
	MYSQL_FIELD* fields;
	MYSQL_RES* metaData;
	my_bool isnull[5];

	TJob_Record jobrec;
	char namebuf[64], statusbuf[16], outputbuf[16];
	unsigned long namelen, statuslen, outputlen;

	metaData = mysql_stmt_result_metadata(mStmt_Get_Job_By_Name);
	if (!metaData)
		return { Invalid_Job_Id };

	unsigned long parLen = static_cast<unsigned long>(job_name.size());
	memset(&param_bind, 0, sizeof(MYSQL_BIND));
	param_bind.buffer_type = MYSQL_TYPE_STRING;
	param_bind.buffer = const_cast<char*>(job_name.c_str());
	param_bind.is_null = 0;
	param_bind.buffer_length = parLen;
	param_bind.length = &parLen;

	fields = mysql_fetch_fields(metaData);
	memset(bind, 0, sizeof(bind));
	bind[0].buffer_type = fields[0].type;
	bind[0].buffer = &jobrec.id;
	bind[0].is_null = &isnull[0];

	bind[1].buffer_type = fields[1].type;
	bind[1].buffer = namebuf;
	bind[1].buffer_length = sizeof(namebuf);
	bind[1].length = &namelen;
	bind[1].is_null = &isnull[1];

	bind[2].buffer_type = fields[2].type;
	bind[2].buffer = statusbuf;
	bind[2].buffer_length = sizeof(statusbuf);
	bind[2].length = &statuslen;
	bind[2].is_null = &isnull[2];

	bind[3].buffer_type = fields[3].type;
	bind[3].buffer = outputbuf;
	bind[3].buffer_length = sizeof(outputbuf);
	bind[3].length = &outputlen;
	bind[3].is_null = &isnull[3];

	bind[4].buffer_type = fields[4].type;
	bind[4].buffer = &jobrec.pipeline_id;
	bind[4].is_null = &isnull[4];

	auto result = mysql_stmt_bind_param(mStmt_Get_Job_By_Name, &param_bind);
	if (result != 0)
		return { Invalid_Job_Id };

	result = mysql_stmt_bind_result(mStmt_Get_Job_By_Name, bind);
	if (result != 0)
		return { Invalid_Job_Id };

	result = mysql_stmt_execute(mStmt_Get_Job_By_Name);
	if (result != 0)
		return { Invalid_Job_Id };

	result = mysql_stmt_fetch(mStmt_Get_Job_By_Name);
	if (result == MYSQL_NO_DATA)
		return { Invalid_Job_Id };

	mysql_stmt_reset(mStmt_Get_Job_By_Name);

	if (result != 0)
	{
		return { Invalid_Job_Id };
	}

	namebuf[namelen] = '\0';
	statusbuf[statuslen] = '\0';
	outputbuf[outputlen] = '\0';

	jobrec.name = namebuf;

	std::string status = statusbuf;
	if (status == "wait")
		jobrec.status = NJob_Status::Wait;
	else if (status == "work")
		jobrec.status = NJob_Status::Work;
	else if (status == "done")
		jobrec.status = NJob_Status::Done;

	std::string output = outputbuf;
	if (output == "none")
		jobrec.output = NJob_Output::None;
	else if (output == "accept")
		jobrec.output = NJob_Output::Accept;
	else if (output == "reject")
		jobrec.output = NJob_Output::Reject;

	mysql_free_result(metaData);

	return jobrec;
}

bool CDatabase_Handler::Set_Job_Status(TJob_Id job_id, NJob_Status status)
{
	MYSQL_BIND param_bind[2];

	memset(param_bind, 0, sizeof(param_bind));
	std::string statusStr;
	switch (status)
	{
		case NJob_Status::Wait: statusStr = "wait"; break;
		case NJob_Status::Work: statusStr = "work"; break;
		case NJob_Status::Done: statusStr = "done"; break;
	}

	unsigned long parLen = static_cast<unsigned long>(statusStr.size());
	param_bind[0].buffer_type = MYSQL_TYPE_STRING;
	param_bind[0].buffer = const_cast<char*>(statusStr.c_str());
	param_bind[0].is_null = 0;
	param_bind[0].buffer_length = parLen;
	param_bind[0].length = &parLen;

	param_bind[1].buffer_type = MYSQL_TYPE_LONG;
	param_bind[1].buffer = reinterpret_cast<char*>(&job_id);
	param_bind[1].is_null = 0;
	param_bind[1].length = 0;

	auto result = mysql_stmt_bind_param(mStmt_Set_Job_Status, param_bind);
	if (result != 0)
		return false;

	result = mysql_stmt_execute(mStmt_Set_Job_Status);
	if (result != 0)
		return false;

	mysql_stmt_reset(mStmt_Set_Job_Status);

	return false;
}

bool CDatabase_Handler::Set_Job_Output(TJob_Id job_id, NJob_Output status)
{
	MYSQL_BIND param_bind[2];

	memset(param_bind, 0, sizeof(param_bind));
	std::string statusStr;
	switch (status)
	{
		case NJob_Output::None: statusStr = "none"; break;
		case NJob_Output::Accept: statusStr = "accept"; break;
		case NJob_Output::Reject: statusStr = "reject"; break;
	}

	unsigned long parLen = static_cast<unsigned long>(statusStr.size());
	param_bind[0].buffer_type = MYSQL_TYPE_STRING;
	param_bind[0].buffer = const_cast<char*>(statusStr.c_str());
	param_bind[0].is_null = 0;
	param_bind[0].buffer_length = parLen;
	param_bind[0].length = &parLen;

	param_bind[1].buffer_type = MYSQL_TYPE_LONG;
	param_bind[1].buffer = reinterpret_cast<char*>(&job_id);
	param_bind[1].is_null = 0;
	param_bind[1].length = 0;

	auto result = mysql_stmt_bind_param(mStmt_Set_Job_Output, param_bind);
	if (result != 0)
		return false;

	result = mysql_stmt_execute(mStmt_Set_Job_Output);
	if (result != 0)
		return false;

	mysql_stmt_reset(mStmt_Set_Job_Output);

	return false;
}

bool CDatabase_Handler::Add_Job_Report(TJob_Id job_id, ziran::NJob_Report_Type type, const std::string& value, std::optional<std::string> identifier)
{
	MYSQL_BIND param_bind[4];

	memset(param_bind, 0, sizeof(param_bind));
	std::string typeStr;
	switch (type)
	{
		case ziran::NJob_Report_Type::Error: typeStr = "error"; break;
		case ziran::NJob_Report_Type::Warning: typeStr = "warning"; break;
		case ziran::NJob_Report_Type::Info: typeStr = "info"; break;
	}

	param_bind[0].buffer_type = MYSQL_TYPE_LONG;
	param_bind[0].buffer = reinterpret_cast<char*>(&job_id);
	param_bind[0].is_null = 0;
	param_bind[0].length = 0;

	unsigned long parLen = static_cast<unsigned long>(typeStr.size());
	param_bind[1].buffer_type = MYSQL_TYPE_STRING;
	param_bind[1].buffer = const_cast<char*>(typeStr.c_str());
	param_bind[1].is_null = 0;
	param_bind[1].buffer_length = parLen;
	param_bind[1].length = &parLen;

	unsigned long parLen2 = static_cast<unsigned long>(identifier.has_value() ? identifier.value().size() : 0);
	my_bool isnull = 1;
	param_bind[2].buffer_type = MYSQL_TYPE_STRING;
	param_bind[2].buffer = identifier.has_value() ? const_cast<char*>(identifier.value().c_str()) : nullptr;
	param_bind[2].is_null = identifier.has_value() ? nullptr : &isnull;
	param_bind[2].buffer_length = parLen2;
	param_bind[2].length = &parLen2;

	unsigned long parLen3 = static_cast<unsigned long>(value.size());
	param_bind[3].buffer_type = MYSQL_TYPE_STRING;
	param_bind[3].buffer = const_cast<char*>(value.c_str());
	param_bind[3].is_null = 0;
	param_bind[3].buffer_length = parLen3;
	param_bind[3].length = &parLen3;

	auto result = mysql_stmt_bind_param(mStmt_Add_Job_Report, param_bind);
	if (result != 0)
		return false;

	result = mysql_stmt_execute(mStmt_Add_Job_Report);
	if (result != 0)
		return false;

	mysql_stmt_reset(mStmt_Add_Job_Report);

	return false;
}

const std::vector<TPipeline_Item_Record>& CDatabase_Handler::Get_Pipeline_Items(TJob_Id job_id) const
{
	// "at" used intentionally to throw exception when no such job is present
	return mDatabase_Pipeline_Items.at(job_id);
}

const GUID& CDatabase_Handler::Get_Plugin_GUID_By_Id(int id) const
{
	// "at" used intentionally to throw exception when no such plugin is present
	return mDatabase_Plugins.at(id);
}

std::map<std::string, std::string> CDatabase_Handler::Parse_Parameters(const std::string& str, std::vector<std::string>& log)
{
	if (str.empty())
		return {};

	// very, very primitive parser of key-value (string-string) pairs; this is prepared for future revisions, in case somebody decides to implement full JSON parameters

	std::map<std::string, std::string> m;

	std::string key;

	std::ostringstream pbuf;
	int bracketState = 0;
	bool keystate = false; // is key parsed?
	bool qstate = false; // quotes
	bool parstate = false; // is parameter parsed?

	for (auto& c : str)
	{
		if (qstate && c != '"')
			pbuf << c;
		else if (qstate && c == '"')
		{
			qstate = false;
			if (keystate)
			{
				m[key] = pbuf.str();
				parstate = true;
			}
			else
			{
				key = pbuf.str();
				keystate = true;
			}

			pbuf.str("");
			pbuf.clear();
		}
		else if (!qstate && !parstate && c == '"')
			qstate = true;
		else
		{
			// ignore whitespaces outside quotes
			if (c == ' ')
				continue;
			else if (c == ':' && keystate && !parstate)
				continue;
			else if (c == ',' && parstate)
			{
				parstate = false;
				keystate = false;
				continue;
			}
			else if (c == '}')
			{
				bracketState--;
				if (bracketState == 0)
					break;
			}
			else if (c == '{')
			{
				bracketState++;
			}
			else
			{
				log.push_back("Invalid character: '"s + c + "'");
			}
		}
	}

	return m;
}
