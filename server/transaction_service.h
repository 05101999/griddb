﻿/*
	Copyright (c) 2017 TOSHIBA Digital Solutions Corporation

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*!
	@file
	@brief Definition of TransactionService
*/
#ifndef TRANSACTION_SERVICE_H_
#define TRANSACTION_SERVICE_H_

#include "util/trace.h"
#include "data_type.h"

#include "cluster_event_type.h"
#include "event_engine.h"
#include "transaction_manager.h"
#include "checkpoint_service.h"
#include "data_store.h"
#include "sync_service.h"
#include "result_set.h"
#include "transaction_statement_message.h"




#include "sql_job_manager.h"

struct TableSchemaInfo;
class DDLResultHandler;

#define TXN_PRIVATE private
#define TXN_PROTECTED protected

typedef StatementId ExecId;

#define TXN_THROW_DECODE_ERROR(errorCode, message) \
	GS_THROW_CUSTOM_ERROR(EncodeDecodeException, errorCode, message)

#define TXN_THROW_ENCODE_ERROR(errorCode, message) \
	GS_THROW_CUSTOM_ERROR(EncodeDecodeException, errorCode, message)

#define TXN_RETHROW_DECODE_ERROR(cause, message) \
	GS_RETHROW_CUSTOM_ERROR(                     \
		EncodeDecodeException, GS_ERROR_DEFAULT, cause, message)

#define TXN_RETHROW_ENCODE_ERROR(cause, message) \
	GS_RETHROW_CUSTOM_ERROR(                     \
		EncodeDecodeException, GS_ERROR_DEFAULT, cause, message)

const bool TXN_DETAIL_EXCEPTION_HIDDEN = true;

class SystemService;
class ClusterService;
class TriggerService;
class PartitionTable;
class DataStore;
class LogManager;
class ResultSet;
class ResultSetOption;
class MetaContainer;

class PutUserHandler;
class DropUserHandler;
class GetUsersHandler;
class PutDatabaseHandler;
class DropDatabaseHandler;
class GetDatabasesHandler;
class PutPrivilegeHandler;
class DropPrivilegeHandler;
struct SQLParsedInfo;

UTIL_TRACER_DECLARE(TRANSACTION_SERVICE);
UTIL_TRACER_DECLARE(REPLICATION);
UTIL_TRACER_DECLARE(SESSION_TIMEOUT);
UTIL_TRACER_DECLARE(TRANSACTION_TIMEOUT);
UTIL_TRACER_DECLARE(REPLICATION_TIMEOUT);
UTIL_TRACER_DECLARE(AUTHENTICATION_TIMEOUT);

enum RoleType {
	ALL,
	READ
};

/*!
	@brief Exception class for denying the statement execution
*/
class DenyException : public util::Exception {
public:
	explicit DenyException(UTIL_EXCEPTION_CONSTRUCTOR_ARGS_DECL) throw() :
			Exception(UTIL_EXCEPTION_CONSTRUCTOR_ARGS_SET) {}
	virtual ~DenyException() throw() {}
};

/*!
	@brief Exception class to notify encoding/decoding failure
*/
class EncodeDecodeException : public util::Exception {
public:
	explicit EncodeDecodeException(
			UTIL_EXCEPTION_CONSTRUCTOR_ARGS_DECL) throw() :
			Exception(UTIL_EXCEPTION_CONSTRUCTOR_ARGS_SET) {}
	virtual ~EncodeDecodeException() throw() {}
};

enum BackgroundEventType {
	TXN_BACKGROUND,
	SYNC_EXEC,
	CP_CHUNKCOPY
};

class BaseStatementHandler  : public EventHandler {
	virtual void initialize(const ResourceSet &resourceSet) {
		UNUSED_VARIABLE(resourceSet);
	}
};

/*!
	@brief Handles the statement(event) requested from a client or another node
*/
class StatementHandler : public EventHandler, public ResourceSetReceiver {
	friend struct ScenarioConfig;
	friend class TransactionHandlerTest;

public:
	typedef StatementMessage Message;

	typedef Message::OptionType OptionType;
	typedef Message::Utils MessageUtils;
	typedef Message::OptionSet OptionSet;
	typedef Message::FixedRequest FixedRequest;
	typedef Message::Request Request;
	typedef Message::Options Options;

	typedef Message::RequestType RequestType;
	typedef Message::UserType UserType;
	typedef Message::CreateDropIndexMode CreateDropIndexMode;
	typedef Message::CaseSensitivity CaseSensitivity;
	typedef Message::QueryContainerKey QueryContainerKey;

	typedef Message::UUIDObject UUIDObject;
	typedef Message::ExtensionParams ExtensionParams;
	typedef Message::ExtensionColumnTypes ExtensionColumnTypes;
	typedef Message::IntervalBaseValue IntervalBaseValue;
	typedef Message::PragmaList PragmaList;

	typedef Message::CompositeIndexInfos CompositeIndexInfos;


	StatementHandler();

	virtual ~StatementHandler();
	void initialize(const ResourceSet &resourceSet);
	void setNewSQL();

	static const size_t USER_NAME_SIZE_MAX = 64;  
	static const size_t PASSWORD_SIZE_MAX = 64;  
	static const size_t DATABASE_NAME_SIZE_MAX = 64;  

	static const size_t USER_NUM_MAX = 128;		 
	static const size_t DATABASE_NUM_MAX = 128;  

	enum QueryResponseType {
		ROW_SET					= 0,
		AGGREGATION				= 1,
		QUERY_ANALYSIS			= 2,
		PARTIAL_FETCH_STATE		= 3,	
		PARTIAL_EXECUTION_STATE = 4,	
		DIST_TARGET		= 32,	
		DIST_LIMIT		= 33,	
		DIST_AGGREGATION = 34,	
		DIST_ORDER		= 35,	
	};

	typedef int32_t ProtocolVersion;

	static const ProtocolVersion PROTOCOL_VERSION_UNDEFINED;

	static const ProtocolVersion TXN_V1_0_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V1_1_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V1_5_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V2_0_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V2_1_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V2_5_X_CLIENT_VERSION;

	static const ProtocolVersion TXN_V2_7_X_CLIENT_VERSION;

	static const ProtocolVersion TXN_V2_8_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V2_9_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V3_0_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V3_0_X_CE_CLIENT_VERSION;
	static const ProtocolVersion TXN_V3_1_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V3_2_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V3_5_X_CLIENT_VERSION;
	static const ProtocolVersion TXN_V4_0_0_CLIENT_VERSION;
	static const ProtocolVersion TXN_V4_0_1_CLIENT_VERSION;
	static const ProtocolVersion TXN_CLIENT_VERSION;


	typedef uint8_t StatementExecStatus;  
	static const StatementExecStatus TXN_STATEMENT_SUCCESS;  
	static const StatementExecStatus
		TXN_STATEMENT_ERROR;  
	static const StatementExecStatus
		TXN_STATEMENT_NODE_ERROR;  
	static const StatementExecStatus
		TXN_STATEMENT_DENY;  
	static const StatementExecStatus
		TXN_STATEMENT_SUCCESS_BUT_REPL_TIMEOUT;  

	typedef util::XArray<uint8_t> RowKeyData;  
	typedef util::XArray<uint8_t> RowData;	 
	typedef util::Map<int8_t, util::XArray<uint8_t> *> PartialQueryOption;	 

	/*!
		@brief Represents fetch setting
	*/
	struct FetchOption {
		FetchOption() : limit_(0), size_(0) {}

		ResultSize limit_;
		ResultSize size_;
	};

	/*!
		@brief Represents geometry query
	*/
	struct GeometryQuery {
		explicit GeometryQuery(util::StackAllocator &alloc)
			: columnId_(UNDEF_COLUMNID),
			  intersection_(alloc),
			  disjoint_(alloc),
			  operator_(GEOMETRY_INTERSECT) {}

		ColumnId columnId_;
		util::XArray<uint8_t> intersection_;
		util::XArray<uint8_t> disjoint_;
		GeometryOperator operator_;
	};

	/*!
		@brief Represents time-related condition
	*/
	struct TimeRelatedCondition {
		TimeRelatedCondition()
			: rowKey_(UNDEF_TIMESTAMP), operator_(TIME_PREV) {}

		Timestamp rowKey_;
		TimeOperator operator_;
	};

	/*!
		@brief Represents interpolation condition
	*/
	struct InterpolateCondition {
		InterpolateCondition()
			: rowKey_(UNDEF_TIMESTAMP), columnId_(UNDEF_COLUMNID) {}

		Timestamp rowKey_;
		ColumnId columnId_;
	};

	/*!
		@brief Represents aggregate condition
	*/
	struct AggregateQuery {
		AggregateQuery()
			: start_(UNDEF_TIMESTAMP),
			  end_(UNDEF_TIMESTAMP),
			  columnId_(UNDEF_COLUMNID),
			  aggregationType_(AGG_MIN) {}

		Timestamp start_;
		Timestamp end_;
		ColumnId columnId_;
		AggregationType aggregationType_;
	};

	/*!
		@brief Represents time range condition
	*/
	struct RangeQuery {
		RangeQuery()
			: start_(UNDEF_TIMESTAMP),
			  end_(UNDEF_TIMESTAMP),
			  order_(ORDER_ASCENDING) {}

		Timestamp start_;
		Timestamp end_;
		OutputOrder order_;
	};

	/*!
		@brief Represents sampling condition
	*/
	struct SamplingQuery {
		explicit SamplingQuery(util::StackAllocator &alloc)
			: start_(UNDEF_TIMESTAMP),
			  end_(UNDEF_TIMESTAMP),
			  timeUnit_(TIME_UNIT_YEAR),
			  interpolatedColumnIdList_(alloc),
			  mode_(INTERP_MODE_LINEAR_OR_PREVIOUS) {}

		Sampling toSamplingOption() const;

		Timestamp start_;
		Timestamp end_;
		uint32_t interval_;
		TimeUnit timeUnit_;
		util::XArray<uint32_t> interpolatedColumnIdList_;
		InterpolationMode mode_;
	};


	/*!
		@brief Represents the information about a user
	*/
	struct UserInfo {
		explicit UserInfo(util::StackAllocator &alloc)
			: userName_(alloc),
			  property_(0),
			  withDigest_(false),
			  digest_(alloc) {}

		util::String userName_;  
		int8_t property_;  
		bool withDigest_;	  
		util::String digest_;  

		std::string dump() {
			util::NormalOStringStream strstrm;
			strstrm << "\tuserName=" << userName_ << std::endl;
			strstrm << "\tproperty=" << static_cast<int>(property_)
					<< std::endl;
			if (withDigest_) {
				strstrm << "\twithDigest=true" << std::endl;
			}
			else {
				strstrm << "\twithDigest=false" << std::endl;
			}
			strstrm << "\tdigest=" << digest_ << std::endl;
			return strstrm.str();
		}
	};

	/*!
		@brief Represents the privilege information about a user
	*/
	struct PrivilegeInfo {
		explicit PrivilegeInfo(util::StackAllocator &alloc)
			: userName_(alloc), privilege_(alloc) {}

		util::String userName_;   
		util::String privilege_;  

		std::string dump() {
			util::NormalOStringStream strstrm;
			strstrm << "\tuserName=" << userName_ << std::endl;
			strstrm << "\tprivilege=" << privilege_ << std::endl;
			return strstrm.str();
		}
	};

	/*!
		@brief Represents the information about a database
	*/
	struct DatabaseInfo {
		explicit DatabaseInfo(util::StackAllocator &alloc)
			: dbName_(alloc), property_(0), privilegeInfoList_(alloc) {}

		util::String dbName_;  
		int8_t property_;	  
		util::XArray<PrivilegeInfo *> privilegeInfoList_;  

		std::string dump() {
			util::NormalOStringStream strstrm;
			strstrm << "\tdbName=" << dbName_ << std::endl;
			strstrm << "\tproperty=" << static_cast<int>(property_)
					<< std::endl;
			strstrm << "\tprivilegeNum=" << privilegeInfoList_.size()
					<< std::endl;
			for (size_t i = 0; i < privilegeInfoList_.size(); i++) {
				strstrm << privilegeInfoList_[i]->dump();
			}
			return strstrm.str();
		}
	};

	struct ConnectionOption;

	/*!
		@brief Represents response to a client
	*/
	struct Response {
		explicit Response(util::StackAllocator &alloc)
			: binaryData_(alloc),
			  containerNum_(0),
			  containerNameList_(alloc),
			  existFlag_(false),
			  schemaVersionId_(UNDEF_SCHEMAVERSIONID),
			  containerId_(UNDEF_CONTAINERID),
			  currentStatus_(-1),
			  currentAffinity_(UNDEF_NODE_AFFINITY_NUMBER),
			  indexInfo_(alloc),
			  existIndex_(0),
			  binaryData2_(alloc),
			  rs_(NULL),
			  last_(0),
			  userInfoList_(alloc),
			  databaseInfoList_(alloc),
			  containerAttribute_(CONTAINER_ATTR_SINGLE),
			  putRowOption_(0),
			  schemaMessage_(NULL)
			  , compositeIndexInfos_(NULL)
			  , connectionOption_(NULL)
		{
		}


		util::XArray<uint8_t> binaryData_;

		uint64_t containerNum_;
		util::XArray<FullContainerKey> containerNameList_;


		bool existFlag_;
		SchemaVersionId schemaVersionId_;
		ContainerId containerId_;

		LargeContainerStatusType currentStatus_;
		NodeAffinityNumber currentAffinity_;
		IndexInfo indexInfo_;
		uint8_t existIndex_;

		util::XArray<uint8_t> binaryData2_;


		ResultSet *rs_;




		RowId last_;




		util::XArray<UserInfo *> userInfoList_;

		util::XArray<DatabaseInfo *> databaseInfoList_;

		ContainerAttribute containerAttribute_;

		uint8_t putRowOption_;
		TableSchemaInfo *schemaMessage_;

		CompositeIndexInfos *compositeIndexInfos_;

		ConnectionOption *connectionOption_;
	};

	/*!
		@brief Represents the information about ReplicationAck
	*/
	struct ReplicationAck {
		explicit ReplicationAck(PartitionId pId) : pId_(pId) {}

		const PartitionId pId_;
		ClusterVersionId clusterVer_;
		ReplicationId replId_;

		int32_t replMode_;
		int32_t replStmtType_;
		StatementId replStmtId_;
		ClientId clientId_;
		int32_t taskStatus_;
	};

	void setSuccessReply(
			util::StackAllocator &alloc, Event &ev, StatementId stmtId,
			StatementExecStatus status, const Response &response,
			const Request &request);
	static void setErrorReply(
			Event &ev, StatementId stmtId,
			StatementExecStatus status, const std::exception &exception,
			const NodeDescriptor &nd);

	static void setReplyOption(OptionSet &optionSet, const Request &request);
	static void setReplyOption(
			OptionSet &optionSet, const ReplicationContext &replContext);

	static void setReplyOptionForContinue(
			OptionSet &optionSet, const Request &request);
	static void setReplyOptionForContinue(
			OptionSet &optionSet, const ReplicationContext &replContext);

	static void setSQLResonseInfo(
			ReplicationContext &replContext, const Request &request,
			const Response &response);

	static EventType resolveReplyEventType(
			EventType stmtType, const OptionSet &optionSet);

	typedef uint32_t
		ClusterRole;  
	static const ClusterRole CROLE_UNKNOWN;	
	static const ClusterRole CROLE_SUBMASTER;  
	static const ClusterRole CROLE_MASTER;	 
	static const ClusterRole CROLE_FOLLOWER;   
	static const ClusterRole CROLE_ANY;		   
	static const char8_t *const clusterRoleStr[8];

	typedef uint32_t PartitionRoleType;			   
	static const PartitionRoleType PROLE_UNKNOWN;  
	static const PartitionRoleType
		PROLE_NONE;  
	static const PartitionRoleType
		PROLE_OWNER;  
	static const PartitionRoleType
		PROLE_BACKUP;  
	static const PartitionRoleType
		PROLE_CATCHUP;  
	static const PartitionRoleType PROLE_ANY;  
	static const char8_t *const partitionRoleTypeStr[16];

	typedef uint32_t PartitionStatus;  
	static const PartitionStatus PSTATE_UNKNOWN;  
	static const PartitionStatus
		PSTATE_ON;  
	static const PartitionStatus
		PSTATE_SYNC;  
	static const PartitionStatus
		PSTATE_OFF;  
	static const PartitionStatus
		PSTATE_STOP;  
	static const PartitionStatus PSTATE_ANY;  
	static const char8_t *const partitionStatusStr[16];

	static const bool IMMEDIATE_CONSISTENCY =
		true;  
	static const bool ANY_CONSISTENCY = false;  

	static const bool NO_REPLICATION =
		false;  

	typedef std::pair<const char8_t*, const char8_t*> ExceptionParameterEntry;
	typedef util::Vector<ExceptionParameterEntry> ExceptionParameterList;

	ClusterService *clusterService_;
	ClusterManager *clusterManager_;
	ChunkManager *chunkManager_;
	DataStore *dataStore_;
	LogManager *logManager_;
	PartitionTable *partitionTable_;
	TransactionService *transactionService_;
	TransactionManager *transactionManager_;
	TriggerService *triggerService_;
	SystemService *systemService_;
	RecoveryManager *recoveryManager_;
	SQLService *sqlService_;
	bool isNewSQL_;
	const ResourceSet *resourceSet_;

	/*!
		@brief Represents the information about a connection
	*/
	struct ConnectionOption {
	public:
		ConnectionOption()
			: clientVersion_(PROTOCOL_VERSION_UNDEFINED),
			  txnTimeoutInterval_(TXN_DEFAULT_TRANSACTION_TIMEOUT_INTERVAL),
			  isAuthenticated_(false),
			  isImmediateConsistency_(false),
			  handlingPartitionId_(UNDEF_PARTITIONID),
			  connected_(true),
			  dbId_(0),
			  isAdminAndPublicDB_(true),
			  userType_(Message::USER_NORMAL),
			  authenticationTime_(0),
			  requestType_(Message::REQUEST_NOSQL)
			  ,
			  authMode_(0)
			  ,
			  storeMemoryAgingSwapRate_(TXN_UNSET_STORE_MEMORY_AGING_SWAP_RATE),
			  timeZone_(util::TimeZone())
			  ,
				updatedEnvBits_(0)
			  ,
				retryCount_(0)
			  ,
				clientId_()
			  ,
			  keepaliveTime_(0)
			  , handlingClientId_()
		{
		}

		void clear() {
			clientVersion_ = PROTOCOL_VERSION_UNDEFINED;
			txnTimeoutInterval_ = TXN_DEFAULT_TRANSACTION_TIMEOUT_INTERVAL;
			isAuthenticated_ = false;
			isImmediateConsistency_ = false;
			handlingPartitionId_ = UNDEF_PARTITIONID;
			connected_ = false;
			updatedEnvBits_ = 0;
			retryCount_ = 0;
			dbId_ = 0;
			isAdminAndPublicDB_ = true;
			userType_ = Message::USER_NORMAL;
			authenticationTime_ = 0;
			requestType_ = Message::REQUEST_NOSQL;

			storeMemoryAgingSwapRate_ = TXN_UNSET_STORE_MEMORY_AGING_SWAP_RATE;
			timeZone_ = util::TimeZone();
			keepaliveTime_ = 0;
			currentSessionId_ = 0;
			initializeCoreInfo();
		}

		void getHandlingClientId(ClientId &clientId);
		void setHandlingClientId(ClientId &clientId);
		void getLoginInfo(SQLString &userName, SQLString &dbName, SQLString &applicationName);
		void getSessionIdList(ClientId &clientId, util::XArray<SessionId> &sessionIdList);
		void setConnectionEnv(uint32_t paramId, const char8_t *value);
		bool getConnectionEnv(uint32_t paramId, std::string &value, bool &hasData);
		void removeSessionId(ClientId &clientId);

		void initializeCoreInfo();

		void setFirstStep(ClientId &clientId, double storeMemoryAgingSwapRate, const util::TimeZone &timeZone);
		void setBeforeAuth(const char8_t *userName, const char8_t *dbName, const char8_t *applicationName,
			bool isImmediateConsistency, int32_t txnTimeoutInterval, UserType userType,
			RequestType requestType, bool isAdminAndPublicDB);
		void setAfterAuth(DatabaseId dbId, EventMonotonicTime authenticationTime, RoleType role);

		void checkPrivilegeForOperator();
		void checkForUpdate(bool forUpdate);
		void checkSelect(SQLParsedInfo &parsedInfo);

		template<typename Alloc>
		bool getApplicationName(
				util::BasicString<
						char8_t, std::char_traits<char8_t>, Alloc> &name) {
			util::BasicString<
					char8_t, std::char_traits<char8_t>,
					util::StdAllocator<char8_t, void> > localName(
					*name.get_allocator().base());
			const bool ret = getApplicationName(&localName);
			name = localName.c_str();
			return ret;
		}
		bool getApplicationName(
				util::BasicString<
						char8_t, std::char_traits<char8_t>,
						util::StdAllocator<char8_t, void> > *name);
		void getApplicationName(std::ostream &os);

		ProtocolVersion clientVersion_;
		int32_t txnTimeoutInterval_;
		bool isAuthenticated_;
		bool isImmediateConsistency_;

		PartitionId handlingPartitionId_;
		bool connected_;
		DatabaseId dbId_;
		bool isAdminAndPublicDB_;
		UserType userType_;  
		EventMonotonicTime authenticationTime_;
		RequestType requestType_;  

		std::string userName_;
		std::string dbName_;
		RoleType role_;
		const int8_t authMode_;
		double storeMemoryAgingSwapRate_;
		util::TimeZone timeZone_;

		uint32_t updatedEnvBits_;

		int32_t retryCount_;
		ClientId clientId_;
		EventMonotonicTime keepaliveTime_;
		SessionId currentSessionId_;

	private:
		util::Mutex mutex_;
		std::string applicationName_;
		ClientId handlingClientId_;
		std::set<SessionId> statementList_;
		std::map<uint32_t, std::string> envMap_;

	};

	struct LockConflictStatus {
		LockConflictStatus(
				int32_t txnTimeoutInterval,
				EventMonotonicTime initialConflictMillis,
				EventMonotonicTime emNow) :
				txnTimeoutInterval_(txnTimeoutInterval),
				initialConflictMillis_(initialConflictMillis),
				emNow_(emNow) {
		}

		int32_t txnTimeoutInterval_;
		EventMonotonicTime initialConflictMillis_;
		EventMonotonicTime emNow_;
	};

	struct ErrorMessage {
		ErrorMessage(
				std::exception &cause, const char8_t *description,
				const Event &ev, const Request &request) :
				elements_(
						cause, description, ev,
						request.fixed_, &request.optional_) {
		}

		ErrorMessage(
				std::exception &cause, const char8_t *description,
				const Event &ev, const FixedRequest &request) :
				elements_(cause, description, ev, request, NULL) {
		}

		ErrorMessage withDescription(const char8_t *description) const {
			ErrorMessage org = *this;
			org.elements_.description_ = description;
			return org;
		}

		void format(std::ostream &os) const;
		void formatParameters(std::ostream &os) const;

		struct Elements {
			Elements(
					std::exception &cause, const char8_t *description,
					const Event &ev, const FixedRequest &request,
					const OptionSet *options) :
					cause_(cause),
					description_(description),
					ev_(ev),
					pId_(ev_.getPartitionId()),
					stmtType_(ev_.getType()),
					clientId_(request.clientId_),
					stmtId_(request.cxtSrc_.stmtId_),
					containerId_(request.cxtSrc_.containerId_),
					options_(options) {
			}
			std::exception &cause_;
			const char8_t *description_;
			const Event &ev_;
			PartitionId pId_;
			EventType stmtType_;
			ClientId clientId_;
			StatementId stmtId_;
			ContainerId containerId_;
			const OptionSet *options_;
		} elements_;
	};


	void checkAuthentication(
			const NodeDescriptor &ND, EventMonotonicTime emNow);
	void checkConsistency(const NodeDescriptor &ND, bool requireImmediate);
	void checkExecutable(
			PartitionId pId, ClusterRole requiredClusterRole,
			PartitionRoleType requiredPartitionRole,
			PartitionStatus requiredPartitionStatus, PartitionTable *pt);
	void checkExecutable(ClusterRole requiredClusterRole);
	static void checkExecutable(ClusterRole requiredClusterRole, PartitionTable *pt);

	void checkTransactionTimeout(
			EventMonotonicTime now,
			EventMonotonicTime queuedTime, int32_t txnTimeoutIntervalSec,
			uint32_t queueingCount);
	void checkContainerExistence(BaseContainer *container);
	void checkContainerSchemaVersion(
			BaseContainer *container, SchemaVersionId schemaVersionId);

	void checkFetchOption(FetchOption fetchOption);
	void checkSizeLimit(ResultSize limit);
	static void checkLoggedInDatabase(
			DatabaseId loginDbId, const char8_t *loginDbName,
			DatabaseId specifiedDbId, const char8_t *specifiedDbName
			, bool isNewSQL
			);
	void checkQueryOption(
			const OptionSet &optionSet,
			const FetchOption &fetchOption, bool isPartial, bool isTQL);

	void createResultSetOption(
			const OptionSet &optionSet,
			const int32_t *fetchBytesSize, bool partial,
			const PartialQueryOption &partialOption,
			ResultSetOption &queryOption);
	static void checkLogVersion(uint16_t logVersion);


	static FixedRequest::Source getRequestSource(const Event &ev);

	static void decodeRequestCommonPart(
			EventByteInStream &in, Request &request,
			ConnectionOption &connOption);
	static void decodeRequestOptionPart(
			EventByteInStream &in, Request &request,
			ConnectionOption &connOption);

	static void decodeOptionPart(
			EventByteInStream &in, OptionSet &optionSet);

	static bool isNewSQL(const Request &request);
	static bool isSkipReply(const Request &request);

	static CaseSensitivity getCaseSensitivity(const Request &request);
	static CaseSensitivity getCaseSensitivity(const OptionSet &optionSet);

	static CreateDropIndexMode getCreateDropIndexMode(
			const OptionSet &optionSet);
	static bool isDdlTransaction(const OptionSet &optionSet);

	static void assignIndexExtension(
			IndexInfo &indexInfo, const OptionSet &optionSet);
	static const char8_t* getExtensionName(const OptionSet &optionSet);

	static void decodeIndexInfo(
		util::ByteStream<util::ArrayInStream> &in, IndexInfo &indexInfo);

	static void decodeTriggerInfo(
		util::ByteStream<util::ArrayInStream> &in, TriggerInfo &triggerInfo);
	static void decodeMultipleRowData(util::ByteStream<util::ArrayInStream> &in,
		uint64_t &numRow, RowData &rowData);
	static void decodeFetchOption(
		util::ByteStream<util::ArrayInStream> &in, FetchOption &fetchOption);
	static void decodePartialQueryOption(
		util::ByteStream<util::ArrayInStream> &in, util::StackAllocator &alloc,
		bool &isPartial, PartialQueryOption &partitalQueryOption);

	static void decodeGeometryRelatedQuery(
		util::ByteStream<util::ArrayInStream> &in, GeometryQuery &query);
	static void decodeGeometryWithExclusionQuery(
		util::ByteStream<util::ArrayInStream> &in, GeometryQuery &query);
	static void decodeTimeRelatedConditon(util::ByteStream<util::ArrayInStream> &in,
		TimeRelatedCondition &condition);
	static void decodeInterpolateConditon(util::ByteStream<util::ArrayInStream> &in,
		InterpolateCondition &condition);
	static void decodeAggregateQuery(
		util::ByteStream<util::ArrayInStream> &in, AggregateQuery &query);
	static void decodeRangeQuery(
		util::ByteStream<util::ArrayInStream> &in, RangeQuery &query);
	static void decodeSamplingQuery(
		util::ByteStream<util::ArrayInStream> &in, SamplingQuery &query);
	static void decodeContainerConditionData(util::ByteStream<util::ArrayInStream> &in,
		DataStore::ContainerCondition &containerCondition);
	static void decodeResultSetId(
		util::ByteStream<util::ArrayInStream> &in, ResultSetId &resultSetId);

	template <typename IntType>
	static void decodeIntData(
		util::ByteStream<util::ArrayInStream> &in, IntType &intData);
	template <typename LongType>
	static void decodeLongData(
		util::ByteStream<util::ArrayInStream> &in, LongType &longData);
	template <typename StringType>
	static void decodeStringData(
		util::ByteStream<util::ArrayInStream> &in, StringType &strData);
	static void decodeBooleanData(
		util::ByteStream<util::ArrayInStream> &in, bool &boolData);
	static void decodeBinaryData(util::ByteStream<util::ArrayInStream> &in,
		util::XArray<uint8_t> &binaryData, bool readAll);
	static void decodeVarSizeBinaryData(util::ByteStream<util::ArrayInStream> &in,
		util::XArray<uint8_t> &binaryData);
	template <typename EnumType>
	static void decodeEnumData(
		util::ByteStream<util::ArrayInStream> &in, EnumType &enumData);
	static void decodeUUID(util::ByteStream<util::ArrayInStream> &in, uint8_t *uuid,
		size_t uuidSize);

	static void decodeReplicationAck(
		util::ByteStream<util::ArrayInStream> &in, ReplicationAck &ack);
	static void decodeStoreMemoryAgingSwapRate(
		util::ByteStream<util::ArrayInStream> &in,
		double &storeMemoryAgingSwapRate);
	static void decodeUserInfo(
		util::ByteStream<util::ArrayInStream> &in, UserInfo &userInfo);
	static void decodeDatabaseInfo(util::ByteStream<util::ArrayInStream> &in,
		DatabaseInfo &dbInfo, util::StackAllocator &alloc);

	static EventByteOutStream encodeCommonPart(
		Event &ev, StatementId stmtId, StatementExecStatus status);

	static void encodeIndexInfo(
		EventByteOutStream &out, const IndexInfo &indexInfo);
	static void encodeIndexInfo(
		util::XArrayByteOutStream &out, const IndexInfo &indexInfo);

	template <typename IntType>
	static void encodeIntData(EventByteOutStream &out, IntType intData);
	template <typename LongType>
	static void encodeLongData(EventByteOutStream &out, LongType longData);
	template<typename CharT, typename Traits, typename Alloc>
	static void encodeStringData(
			EventByteOutStream &out,
			const std::basic_string<CharT, Traits, Alloc> &strData);
	static void encodeStringData(
			EventByteOutStream &out, const char *strData);

	static void encodeBooleanData(EventByteOutStream &out, bool boolData);
	static void encodeBinaryData(
		EventByteOutStream &out, const uint8_t *data, size_t size);
	template <typename EnumType>
	static void encodeEnumData(EventByteOutStream &out, EnumType enumData);
	static void encodeUUID(
		EventByteOutStream &out, const uint8_t *uuid, size_t uuidSize);
	static void encodeVarSizeBinaryData(
		EventByteOutStream &out, const uint8_t *data, size_t size);
	static void encodeContainerKey(EventByteOutStream &out,
		const FullContainerKey &containerKey);

	template <typename ByteOutStream>
	static void encodeException(ByteOutStream &out,
		const std::exception &exception, bool detailsHidden,
		const ExceptionParameterList *paramList = NULL);
	template <typename ByteOutStream>
	static void encodeException(ByteOutStream &out,
		const std::exception &exception, const NodeDescriptor &nd);

	static void encodeReplicationAckPart(EventByteOutStream &out,
		ClusterVersionId clusterVer, int32_t replMode, const ClientId &clientId,
		ReplicationId replId, EventType replStmtType, StatementId replStmtId,
		int32_t taskStatus);
	static int32_t encodeDistributedResult(
		EventByteOutStream &out, const ResultSet &rs, int64_t *encodedSize);
	static void encodeStoreMemoryAgingSwapRate(
		EventByteOutStream &out, double storeMemoryAgingSwapRate);
	static void encodeTimeZone(
		EventByteOutStream &out, const util::TimeZone &timeZone);



	static bool isUpdateStatement(EventType stmtType);

	void replySuccess(
			EventContext &ec, util::StackAllocator &alloc,
			const NodeDescriptor &ND, EventType stmtType,
			StatementExecStatus status, const Request &request,
			const Response &response, bool ackWait);
	void continueEvent(
			EventContext &ec, util::StackAllocator &alloc,
			const NodeDescriptor &ND, EventType stmtType, StatementId originalStmtId,
			const Request &request, const Response &response, bool ackWait);
	void continueEvent(
			EventContext &ec, util::StackAllocator &alloc,
			StatementExecStatus status, const ReplicationContext &replContext);
	void replySuccess(
			EventContext &ec, util::StackAllocator &alloc,
			StatementExecStatus status, const ReplicationContext &replContext);

	void replyError(
			EventContext &ec, util::StackAllocator &alloc,
			const NodeDescriptor &ND, EventType stmtType,
			StatementExecStatus status, const Request &request,
			const std::exception &e);

	bool executeReplication(
			const Request &request, EventContext &ec,
			util::StackAllocator &alloc, const NodeDescriptor &clientND,
			TransactionContext &txn, EventType replStmtType, StatementId replStmtId,
			int32_t replMode,
			const ClientId *closedResourceIds, size_t closedResourceIdCount,
			const util::XArray<uint8_t> **logRecordList, size_t logRecordCount,
			const Response &response) {
		return executeReplication(request, ec, alloc, clientND, txn, replStmtType,
				replStmtId, replMode, ReplicationContext::TASK_FINISHED,
				closedResourceIds, closedResourceIdCount,
				logRecordList, logRecordCount, replStmtId, 0, response);
	}
	bool executeReplication(
			const Request &request, EventContext &ec,
			util::StackAllocator &alloc, const NodeDescriptor &clientND,
			TransactionContext &txn, EventType replStmtType, StatementId replStmtId,
			int32_t replMode, ReplicationContext::TaskStatus taskStatus,
			const ClientId *closedResourceIds, size_t closedResourceIdCount,
			const util::XArray<uint8_t> **logRecordList, size_t logRecordCount,
			StatementId originalStmtId, int32_t delayTime,
			const Response &response);
	void replyReplicationAck(
			EventContext &ec, util::StackAllocator &alloc,
			const NodeDescriptor &ND, const ReplicationAck &ack,
			bool optionalFormat);

	void handleError(
			EventContext &ec, util::StackAllocator &alloc, Event &ev,
			const Request &request, std::exception &e);

	bool abortOnError(TransactionContext &txn, util::XArray<uint8_t> &log);

	static void checkLockConflictStatus(
			const LockConflictStatus &status, LockConflictException &e);
	static void retryLockConflictedRequest(
			EventContext &ec, Event &ev, const LockConflictStatus &status);
	static LockConflictStatus getLockConflictStatus(
			const Event &ev, EventMonotonicTime emNow, const Request &request);
	static LockConflictStatus getLockConflictStatus(
			const Event &ev, EventMonotonicTime emNow,
			const TransactionManager::ContextSource &cxtSrc,
			const OptionSet &optionSet);
	static void updateLockConflictStatus(
			EventContext &ec, Event &ev, const LockConflictStatus &status);

	template<OptionType T>
	static void updateRequestOption(
			util::StackAllocator &alloc, Event &ev,
			const typename Message::OptionCoder<T>::ValueType &value);

	bool checkPrivilege(
			EventType command,
			UserType userType, RequestType requestType, bool isSystemMode,
			int32_t featureVersion,
			ContainerType resourceType, ContainerAttribute resourceSubType,
			ContainerAttribute expectedResourceSubType = CONTAINER_ATTR_ANY);

	bool isMetaContainerVisible(
			const MetaContainer &metaContainer, int32_t visibility);

	static bool isSupportedContainerAttribute(ContainerAttribute attribute);


	static void checkDbAccessible(
			const char8_t *loginDbName, const char8_t *specifiedDbName
			, bool isNewSql
	);

	static const char8_t *clusterRoleToStr(ClusterRole role);
	static const char8_t *partitionRoleTypeToStr(PartitionRoleType role);
	static const char8_t *partitionStatusToStr(PartitionStatus status);

	static bool getApplicationNameByOptionsOrND(
			const OptionSet *optionSet, const NodeDescriptor *nd,
			util::String *nameStr, std::ostream *os);

protected:
	const KeyConstraint& getKeyConstraint(
			const OptionSet &optionSet, bool checkLength = true) const;
	const KeyConstraint& getKeyConstraint(
			ContainerAttribute containerAttribute, bool checkLength = true) const;

	KeyConstraint keyConstraint_[2][2][2];
};

template <typename IntType>
void StatementHandler::decodeIntData(
	util::ByteStream<util::ArrayInStream> &in, IntType &intData) {
	try {
		in >> intData;
	}
	catch (std::exception &e) {
		TXN_RETHROW_DECODE_ERROR(e, "");
	}
}

template <typename LongType>
void StatementHandler::decodeLongData(
	util::ByteStream<util::ArrayInStream> &in, LongType &longData) {
	try {
		in >> longData;
	}
	catch (std::exception &e) {
		TXN_RETHROW_DECODE_ERROR(e, "");
	}
}

template <typename StringType>
void StatementHandler::decodeStringData(
	util::ByteStream<util::ArrayInStream> &in, StringType &strData) {
	try {
		in >> strData;
	}
	catch (std::exception &e) {
		TXN_RETHROW_DECODE_ERROR(e, "");
	}
}

template <typename EnumType>
void StatementHandler::decodeEnumData(
	util::ByteStream<util::ArrayInStream> &in, EnumType &enumData) {
	try {
		int8_t tmp;
		in >> tmp;
		enumData = static_cast<EnumType>(tmp);
	}
	catch (std::exception &e) {
		TXN_RETHROW_DECODE_ERROR(e, "");
	}
}

template <typename IntType>
void StatementHandler::encodeIntData(EventByteOutStream &out, IntType intData) {
	try {
		out << intData;
	}
	catch (std::exception &e) {
		TXN_RETHROW_ENCODE_ERROR(e, "");
	}
}

template <typename LongType>
void StatementHandler::encodeLongData(
	EventByteOutStream &out, LongType longData) {
	try {
		out << longData;
	}
	catch (std::exception &e) {
		TXN_RETHROW_ENCODE_ERROR(e, "");
	}
}

template<typename CharT, typename Traits, typename Alloc>
void StatementHandler::encodeStringData(
		EventByteOutStream &out,
		const std::basic_string<CharT, Traits, Alloc> &strData) {
	try {
		const uint32_t size = static_cast<uint32_t>(strData.size());
		out << size;
		out << std::pair<const uint8_t *, size_t>(
			reinterpret_cast<const uint8_t *>(strData.c_str()), size);
	}
	catch (std::exception &e) {
		TXN_RETHROW_ENCODE_ERROR(e, "");
	}
}

template <typename EnumType>
void StatementHandler::encodeEnumData(
	EventByteOutStream &out, EnumType enumData) {
	try {
		const uint8_t tmp = static_cast<uint8_t>(enumData);
		out << tmp;
	}
	catch (std::exception &e) {
		TXN_RETHROW_ENCODE_ERROR(e, "");
	}
}

inline std::ostream& operator<<(
		std::ostream &os, const StatementHandler::ErrorMessage &errorMessage) {
	errorMessage.format(os);
	return os;
}

/*!
	@brief Handles CONNECT statement
*/
class ConnectHandler : public StatementHandler {
public:
	ConnectHandler(ProtocolVersion currentVersion,
		const ProtocolVersion *acceptableProtocolVersions);

	void operator()(EventContext &ec, Event &ev);

private:
	typedef uint32_t OldStatementId;  
	static const OldStatementId UNDEF_OLD_STATEMENTID = UINT32_MAX;

	const ProtocolVersion currentVersion_;
	const ProtocolVersion *acceptableProtocolVersions_;

	struct ConnectRequest {
		ConnectRequest(PartitionId pId, EventType stmtType)
			: pId_(pId),
			  stmtType_(stmtType),
			  oldStmtId_(UNDEF_OLD_STATEMENTID),
			  clientVersion_(PROTOCOL_VERSION_UNDEFINED) {}

		const PartitionId pId_;
		const EventType stmtType_;
		OldStatementId oldStmtId_;
		ProtocolVersion clientVersion_;
	};

	void checkClientVersion(ProtocolVersion clientVersion);

	EventByteOutStream encodeCommonPart(
		Event &ev, OldStatementId stmtId, StatementExecStatus status);

	void replySuccess(EventContext &ec, util::StackAllocator &alloc,
		const NodeDescriptor &ND, EventType stmtType,
		StatementExecStatus status, const ConnectRequest &request,
		const Response &response, bool ackWait);

	void replyError(EventContext &ec, util::StackAllocator &alloc,
		const NodeDescriptor &ND, EventType stmtType,
		StatementExecStatus status, const ConnectRequest &request,
		const std::exception &e);

	void handleError(EventContext &ec, util::StackAllocator &alloc, Event &ev,
		const ConnectRequest &request, std::exception &e);
};

/*!
	@brief Handles DISCONNECT statement
*/
class DisconnectHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles LOGOUT statement
*/
class LogoutHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles GET_PARTITION_ADDRESS statement
*/
class GetPartitionAddressHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

	static void encodeClusterInfo(
			util::XArrayByteOutStream &out, EventEngine &ee,
			PartitionTable &partitionTable, ContainerHashMode hashMode);
};

/*!
	@brief Handles GET_PARTITION_CONTAINER_NAMES statement
*/
class GetPartitionContainerNamesHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles GET_CONTAINER_PROPERTIES statement
*/
class GetContainerPropertiesHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

	template<typename C>
	void encodeContainerProps(
			TransactionContext &txn, EventByteOutStream &out, C *container,
			uint32_t propFlags, uint32_t propTypeCount, bool forMeta,
			EventMonotonicTime emNow, util::String &containerNameStr,
			const char8_t *dbName
			, int64_t currentTime
			, int32_t acceptableFeatureVersion
			);

	static void getPartitioningTableIndexInfo(
		TransactionContext &txn, EventMonotonicTime emNow, DataStore &dataStore,
		BaseContainer &largeContainer, const char8_t *containerName,
		const char *dbName, util::Vector<IndexInfo> &indexInfoList);

private:
	typedef util::XArray<util::XArray<uint8_t> *> ContainerNameList;

	enum ContainerProperty {
		CONTAINER_PROPERTY_ID,
		CONTAINER_PROPERTY_SCHEMA,
		CONTAINER_PROPERTY_INDEX,
		CONTAINER_PROPERTY_EVENT_NOTIFICATION,
		CONTAINER_PROPERTY_TRIGGER,
		CONTAINER_PROPERTY_ATTRIBUTES,
		CONTAINER_PROPERTY_INDEX_DETAIL,
		CONTAINER_PROPERTY_NULLS_STATISTICS,
		CONTAINER_PROPERTY_PARTITIONING_METADATA = 16
	};

	enum MetaDistributionType {
		META_DIST_NONE,
		META_DIST_FULL,
		META_DIST_NODE
	};

	enum PartitioningContainerOption {
		PARTITIONING_PROPERTY_AFFINITY_ = 0 
	};

	void encodeResultListHead(EventByteOutStream &out, uint32_t totalCount);
	void encodePropsHead(EventByteOutStream &out, uint32_t propTypeCount);
	void encodeId(
			EventByteOutStream &out, SchemaVersionId schemaVersionId,
			ContainerId containerId, const FullContainerKey &containerKey,
			ContainerId metaContainerId, MetaDistributionType metaDistType,
			int8_t metaNamingType);
	void encodeMetaId(
			EventByteOutStream &out, ContainerId metaContainerId,
			MetaDistributionType metaDistType, int8_t metaNamingType);
	void encodeSchema(EventByteOutStream &out, ContainerType containerType,
			const util::XArray<uint8_t> &serializedCollectionInfo);
	void encodeIndex(
			EventByteOutStream &out, const util::Vector<IndexInfo> &indexInfoList);
	void encodeEventNotification(EventByteOutStream &out,
			const util::XArray<char *> &urlList,
			const util::XArray<uint32_t> &urlLenList);
	void encodeTrigger(EventByteOutStream &out,
			const util::XArray<const uint8_t *> &triggerList);
	void encodeAttributes(
			EventByteOutStream &out, const ContainerAttribute containerAttribute);
	void encodeIndexDetail(
			EventByteOutStream &out, const util::Vector<IndexInfo> &indexInfoList);
	void encodePartitioningMetaData(
			EventByteOutStream &out,
			TransactionContext &txn, EventMonotonicTime emNow,
			BaseContainer &largeContainer,  ContainerAttribute attribute,
			const char *dbName, const char8_t *containerName
			, int64_t currentTime
			);
	void encodeNulls(
			EventByteOutStream &out,
			const util::XArray<uint8_t> &nullsList);

	static void checkIndexInfoVersion(
			const util::Vector<IndexInfo> indexInfoList,
			int32_t acceptableFeatureVersion);
	static void checkIndexInfoVersion(
			const IndexInfo &info, int32_t acceptableFeatureVersion);
};

/*!
	@brief Handles PUT_CONTAINER statement
*/
class PutContainerHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

private:
	void setContainerAttributeForCreate(OptionSet &optionSet);
};

class PutLargeContainerHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

private:
	void setContainerAttributeForCreate(OptionSet &optionSet);
};

class UpdateContainerStatusHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
	void decode(EventByteInStream &in, Request &request,
		ConnectionOption &conn,
		util::XArray<uint8_t> &containerNameBinary,
		ContainerCategory &category, NodeAffinityNumber &affinity,
		TablePartitioningVersionId &versionId,
		ContainerId &largeContainerId, LargeContainerStatusType &status, IndexInfo &indexInfo);
private:
};

class CreateLargeIndexHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
	bool checkCreateIndex(IndexInfo &indexInfo,
			ColumnType targetColumnType, ContainerType targetContainerType);

};

/*!
	@brief Handles DROP_CONTAINER statement
*/
class DropContainerHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles GET_CONTAINER statement
*/
class GetContainerHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles CREATE_DROP_INDEX statement
*/
class CreateDropIndexHandler : public StatementHandler {
public:

protected:
	bool getExecuteFlag(
		EventType eventType, CreateDropIndexMode mode,
		TransactionContext &txn, BaseContainer &container,
		const IndexInfo &info, bool isCaseSensitive);

	bool compareIndexName(util::StackAllocator &alloc,
		const util::String &specifiedName, const util::String &existingName,
		bool isCaseSensitive);
};

/*!
	@brief Handles CREATE_DROP_INDEX statement
*/
class CreateIndexHandler : public CreateDropIndexHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles CREATE_DROP_INDEX statement
*/
class DropIndexHandler : public CreateDropIndexHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles DROP_TRIGGER statement
*/
class CreateDropTriggerHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles FLUSH_LOG statement
*/
class FlushLogHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles WRITE_LOG_PERIODICALLY event
*/
class WriteLogPeriodicallyHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles CREATE_TRANSACTIN_CONTEXT statement
*/
class CreateTransactionContextHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles CLOSE_TRANSACTIN_CONTEXT statement
*/
class CloseTransactionContextHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles COMMIT_TRANSACTIN statement
*/
class CommitAbortTransactionHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles PUT_ROW statement
*/
class PutRowHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles PUT_ROW_SET statement
*/
class PutRowSetHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles REMOVE_ROW statement
*/
class RemoveRowHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles UPDATE_ROW_BY_ID statement
*/
class UpdateRowByIdHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles REMOVE_ROW_BY_ID statement
*/
class RemoveRowByIdHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles GET_ROW statement
*/
class GetRowHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles GET_ROW_SET statement
*/
class GetRowSetHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles QUERY_TQL statement
*/
class QueryTqlHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles APPEND_ROW statement
*/
class AppendRowHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles QUERY_GEOMETRY_RELATED statement
*/
class QueryGeometryRelatedHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles QUERY_GEOMETRY_WITH_EXCLUSION statement
*/
class QueryGeometryWithExclusionHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles GET_ROW_TIME_RELATED statement
*/
class GetRowTimeRelatedHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles ROW_INTERPOLATE statement
*/
class GetRowInterpolateHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles AGGREGATE statement
*/
class AggregateHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles QUERY_TIME_RANGE statement
*/
class QueryTimeRangeHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles QUERY_TIME_SAMPING statement
*/
class QueryTimeSamplingHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles FETCH_RESULT_SET statement
*/
class FetchResultSetHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};
/*!
	@brief Handles CLOSE_RESULT_SET statement
*/
class CloseResultSetHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles MULTI_CREATE_TRANSACTION_CONTEXT statement
*/
class MultiCreateTransactionContextHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

private:
	struct SessionCreationEntry {
		SessionCreationEntry() :
			containerAttribute_(CONTAINER_ATTR_ANY),
			containerName_(NULL),
			containerId_(UNDEF_CONTAINERID),
			sessionId_(UNDEF_SESSIONID) {}

		ContainerAttribute containerAttribute_;
		util::XArray<uint8_t> *containerName_;
		ContainerId containerId_;
		SessionId sessionId_;
	};

	bool decodeMultiTransactionCreationEntry(
		util::ByteStream<util::ArrayInStream> &in,
		util::XArray<SessionCreationEntry> &entryList);
};
/*!
	@brief Handles MULTI_CLOSE_TRANSACTION_CONTEXT statement
*/
class MultiCloseTransactionContextHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

private:
	struct SessionCloseEntry {
		SessionCloseEntry()
			: stmtId_(UNDEF_STATEMENTID),
			  containerId_(UNDEF_CONTAINERID),
			  sessionId_(UNDEF_SESSIONID) {}

		StatementId stmtId_;
		ContainerId containerId_;
		SessionId sessionId_;
	};

	void decodeMultiTransactionCloseEntry(
		util::ByteStream<util::ArrayInStream> &in,
		util::XArray<SessionCloseEntry> &entryList);
};

/*!
	@brief Handles multi-type statement
*/
class MultiStatementHandler : public StatementHandler {
public:
	struct MultiOpErrorMessage;

TXN_PROTECTED :
	enum ContainerResult {
		CONTAINER_RESULT_SUCCESS,
		CONTAINER_RESULT_ALREADY_EXECUTED,
		CONTAINER_RESULT_FAIL
	};

	struct Progress {
		util::XArray<ContainerResult> containerResult_;
		util::XArray<uint8_t> lastExceptionData_;
		util::XArray<const util::XArray<uint8_t> *> logRecordList_;
		LockConflictStatus lockConflictStatus_;
		bool lockConflicted_;
		StatementId mputStartStmtId_;
		uint64_t totalRowCount_;

		Progress(
				util::StackAllocator &alloc,
				const LockConflictStatus &lockConflictStatus) :
				containerResult_(alloc),
				lastExceptionData_(alloc),
				logRecordList_(alloc),
				lockConflictStatus_(lockConflictStatus),
				lockConflicted_(false),
				mputStartStmtId_(UNDEF_STATEMENTID),
				totalRowCount_(0) {
		}
	};

	void handleExecuteError(
			util::StackAllocator &alloc,
			const Event &ev, const Request &wholeRequest,
			EventType stmtType, PartitionId pId, const ClientId &clientId,
			const TransactionManager::ContextSource &src, Progress &progress,
			std::exception &e, const char8_t *executionName);

	void handleWholeError(
			EventContext &ec, util::StackAllocator &alloc,
			const Event &ev, const Request &request, std::exception &e);

	void decodeContainerOptionPart(
			EventByteInStream &in, const FixedRequest &fixedRequest,
			OptionSet &optionSet);
};

struct MultiStatementHandler::MultiOpErrorMessage {
	MultiOpErrorMessage(
			std::exception &cause, const char8_t *description,
			const Event &ev, const Request &wholeRequest,
			EventType stmtType, PartitionId pId, const ClientId &clientId,
			const TransactionManager::ContextSource &cxtSrc,
			const char8_t *containerName, const char8_t *executionName) :
			base_(cause, description, ev, wholeRequest),
			containerName_(containerName),
			executionName_(executionName) {
		base_.elements_.stmtType_ = stmtType;
		base_.elements_.pId_ = pId;
		base_.elements_.stmtId_ = UNDEF_STATEMENTID;
		base_.elements_.clientId_ = clientId;
		base_.elements_.containerId_ = cxtSrc.containerId_;
	}

	MultiOpErrorMessage withDescription(const char8_t *description) const {
		MultiOpErrorMessage org = *this;
		org.base_.elements_.description_ = description;
		return org;
	}

	void format(std::ostream &os) const;

	ErrorMessage base_;
	const char8_t *containerName_;
	const char8_t *executionName_;
};

inline std::ostream& operator<<(
		std::ostream &os,
		const MultiStatementHandler::MultiOpErrorMessage &errorMessage) {
	errorMessage.format(os);
	return os;
}

/*!
	@brief Handles MULTI_PUT statement
*/
class MultiPutHandler : public MultiStatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

private:
	struct RowSetRequest {
		StatementId stmtId_;
		ContainerId containerId_;
		SessionId sessionId_;
		TransactionManager::GetMode getMode_;
		TransactionManager::TransactionMode txnMode_;

		OptionSet option_;

		int32_t schemaIndex_;
		uint64_t rowCount_;
		util::XArray<uint8_t> rowSetData_;

		explicit RowSetRequest(util::StackAllocator &alloc)
			: stmtId_(UNDEF_STATEMENTID),
			  containerId_(UNDEF_CONTAINERID),
			  sessionId_(UNDEF_SESSIONID),
			  getMode_(TransactionManager::AUTO),
			  txnMode_(TransactionManager::AUTO_COMMIT),
			  option_(alloc),
			  schemaIndex_(-1),
			  rowCount_(0),
			  rowSetData_(alloc) {}
	};

	typedef std::pair<int32_t, ColumnSchemaId> CheckedSchemaId;
	typedef util::SortedList<CheckedSchemaId> CheckedSchemaIdSet;

	void execute(
			EventContext &ec, const Event &ev, const Request &request,
			const RowSetRequest &rowSetRequest, const MessageSchema &schema,
			CheckedSchemaIdSet &idSet, Progress &progress,
			PutRowOption putRowOption);

	void checkSchema(TransactionContext &txn, BaseContainer &container,
		const MessageSchema &schema, int32_t localSchemaId,
		CheckedSchemaIdSet &idSet);

	TransactionManager::ContextSource createContextSource(
			const Request &request, const RowSetRequest &rowSetRequest);

	void decodeMultiSchema(
			util::ByteStream<util::ArrayInStream> &in,
			util::XArray<const MessageSchema *> &schemaList);
	void decodeMultiRowSet(
			util::ByteStream<util::ArrayInStream> &in, const Request &request,
			const util::XArray<const MessageSchema *> &schemaList,
			util::XArray<const RowSetRequest *> &rowSetList);
};
/*!
	@brief Handles MULTI_GET statement
*/
class MultiGetHandler : public MultiStatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

private:
	typedef util::XArray<RowKeyData *> RowKeyDataList;
	typedef int32_t LocalSchemaId;
	typedef util::Map<ContainerId, LocalSchemaId> SchemaMap;
	typedef util::XArray<ColumnType> RowKeyColumnTypeList;

	enum RowKeyPredicateType { PREDICATE_TYPE_RANGE, PREDICATE_TYPE_DISTINCT };

	struct RowKeyPredicate {
		ColumnType keyType_;
		RowKeyPredicateType predicateType_;
		int8_t rangeFlags_[2];
		int32_t rowCount_;
		RowKeyDataList *keys_;
		RowKeyData *compositeKeys_;
		RowKeyColumnTypeList *compositeColumnTypes_;
	};

	struct SearchEntry {
		SearchEntry(ContainerId containerId, const FullContainerKey *containerKey,
			const RowKeyPredicate *predicate)
			: stmtId_(0),
			  containerId_(containerId),
			  sessionId_(TXN_EMPTY_CLIENTID.sessionId_),
			  getMode_(TransactionManager::AUTO),
			  txnMode_(TransactionManager::AUTO_COMMIT),
			  containerKey_(containerKey),
			  predicate_(predicate) {}

		StatementId stmtId_;
		ContainerId containerId_;
		SessionId sessionId_;
		TransactionManager::GetMode getMode_;
		TransactionManager::TransactionMode txnMode_;

		const FullContainerKey *containerKey_;
		const RowKeyPredicate *predicate_;
	};

	enum OptionType {


		OPTION_END = 0xFFFFFFFF
	};

	uint32_t execute(
			EventContext &ec, const Event &ev, const Request &request,
			const SearchEntry &entry, const SchemaMap &schemaMap,
			EventByteOutStream &replyOut, Progress &progress);

	void buildSchemaMap(
			PartitionId pId,
			const util::XArray<SearchEntry> &searchList, SchemaMap &schemaMap,
			EventByteOutStream &out);

	void checkContainerRowKey(
			BaseContainer *container, const RowKeyPredicate &predicate);

	TransactionManager::ContextSource createContextSource(
			const Request &request, const SearchEntry &entry);

	void decodeMultiSearchEntry(
			util::ByteStream<util::ArrayInStream> &in,
			PartitionId pId, DatabaseId loginDbId,
			const char8_t *loginDbName, const char8_t *specifiedDbName,
			UserType userType, RequestType requestType, bool isSystemMode,
			util::XArray<SearchEntry> &searchList);
	RowKeyPredicate decodePredicate(
			util::ByteStream<util::ArrayInStream> &in, util::StackAllocator &alloc);

	void encodeEntry(
			const FullContainerKey &containerKey, ContainerId containerId,
			const SchemaMap &schemaMap, ResultSet &rs, EventByteOutStream &out);

	void executeRange(
			EventContext &ec, const SchemaMap &schemaMap,
			TransactionContext &txn, BaseContainer *container, const RowKeyData *startKey,
			const RowKeyData *finishKey, EventByteOutStream &replyOut, Progress &progress);
	void executeGet(
			EventContext &ec, const SchemaMap &schemaMap,
			TransactionContext &txn, BaseContainer *container, const RowKeyData &rowKey,
			EventByteOutStream &replyOut, Progress &progress);
};
/*!
	@brief Handles MULTI_QUERY statement
*/
class MultiQueryHandler : public MultiStatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);

private:
	struct QueryRequest {
		explicit QueryRequest(util::StackAllocator &alloc)
			: stmtType_(UNDEF_EVENT_TYPE),
			  stmtId_(UNDEF_STATEMENTID),
			  containerId_(UNDEF_CONTAINERID),
			  sessionId_(UNDEF_SESSIONID),
			  schemaVersionId_(UNDEF_SCHEMAVERSIONID),
			  getMode_(TransactionManager::AUTO),
			  txnMode_(TransactionManager::AUTO_COMMIT),
			  optionSet_(alloc),
			  isPartial_(false),
			  partialQueryOption_(alloc) {
			query_.ptr_ = NULL;
		}

		EventType stmtType_;

		StatementId stmtId_;
		ContainerId containerId_;
		SessionId sessionId_;
		SchemaVersionId schemaVersionId_;
		TransactionManager::GetMode getMode_;
		TransactionManager::TransactionMode txnMode_;

		OptionSet optionSet_;

		FetchOption fetchOption_;
		bool isPartial_;
		PartialQueryOption partialQueryOption_;

		union {
			void *ptr_;
			util::String *tqlQuery_;
			GeometryQuery *geometryQuery_;
			RangeQuery *rangeQuery_;
			SamplingQuery *samplingQuery_;
		} query_;
	};

	enum PartialResult { PARTIAL_RESULT_SUCCESS, PARTIAL_RESULT_FAIL };

	typedef util::XArray<const QueryRequest *> QueryRequestList;

	void execute(
			EventContext &ec, const Event &ev, const Request &request,
			int32_t &fetchByteSize, const QueryRequest &queryRequest,
			EventByteOutStream &replyOut, Progress &progress);

	TransactionManager::ContextSource createContextSource(
			const Request &request, const QueryRequest &queryRequest);

	void decodeMultiQuery(
			util::ByteStream<util::ArrayInStream> &in, const Request &request,
			util::XArray<const QueryRequest *> &queryList);

	void encodeMultiSearchResultHead(
			EventByteOutStream &out, uint32_t queryCount);
	void encodeEmptySearchResult(
			util::StackAllocator &alloc, EventByteOutStream &out);
	void encodeSearchResult(util::StackAllocator &alloc,
			EventByteOutStream &out, const ResultSet &rs);

	const char8_t *getQueryTypeName(EventType queryStmtType);
};

/*!
	@brief Handles REPLICATION_LOG statement requested from another node
*/
class ReplicationLogHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
protected:
	virtual void decode(EventByteInStream &in, ReplicationAck &ack, Request &requestOption, 
		ConnectionOption &connOption) {
		UNUSED_VARIABLE(requestOption);
		UNUSED_VARIABLE(connOption);

		decodeReplicationAck(in, ack);
	}

	virtual bool isOptionalFormat() const {
		return false;
	}
};
/*!
	@brief Handles REPLICATION_ACK statement requested from another node
*/
class ReplicationAckHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
protected:
	virtual void decode(EventByteInStream &in, ReplicationAck &ack, Request &requestOption, 
		ConnectionOption &connOption) {
		decodeReplicationAck(in, ack);
	}
};

/*!
	@brief Handles REPLICATION_LOG2 statement requested from another node
*/
class ReplicationLog2Handler : public ReplicationLogHandler {
public:
protected:
	void decode(EventByteInStream &in, ReplicationAck &ack, Request &requestOption, 
		ConnectionOption &connOption) {
		decodeRequestCommonPart(in, requestOption, connOption);
		decodeReplicationAck(in, ack);
	}
	bool isOptionalFormat() const {
		return true;
	}
};

/*!
	@brief Handles REPLICATION_ACK2 statement requested from another node
*/
class ReplicationAck2Handler : public ReplicationAckHandler {
public:
protected:
	void decode(EventByteInStream &in, ReplicationAck &ack, Request &requestOption,
		ConnectionOption &connOption) {
		decodeRequestCommonPart(in, requestOption, connOption);
		decodeReplicationAck(in, ack);
	}
};


class DbUserHandler : public StatementHandler {
public:
	static void checkPasswordLength(const char8_t *password);

	struct DUColumnInfo {
		const char *name;
		ColumnType type;
	};

	struct DUColumnValue {
		ColumnType type;
		const char8_t *sval;
		int8_t bval;
	};

protected:
	struct DUGetInOut {
		enum DUGetType {
			RESULT_NUM,
			DBID,
			ROLE,
			REMOVE,
		};

		explicit DUGetInOut()
			: type(RESULT_NUM),
			  count(-1), s(NULL), dbId(UNDEF_DBID), role(READ),
			  logRecordList(NULL) {}

		void setForDbId(const char8_t *dbName, UserType userType) {
			type = DBID;
			s = dbName;
			this->userType = userType;
		}

		void setForRole() {
			type = ROLE;
		}
		void setForRemove(util::XArray<const util::XArray<uint8_t> *> *logRecordList) {
			type = REMOVE;
			this->logRecordList = logRecordList;
		}

		int8_t type;
		int64_t count;
		const char8_t *s;
		UserType userType;
		DatabaseId dbId;
		RoleType role;
		util::XArray<const util::XArray<uint8_t> *> *logRecordList;
	};

	struct DUQueryInOut {
		enum DUQueryType {
			RESULT_NUM,
			AGG,
			USER_DETAILS,
			DB_DETAILS,
			USER_INFO,
			DB_INFO,
			REMOVE,
		};

		enum DUQueryPhase {
			AUTH,
			GET,
			NORMAL,
			SYSTEM,
		};

		explicit DUQueryInOut()
			: type(RESULT_NUM), phase(NORMAL),
			  count(-1), s(NULL), 
			  dbInfo(NULL),
			  flag(false),
			  userInfoList(NULL),
			  dbNameSpecified(false), dbInfoList(NULL),
			  logRecordList(NULL) {}

		void setForAgg(const char8_t *userName = NULL) {
			type = AGG;
			if (userName) {
				this->phase = AUTH;
			}
			s = userName;
			
		}
		void setForUserDetails(const char8_t *digest) {
			type = USER_DETAILS;
			phase = SYSTEM;
			s = digest;
		}
		void setForDatabaseDetails(DatabaseInfo *dbInfo) {
			type = DB_DETAILS;
			phase = SYSTEM;
			this->dbInfo = dbInfo;
		}
		void setForUserInfoList(util::XArray<UserInfo*> *userInfoList) {
			type = USER_INFO;
			phase = GET;
			this->userInfoList = userInfoList;
		}
		void setForDbInfoList(bool dbNameSpecified, util::XArray<DatabaseInfo*> *dbInfoList) {
			type = DB_INFO;
			phase = GET;
			this->dbNameSpecified = dbNameSpecified;
			this->dbInfoList = dbInfoList;
		}
		void setForRemove(util::XArray<const util::XArray<uint8_t> *> *logRecordList) {
			type = REMOVE;
			phase = SYSTEM;
			this->logRecordList = logRecordList;
		}

		int8_t type;
		int8_t phase;
		int64_t count;
		const char8_t *s;
		DatabaseInfo *dbInfo;
		bool flag;
		util::XArray<UserInfo*> *userInfoList;
		bool dbNameSpecified;
		util::XArray<DatabaseInfo*> *dbInfoList;
		util::XArray<const util::XArray<uint8_t> *> *logRecordList;
	};

	static void decodeUserInfo(
		util::ByteStream<util::ArrayInStream> &in, UserInfo &userInfo);
	static void decodeDatabaseInfo(util::ByteStream<util::ArrayInStream> &in,
		DatabaseInfo &dbInfo, util::StackAllocator &alloc);
	
	void makeSchema(util::XArray<uint8_t> &containerSchema, const DUColumnInfo *columnInfoList, int n);

	void makeRow(
		util::StackAllocator &alloc, const ColumnInfo *columnInfoList,
		uint32_t columnNum, DUColumnValue *valueList, RowData &rowData);

	void makeRowKey(const char *name, RowKeyData &rowKey);
	void makeRowKey(DatabaseInfo &dbInfo, RowKeyData &rowKey);


	void putContainer(util::StackAllocator &alloc, 
		util::XArray<uint8_t> &containerInfo, const char *name,
		const util::DateTime now, const EventMonotonicTime emNow, const Request &request,
		util::XArray<const util::XArray<uint8_t>*> &logRecordList);
	bool checkContainer(
		EventContext &ec, Request &request, const char8_t *containerName);
	void putRow(
		EventContext &ec, Event &ev, const Request &request,
		const char8_t *containerName, DUColumnValue *cvList,
		util::XArray<const util::XArray<uint8_t>*> &logRecordList);
			
	void runWithRowKey(
		EventContext &ec, const TransactionManager::ContextSource &cxtSrc, 
		const char8_t *containerName, const RowKeyData &rowKey,
		DUGetInOut *option);
	void runWithTQL(
		EventContext &ec, const TransactionManager::ContextSource &cxtSrc, 
		const char8_t *containerName, const char8_t *tql,
		DUQueryInOut *option);

	void checkUserName(const char8_t *userName, bool detailed);
	void checkAdminUser(UserType userType);
	void checkDatabaseName(const char8_t *dbName);
	void checkConnectedDatabaseName(
		ConnectionOption &connOption, const char8_t *dbName);
	void checkConnectedUserName(
		ConnectionOption &connOption, const char8_t *userName);
	void checkPartitionIdForUsers(PartitionId pId);
	void checkDigest(const char8_t *digest, size_t maxStrLength);
	void checkPrivilegeSize(DatabaseInfo &dbInfo, size_t size);
	void checkModifiable(bool modifiable, bool value);


	void initializeMetaContainer(
		EventContext &ec, Event &ev, const Request &request,
		util::XArray<const util::XArray<uint8_t>*> &logRecordList);

	int64_t getCount(
		EventContext &ec, const Request &request, const char8_t *containerName);
	
	void checkUser(
		EventContext &ec, const Request &request, const char8_t *userName,
		bool &existFlag);
	void checkDatabase(
		EventContext &ec, const Request &request, DatabaseInfo &dbInfo);
	void checkDatabaseDetails(
		EventContext &ec, const Request &request, DatabaseInfo &dbInfo,
		bool &existFlag);

	void putDatabaseRow(
		EventContext &ec, Event &ev, const Request &request,
		DatabaseInfo &dbInfo, bool isCreate,
		util::XArray<const util::XArray<uint8_t>*> &logRecordList);

	void getUserInfoList(
		EventContext &ec, 
		const Request &request, const char8_t *userName,
		UserType userType, util::XArray<UserInfo*> &userInfoList);
	
private:
	void fetchRole(TransactionContext &txn, util::StackAllocator &alloc, 
		BaseContainer *container, ResultSet *rs, RoleType &role);
	void removeRowWithRowKey(TransactionContext &txn, util::StackAllocator &alloc, 
		const TransactionManager::ContextSource &cxtSrc, 
		BaseContainer *container, const RowKeyData &rowKey,
		util::XArray<const util::XArray<uint8_t> *> &logRecordList);
	
	void checkDigest(TransactionContext &txn, util::StackAllocator &alloc, 
		BaseContainer *container, ResultSet *rs, const char8_t *digest);
	bool checkPrivilege(TransactionContext &txn, util::StackAllocator &alloc, 
		BaseContainer *container, ResultSet *rs, DatabaseInfo &dbInfo);
	void makeUserInfoList(
		TransactionContext &txn, util::StackAllocator &alloc,
		BaseContainer &container, ResultSet &rs,
		util::XArray<UserInfo*> &userInfoList);
	void makeDatabaseInfoList(
		TransactionContext &txn, util::StackAllocator &alloc,
		BaseContainer &container, ResultSet &rs,
		util::XArray<DatabaseInfo*> &dbInfoList);
	void removeRowWithRS(TransactionContext &txn, util::StackAllocator &alloc, 
		const TransactionManager::ContextSource &cxtSrc,
		BaseContainer &container, ResultSet &rs,
		util::XArray<const util::XArray<uint8_t> *> &logRecordList);
};

/*!
	@brief Handles PUT_USER statement
*/
class PutUserHandler : public DbUserHandler {
public:
	void operator()(EventContext &ec, Event &ev);
private:
	void checkUserDetails(
		EventContext &ec, const Request &request, const char8_t *userName,
		const char8_t *digest, bool detailFlag, bool &existFlag);

	void putUserRow(
		EventContext &ec, Event &ev, const Request &request,
		UserInfo &userInfo,
		util::XArray<const util::XArray<uint8_t>*> &logRecordList);
};

/*!
	@brief Handles DROP_USER statement
*/
class DropUserHandler : public DbUserHandler {
public:
	void operator()(EventContext &ec, Event &ev);
private:
	void removeUserRowInDB(
		EventContext &ec, Event &ev,
		const Request &request, const char8_t *userName,
		util::XArray<const util::XArray<uint8_t> *> &logRecordList);
	void removeUserRow(
		EventContext &ec, Event &ev, const Request &request,
		const char8_t *userName,
		util::XArray<const util::XArray<uint8_t> *> &logRecordList);
};

/*!
	@brief Handles GET_USERS statement
*/
class GetUsersHandler : public DbUserHandler {
public:
	void operator()(EventContext &ec, Event &ev);
private:
	void checkNormalUser(UserType userType, const char8_t *userName);
	
	void makeUserInfoListForAdmin(util::StackAllocator &alloc,
		const char8_t *userName, util::XArray<UserInfo*> &userInfoList);
};

/*!
	@brief Handles PUT_DATABASE statement
*/
class PutDatabaseHandler : public DbUserHandler {
public:
	void operator()(EventContext &ec, Event &ev);
private:
	void setPrivilegeInfoListForAdmin(util::StackAllocator &alloc,
		const char8_t *userName, util::XArray<PrivilegeInfo *> &privilegeInfoList);
	void checkDatabase(
		EventContext &ec, const Request &request, DatabaseInfo &dbInfo,
		bool &existFlag);
};

/*!
	@brief Handles DROP_DATABASE statement
*/
class DropDatabaseHandler : public DbUserHandler {
public:
	void operator()(EventContext &ec, Event &ev);
private:
	void removeDatabaseRow(
		EventContext &ec, Event &ev,
		const Request &request, const char8_t *dbName, bool isAdmin,
		util::XArray<const util::XArray<uint8_t> *> &logRecordList);
};

/*!
	@brief Handles GET_DATABASES statement
*/
class GetDatabasesHandler : public DbUserHandler {
public:
	void operator()(EventContext &ec, Event &ev);
private:
	void makeDatabaseInfoListForPublic(
		util::StackAllocator &alloc,
		util::XArray<UserInfo *> &userInfoList,
		util::XArray<DatabaseInfo *> &dbInfoList);
	void getDatabaseInfoList(
		EventContext &ec, const Request &request,
		const char8_t *dbName, const char8_t *userName,
		util::XArray<DatabaseInfo *> &dbInfoList);
	
	void checkDatabaseInfoList(int32_t featureVersion, util::XArray<DatabaseInfo *> &dbInfoList);
};

/*!
	@brief Handles PUT_PRIVILEGE statement
*/
class PutPrivilegeHandler : public DbUserHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles DROP_PRIVILEGE statement
*/
class DropPrivilegeHandler : public DbUserHandler {
public:
	void operator()(EventContext &ec, Event &ev);
private:
	void removeDatabaseRow(
		EventContext &ec, Event &ev,
		const Request &request, DatabaseInfo &dbInfo,
		util::XArray<const util::XArray<uint8_t> *> &logRecordList);
};



/*!
	@brief Handles LOGIN statement
*/
class LoginHandler : public DbUserHandler {
public:
	/*!
		@brief Represents the information about AuthenticationAck
	*/
	struct AuthenticationAck {
		explicit AuthenticationAck(PartitionId pId) : pId_(pId) {}

		PartitionId pId_;
		ClusterVersionId clusterVer_;
		AuthenticationId authId_;

		PartitionId authPId_;
	};
	
	static const uint32_t MAX_APPLICATION_NAME_LEN = 64;

	void operator()(EventContext &ec, Event &ev);

protected:
	bool checkPublicDB(const char8_t *dbName);

	void decodeAuthenticationAck(
		util::ByteStream<util::ArrayInStream> &in, AuthenticationAck &ack);
	void encodeAuthenticationAckPart(EventByteOutStream &out,
		ClusterVersionId clusterVer, AuthenticationId authId, PartitionId authPId);

	void executeAuthenticationInternal(
		EventContext &ec,
		util::StackAllocator &alloc, TransactionManager::ContextSource &cxtSrc,
		const char8_t *userName, const char8_t *digest, const char8_t *dbName,
		UserType userType, int checkLevel, DatabaseId &dbId, RoleType &role);

private:
	void checkClusterName(std::string &clusterName);
	void checkApplicationName(const char8_t *applicationName);
	
	bool checkSystemDB(const char8_t *dbName);
	bool checkAdmin(util::String &userName);
	bool checkLocalAuthNode();
	
	void executeAuthentication(
		EventContext &ec, Event &ev,
		const NodeDescriptor &clientND, StatementId authStmtId,
		const char8_t *userName, const char8_t *digest,
		const char8_t *dbName, UserType userType);

};

/*!
	@brief Handles AUTHENTICATION statement requested from another node
*/
class AuthenticationHandler : public LoginHandler {
public:
	void operator()(EventContext &ec, Event &ev);
private:
	void replyAuthenticationAck(EventContext &ec, util::StackAllocator &alloc,
		const NodeDescriptor &ND, const AuthenticationAck &request,
		DatabaseId dbId, RoleType role, bool isNewSQL = false);
};
/*!
	@brief Handles AUTHENTICATION_ACK statement requested from another node
*/
class AuthenticationAckHandler : public LoginHandler {
public:
	void operator()(EventContext &ec, Event &ev);

	void authHandleError(
		EventContext &ec, AuthenticationAck &ack, std::exception &e);
private:
	void replySuccess(
			EventContext &ec, util::StackAllocator &alloc,
			StatementExecStatus status, const Request &request,
			const AuthenticationContext &authContext);
};


/*!
	@brief Handles CHECK_TIMEOUT event
*/
class CheckTimeoutHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
	void checkNoExpireTransaction(util::StackAllocator &alloc, PartitionId pId);

private:
	static const size_t MAX_OUTPUT_COUNT =
		100;  
	static const uint64_t TIMEOUT_CHECK_STATE_TRACE_COUNT =
		100;  

	std::vector<uint64_t> timeoutCheckCount_;

	void checkReplicationTimeout(EventContext &ec);
	void checkAuthenticationTimeout(EventContext &ec);
	void checkTransactionTimeout(
		EventContext &ec, const util::XArray<bool> &checkPartitionFlagList);
	void checkRequestTimeout(
		EventContext &ec, const util::XArray<bool> &checkPartitionFlagList);
	void checkResultSetTimeout(EventContext &ec);
	void checkKeepaliveTimeout(
		EventContext &ec, const util::XArray<bool> &checkPartitionFlagList);

	bool isTransactionTimeoutCheckEnabled(
		PartitionGroupId pgId, util::XArray<bool> &checkPartitionFlagList);
};

/*!
	@brief Handles unknown statement
*/
class UnknownStatementHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles unsupported statement
*/
class UnsupportedStatementHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles ignorable statement
*/
class IgnorableStatementHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles DATA_STORE_PERIODICALLY event
*/
class DataStorePeriodicallyHandler : public StatementHandler {
public:
	DataStorePeriodicallyHandler();
	~DataStorePeriodicallyHandler();

	void operator()(EventContext &ec, Event &ev);

	void setConcurrency(int32_t concurrency) {
		concurrency_ = concurrency;
		expiredCounterList_.assign(concurrency, 0);
		startPosList_.assign(concurrency, 0);
	}

private:
	static const uint64_t PERIODICAL_MAX_SCAN_COUNT =
		500;  

	static const uint64_t PERIODICAL_TABLE_SCAN_MAX_COUNT =
		5000;  

	static const uint64_t PERIODICAL_TABLE_SCAN_WAIT_COUNT =
		30;  

	PartitionId *pIdCursor_;
	int64_t currentLimit_;
	int64_t currentPos_;
	int64_t expiredCheckCount_;
	int64_t expiredCounter_;
	int32_t concurrency_;
	std::vector<int64_t> expiredCounterList_;
	std::vector<int64_t> startPosList_;
};

/*!
	@brief Handles ADJUST_STORE_MEMORY_PERIODICALLY event
*/
class AdjustStoreMemoryPeriodicallyHandler : public StatementHandler {
public:
	AdjustStoreMemoryPeriodicallyHandler();
	~AdjustStoreMemoryPeriodicallyHandler();

	void operator()(EventContext &ec, Event &ev);

private:
	PartitionId *pIdCursor_;
};

/*!
	@brief Handles BACK_GROUND event
*/
class BackgroundHandler : public StatementHandler {
public:
	BackgroundHandler();
	~BackgroundHandler();

	void operator()(EventContext &ec, Event &ev);
	int32_t getWaitTime(EventContext &ec, Event &ev, int32_t opeTime);
	uint64_t diffLsn(PartitionId pId);
private:
	LogSequentialNumber *lsnCounter_;
};

/*!
	@brief Handles CONTINUE_CREATE_INDEX/CONTINUE_ALTER_CONTAINER event
*/
class ContinueCreateDDLHandler : public StatementHandler {
public:
	ContinueCreateDDLHandler();
	~ContinueCreateDDLHandler();

	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles UPDATE_DATA_STORE_STATUS event
*/
class UpdateDataStoreStatusHandler : public StatementHandler {
public:
	UpdateDataStoreStatusHandler();
	~UpdateDataStoreStatusHandler();

	void operator()(EventContext &ec, Event &ev);
};


class SQLGetContainerHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
	void reply(EventContext &ec, Event &ev, TableSchemaInfo &message);
};


/*!
	@brief Handles REMOVE_ROW_BY_ID_SET statement
*/
class RemoveRowSetByIdHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief Handles REMOVE_ROW_BY_ID_SET statement
*/
class UpdateRowSetByIdHandler : public StatementHandler {
public:
	void operator()(EventContext &ec, Event &ev);
};

/*!
	@brief TransactionService
*/
class TransactionService {
public:
	TransactionService(ConfigTable &config,
		const EventEngine::Config &eeConfig,
		const EventEngine::Source &eeSource, const char *name);
	~TransactionService();

	void initialize(const ResourceSet &resourceSet);

	void start();
	void shutdown();
	void waitForShutdown();

	EventEngine *getEE();

	int32_t getWaitTime(EventContext &ec, Event *ev, int32_t opeTime,
			int64_t &executableCount, int64_t &afterCount,
			BackgroundEventType type);

	void checkNoExpireTransaction(util::StackAllocator &alloc, PartitionId pId);
	void changeTimeoutCheckMode(PartitionId pId,
		PartitionTable::PartitionStatus after, ChangePartitionType changeType,
		bool isToSubMaster, ClusterStats &stats);

	void enableTransactionTimeoutCheck(PartitionId pId);
	void disableTransactionTimeoutCheck(PartitionId pId);
	bool isTransactionTimeoutCheckEnabled(PartitionId pId) const;

	size_t getWorkMemoryByteSizeLimit() const;

	uint64_t getTotalReadOperationCount() const;
	uint64_t getTotalWriteOperationCount() const;

	uint64_t getTotalRowReadCount() const;
	uint64_t getTotalRowWriteCount() const;

	uint64_t getTotalBackgroundOperationCount() const;
	uint64_t getTotalNoExpireOperationCount() const;
	uint64_t getTotalAbortDDLCount() const;

	void incrementReadOperationCount(PartitionId pId);
	void incrementWriteOperationCount(PartitionId pId);

	void incrementBackgroundOperationCount(PartitionId pId);
	void incrementNoExpireOperationCount(PartitionId pId);
	void incrementAbortDDLCount(PartitionId pId);

	void addRowReadCount(PartitionId pId, uint64_t count);
	void addRowWriteCount(PartitionId pId, uint64_t count);
	TransactionManager *getManager() {
		return transactionManager_;
	}

	ResultSetHolderManager& getResultSetHolderManager();

	void requestUpdateDataStoreStatus(const Event::Source &eventSource, Timestamp time, bool force);

private:

	void setClusterHandler();

	static class StatSetUpHandler : public StatTable::SetUpHandler {
		virtual void operator()(StatTable &stat);
	} statSetUpHandler_;

	class StatUpdator : public StatTable::StatUpdator {
		virtual bool operator()(StatTable &stat);

	public:
		StatUpdator();
		TransactionService *service_;
		TransactionManager *manager_;
	} statUpdator_;

	EventEngine::Config eeConfig_;
	const EventEngine::Source eeSource_;
	EventEngine *ee_;

	bool initalized_;

	const PartitionGroupConfig pgConfig_;

	class Config : public ConfigTable::ParamHandler {
	public:
		Config(ConfigTable &configTable);

		size_t getWorkMemoryByteSizeLimit() const;

	private:
		void setUpConfigHandler(ConfigTable &configTable);
		virtual void operator()(
			ConfigTable::ParamId id, const ParamValue &value);

		util::Atomic<size_t> workMemoryByteSizeLimit_;
	} serviceConfig_;

	std::vector< util::Atomic<bool> > enableTxnTimeoutCheck_;

	std::vector<uint64_t> readOperationCount_;
	std::vector<uint64_t> writeOperationCount_;
	std::vector<uint64_t> rowReadCount_;
	std::vector<uint64_t> rowWriteCount_;
	std::vector<uint64_t> backgroundOperationCount_;
	std::vector<uint64_t> noExpireOperationCount_;
	std::vector<uint64_t> abortDDLCount_;

	static const int32_t TXN_TIMEOUT_CHECK_INTERVAL =
		3;  
	static const int32_t CHUNK_EXPIRE_CHECK_INTERVAL =
		1;  
	static const int32_t ADJUST_STORE_MEMORY_CHECK_INTERVAL =
		1; 

	ServiceThreadErrorHandler serviceThreadErrorHandler_;

	ConnectHandler connectHandler_;
	DisconnectHandler disconnectHandler_;
	LoginHandler loginHandler_;
	LogoutHandler logoutHandler_;
	GetPartitionAddressHandler getPartitionAddressHandler_;
	GetPartitionContainerNamesHandler getPartitionContainerNamesHandler_;
	GetContainerPropertiesHandler getContainerPropertiesHandler_;
	PutContainerHandler putContainerHandler_;
	PutLargeContainerHandler putLargeContainerHandler_;
	UpdateContainerStatusHandler updateContainerStatusHandler_;
	CreateLargeIndexHandler createLargeIndexHandler_;
	DropContainerHandler dropContainerHandler_;
	GetContainerHandler getContainerHandler_;
	CreateIndexHandler createIndexHandler_;
	DropIndexHandler dropIndexHandler_;
	CreateDropTriggerHandler createDropTriggerHandler_;
	FlushLogHandler flushLogHandler_;
	WriteLogPeriodicallyHandler writeLogPeriodicallyHandler_;
	CreateTransactionContextHandler createTransactionContextHandler_;
	CloseTransactionContextHandler closeTransactionContextHandler_;
	CommitAbortTransactionHandler commitAbortTransactionHandler_;
	PutRowHandler putRowHandler_;
	PutRowSetHandler putRowSetHandler_;
	RemoveRowHandler removeRowHandler_;
	UpdateRowByIdHandler updateRowByIdHandler_;
	RemoveRowByIdHandler removeRowByIdHandler_;
	GetRowHandler getRowHandler_;
	GetRowSetHandler getRowSetHandler_;
	QueryTqlHandler queryTqlHandler_;

	AppendRowHandler appendRowHandler_;
	QueryGeometryRelatedHandler queryGeometryRelatedHandler_;
	QueryGeometryWithExclusionHandler queryGeometryWithExclusionHandler_;
	GetRowTimeRelatedHandler getRowTimeRelatedHandler_;
	GetRowInterpolateHandler getRowInterpolateHandler_;
	AggregateHandler aggregateHandler_;
	QueryTimeRangeHandler queryTimeRangeHandler_;
	QueryTimeSamplingHandler queryTimeSamplingHandler_;
	FetchResultSetHandler fetchResultSetHandler_;
	CloseResultSetHandler closeResultSetHandler_;
	MultiCreateTransactionContextHandler multiCreateTransactionContextHandler_;
	MultiCloseTransactionContextHandler multiCloseTransactionContextHandler_;
	MultiPutHandler multiPutHandler_;
	MultiGetHandler multiGetHandler_;
	MultiQueryHandler multiQueryHandler_;
	ReplicationLogHandler replicationLogHandler_;
	ReplicationAckHandler replicationAckHandler_;
	ReplicationLog2Handler replicationLog2Handler_;
	ReplicationAck2Handler replicationAck2Handler_;
	AuthenticationHandler authenticationHandler_;
	AuthenticationAckHandler authenticationAckHandler_;
	PutUserHandler putUserHandler_;
	DropUserHandler dropUserHandler_;
	GetUsersHandler getUsersHandler_;
	PutDatabaseHandler putDatabaseHandler_;
	DropDatabaseHandler dropDatabaseHandler_;
	GetDatabasesHandler getDatabasesHandler_;
	PutPrivilegeHandler putPrivilegeHandler_;
	DropPrivilegeHandler dropPrivilegeHandler_;
	CheckTimeoutHandler checkTimeoutHandler_;

	UnsupportedStatementHandler unsupportedStatementHandler_;
	UnknownStatementHandler unknownStatementHandler_;
	IgnorableStatementHandler ignorableStatementHandler_;

	DataStorePeriodicallyHandler dataStorePeriodicallyHandler_;
	AdjustStoreMemoryPeriodicallyHandler adjustStoreMemoryPeriodicallyHandler_;
	BackgroundHandler backgroundHandler_;
	ContinueCreateDDLHandler createDDLContinueHandler_;

	UpdateDataStoreStatusHandler updateDataStoreStatusHandler_;

	ShortTermSyncHandler shortTermSyncHandler_;
	LongTermSyncHandler longTermSyncHandler_;
	SyncCheckEndHandler syncCheckEndHandler_;
	DropPartitionHandler dropPartitionHandler_;
	ChangePartitionStateHandler changePartitionStateHandler_;
	ChangePartitionTableHandler changePartitionTableHandler_;

	CheckpointOperationHandler checkpointOperationHandler_;
	SQLGetContainerHandler sqlGetContainerHandler_;

	ExecuteJobHandler executeHandler_;
	ControlJobHandler controlHandler_;

	TransactionManager *transactionManager_;

	SyncManager *syncManager_;
	DataStore *dataStore_;
	CheckpointService *checkpointService_;

	RemoveRowSetByIdHandler removeRowSetByIdHandler_;
	UpdateRowSetByIdHandler updateRowSetByIdHandler_;


public:

	PartitionGroupId getPartitionGroupId(PartitionId pId) {
		return pgConfig_.getPartitionGroupId(pId);
	}

	PartitionGroupId getPartitionGroupCount() {
		return pgConfig_.getPartitionGroupCount();
	}

	ResultSetHolderManager resultSetHolderManager_;
};


template<StatementMessage::OptionType T>
typename StatementMessage::OptionCoder<T>::ValueType
StatementMessage::OptionSet::get() const {
	typedef OptionCoder<T> Coder;
	typedef typename util::IsSame<
			typename Coder::ValueType, typename Coder::StorableType>::Type Same;
	EntryMap::const_iterator it = entryMap_.find(T);

	if (it == entryMap_.end()) {
		return Coder().getDefault();
	}

	return toValue<Coder>(
			it->second.get<typename Coder::StorableType>(), Same());
}

template<StatementMessage::OptionType T>
void StatementMessage::OptionSet::set(
		const typename OptionCoder<T>::ValueType &value) {
	typedef OptionCoder<T> Coder;
	typedef typename util::IsSame<
			typename Coder::ValueType, typename Coder::StorableType>::Type Same;
	ValueStorage storage;
	storage.set(toStorable<Coder>(value, Same()), getAllocator());
	entryMap_[T] = storage;
}

template<StatementMessage::OptionType T>
void StatementHandler::updateRequestOption(
		util::StackAllocator &alloc, Event &ev,
		const typename Message::OptionCoder<T>::ValueType &value) {
	Request request(alloc, getRequestSource(ev));
	util::XArray<uint8_t> remaining(alloc);

	try {
		EventByteInStream in(ev.getInStream());
		request.decode(in);

		decodeBinaryData(in, remaining, true);
	}
	catch (std::exception &e) {
		TXN_RETHROW_DECODE_ERROR(e, "");
	}

	try {
		request.optional_.set<T>(value);
		ev.getMessageBuffer().clear();

		EventByteOutStream out(ev.getOutStream());
		request.encode(out);

		encodeBinaryData(out, remaining.data(), remaining.size());
	}
	catch (std::exception &e) {
		TXN_RETHROW_ENCODE_ERROR(e, "");
	}
}

inline void TransactionService::incrementReadOperationCount(PartitionId pId) {
	++readOperationCount_[pId];
}

inline void TransactionService::incrementWriteOperationCount(PartitionId pId) {
	++writeOperationCount_[pId];
}

inline void TransactionService::addRowReadCount(
	PartitionId pId, uint64_t count) {
	rowReadCount_[pId] += count;
}

inline void TransactionService::addRowWriteCount(
	PartitionId pId, uint64_t count) {
	rowWriteCount_[pId] += count;
}

inline void TransactionService::incrementBackgroundOperationCount(PartitionId pId) {
	++backgroundOperationCount_[pId];
}
inline void TransactionService::incrementNoExpireOperationCount(PartitionId pId) {
	++noExpireOperationCount_[pId];
}
inline void TransactionService::incrementAbortDDLCount(PartitionId pId) {
	++abortDDLCount_[pId];
}



#endif
