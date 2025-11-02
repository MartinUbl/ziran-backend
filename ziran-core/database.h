#pragma once

#include <string>
#include <vector>
#include <optional>
#include <mysql/mysql.h>
#include <map>

#include "../ziran-shared/guid.h"
#include "../ziran-shared/plugin.h"

// job ID type
using TJob_Id = int;

// invalid job - often used as an indicator
constexpr TJob_Id Invalid_Job_Id = static_cast<TJob_Id>(-1);

// available job status
enum class NJob_Status {
	Wait,		// db value 'wait'
	Work,		// db value 'work'
	Done,		// db value 'done'
};

// available job output
enum class NJob_Output {
	None,		// db value 'none'
	Accept,		// db value 'accept'
	Reject,		// db value 'reject'
};

// record of single job; matches the 'job' table layout
struct TJob_Record {
	TJob_Id id;
	std::string name;
	NJob_Status status;
	NJob_Output output;
	int pipeline_id;
};

// job report record structure; matches the 'job_report' table layout
struct TJob_Report_Record {
	int id;
	TJob_Id job_id;
	ziran::NJob_Report_Type log_type;
	std::string log_identifier;
	std::string value;
};

// pipeline item record structure; matches the 'pipeline_item' table layout, except parameters field, which is parsed
struct TPipeline_Item_Record {
	int id;
	int pipeline_id;
	int plugin_id;
	int order;
	std::map<std::string, std::string> parameters;
};

class CDefault_Environment;

/*
 * Database handler to access DB backend
 */
class CDatabase_Handler {
	private:
		// MySQL connection handle
		MYSQL* mConnection = nullptr;

		// stored statements

		MYSQL_STMT* mStmt_Get_Job_By_Name = nullptr;
		MYSQL_STMT* mStmt_Set_Job_Status = nullptr;
		MYSQL_STMT* mStmt_Set_Job_Output = nullptr;
		MYSQL_STMT* mStmt_Add_Job_Report = nullptr;
		MYSQL_STMT* mStmt_Set_Job_Processed_On = nullptr;

		// loaded plugin map (database plugins)
		std::map<int, GUID> mDatabase_Plugins;

		// loaded pipelines map
		std::map<int, std::string> mDatabase_Pipelines;

		// loaded pipeline items map
		std::map<int, std::vector<TPipeline_Item_Record>> mDatabase_Pipeline_Items;

	protected:
		// parses the pseudo-JSON string of parameters to a map
		static std::map<std::string, std::string> Parse_Parameters(const std::string& str);

	public:
		CDatabase_Handler();
		virtual ~CDatabase_Handler();

		// connects to a MySQL database using given credentials and specifiers
		bool Connect(const std::string& host, uint16_t port, const std::string& username, const std::string& password, const std::string& dbname);

		// intializes prepared statements
		bool Init_Statements();
		// loads plugin records from database
		bool Load_DB_Plugins();
		// loads global configuration records from database
		bool Load_DB_Global_Config(CDefault_Environment& env);
		// loads global configuration records from database
		bool Load_DB_Inputs(CDefault_Environment& env);
		// loads pipeline records from database
		bool Load_DB_Pipelines();
		// loads pipeline item records from database (ought to be called after Load_DB_Pipelines)
		bool Load_DB_Pipeline_Items();

		// closes database connection
		bool Close();

		// accessors

		// returns valid job id or Invalid_Job_Id if no such job found
		TJob_Record Get_Job_By_Name(const std::string& job_name);

		// sets job status
		bool Set_Job_Status(TJob_Id job_id, NJob_Status status);

		// set job processed on timestamp
		bool Set_Job_Processed_Timestamp(TJob_Id job_id, time_t timestamp);

		// sets job output status
		bool Set_Job_Output(TJob_Id job_id, NJob_Output status);

		// adds one report line to job report table
		bool Add_Job_Report(TJob_Id job_id, ziran::NJob_Report_Type type, const std::string& value, std::optional<std::string> identifier = std::nullopt, std::optional<std::string> extendedHTMLValue = std::nullopt);

		// retrieves all pipeline items
		const std::vector<TPipeline_Item_Record>& Get_Pipeline_Items(TJob_Id job_id) const;

		// retrieves plugin GUID by its id
		const GUID& Get_Plugin_GUID_By_Id(int id) const;
};
