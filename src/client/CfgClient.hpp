/*
 * Copyright (c) 2018-2019, AT&T Intellectual Property Inc. All rights reserved.
 * Copyright (c) 2015 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 * swig has special rules for mapping to other languages. Therefore,
 * we have to return std::string as a return value (not a pointer or
 * reference) as well as only pass references as parameters.
 *
 * Also had some issues with default parameter values. Therefore, even
 * empty strings need to be passed to interfaces.
 */

#ifndef CFGCLIENT_HPP_
#define CFGCLIENT_HPP_

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct configd_conn;
struct configd_error;

void string2vec(std::string s, std::vector<std::string> &out);

class CfgClientFatalException
{
public:
	CfgClientFatalException (const std::string &msg) {
		_msg = msg;
	};
	~CfgClientFatalException() {};
	const std::string &what() const {
		return _msg;
	};

private:
	std::string _msg;
};

class CfgClientException
{
public:
	CfgClientException (const std::string &msg) {
		_msg = msg;
	};
	~CfgClientException() {};
	const std::string &what() const {
		return _msg;
	};

private:
	std::string _msg;
};

class CfgClient
{
public:
	/**
	 * CfgClient() creates a new connection to configd.
	 * This will create a connection to configd and will inherit
	 * a configuration session from the environment if one exists.
	 * The CfgClientFatalException will be thrown if a connection
	 * cannot be established.
	 */
	CfgClient() throw(CfgClientFatalException);
	/**
	 * ~CfgClient() disconnects from configd.
	 * This doesn't teardown configuration sessions automatically
	 * since this object may be used in a non-configuration or in a
	 * shared configuration context. Use SessionTeardown() to destroy sessions.
	 */
	~CfgClient();

	// Session API
	/**
	 * SessionAttach() attaches to the provided session id.
	 * If the session does not exist then an exception is raised.
	 */
	void SessionAttach(const std::string &sessid) throw(CfgClientException);
	/**
	 * SessionSetup() creates a new configuration session and makes that session
	 * the context for this instance object.
	 * Commonly the pid of the process is used as the session id, but this 
	 * may be any arbitrary string.
	 */
	void SessionSetup(const std::string &sessid) throw(CfgClientException);
	/**
	 * SessionTeardown() destroys a configuration session.
	 * Care should be taken that you really want to teardown the session,
	 * in some contexts it may not be appropriate for this client to destroy a
	 * session.
	 * All sessions should be destroyed as soon as they are no longer needed as
	 * configd maintains the state indefinitiely unless told to destroy it.
	 */
	void SessionTeardown() throw(CfgClientException);
	/**
	 * SessionChanged() reports whether the current session has any
	 * configuration changes.
	 */
	bool SessionChanged() throw(CfgClientException);
	/**
	 * SessionSaved() reports whether the current configuration session has
	 * been saved to the running configuration.
	 */
	bool SessionSaved() throw(CfgClientException);
	/**
	 * SessionMarkSaved() allows one to effect the saved state of the session.
	 * @deprecated SessionMarkSaved is DEPRECATED.
	 */
	void SessionMarkSaved() throw(CfgClientException);
	/**
	 * SessionMarkUnsaved() allows one to effect the saved state of the session.
	 * @deprecated SessionMarkUnsaved is DEPRECATED.
	 */
	void SessionMarkUnsaved() throw(CfgClientException);
	/**
	 * SessionLocked() reports whether or not the current session is locked.
	 * Locks can be taken to prevent multiple writers to a shared session.
	 * Locks also prevent changes during commit.
	 * A session's lock is released if the client that took the lock disconnects.
	 */
	bool SessionLocked() throw(CfgClientException);
	/**
	 * SessionLock() will attempt to lock the current session.
	 * If the session is currently locked then the exception will be raised.
	 */
	void SessionLock() throw(CfgClientException);
	/**
	 * SessionUnlock() will attempt to unlock the current session.
	 * If the session is currently not locked by this process,
	 * then the exception will be raised.
	 */
	void SessionUnlock() throw(CfgClientException);
	/**
	 * SessionExists() reports whether the current session still exists.
	 * This allows for testing if a shared session has been torn down by
	 * another instance.
	 */
	bool SessionExists() throw(CfgClientException);

	// Transaction APIs
	/**
	 * Set() creates a new path in the configuration datastore.
	 * The client needs to be attached to a configuration session
	 * for this call to work.
	 * If any error occurs that error is thrown as an exception.
	 * Creating a path that already exists is considered an error.
	 * @param path The path to the configuration represented as a vector
	 * @return Any informational messages that occurred during set
	 */
	std::string Set(const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * Delete() removes a path in the configuration datastore.
	 * The client needs to be attached to a configuration session
	 * for this call to work.
	 * If any error occurs that error is thrown as an exception.
	 * Deleting a path that does not exist is considered an error.
	 * @param path The path to the configuration represented as a vector
	 * @return Any informational messages that occurred during delete
	 */
	std::string Delete(const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * ValidatePath() checks that the path can be set
	 * @param path The path to the configuration represented as a vector
	 * @return Any informational messages that occurred during the validation
	 *         Invalid paths will throw an exception.
	 */
	std::string ValidatePath(const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * Commit() validates and applies the candidate configuration.
	 * The candidate configuration is applied to the running system resulting
	 * in the running configuration being updated if the transaction is
	 * successful.
	 * @param comment A comment that describes the changes during this commit.
	 *                An empty string is allowed for no description.
	 * @return Any informational messages reported during the commit.
	 */
	std::string Commit(const std::string &comment) throw(CfgClientException);
	/**
	 * Discard() throws away all pending (uncommitted) changes for this session.
	 */
	std::string Discard() throw(CfgClientException);
	/**
	 * Validate() checks that the candidate configuration meets all the
	 * constraints modeled in the schema.
	 */
	std::string Validate() throw(CfgClientException);
	/**
	 * Save() Saves the current running configuration to the 'saved'
	 * configuration.
	 */
	std::string Save() throw(CfgClientException);
	/**
	 * Load() Replaces the configuration with the config in the supplied file
	 * in the candidate database.
	 */
	bool Load(const std::string &file) throw(CfgClientException);

	/**
	 * Merge() Merges the configuration with the config in the supplied file
	 * in the candidate database.
	 */
	bool Merge(const std::string &file) throw(CfgClientException);

	// Template APIs
	/**
	 * TemplateGet() is DEPRECATED.
	 * @deprecated The documenation here is for understanding old users of this method.
	 * This returns a map consisting of a template of the configuration data
	 * that is compatible with older internal Vyatta software. The names
	 * of the fields of this map are using old terminology and the types
	 * represented are not valid according to the YANG spec.
	 * @param path The path to the configuration represented as a vector
	 * @return A map representing the values in the template.
	 *         No more documentation will be provided for this, if you don't
	 *         know what these values mean you shouldn't be using them.
	 */
	std::map<std::string, std::string> TemplateGet(const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * TemplateGetChildren() access the children of the schema node at a given path
	 * Template is an old name kept for historical reasons;
	 * this accesses the YANG schema tree.
	 * @param path The path in the schema tree.
	 * @return The children at the requested path.
	 */
	std::vector<std::string> TemplateGetChildren(const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * TemplateGetAllowed() runs the configd:allowed extension to get the
	 * help values for a value node in the schema tree.
	 * Template is an old name kept for historical reasons;
	 * this accesses the YANG schema tree.
	 * @param path The path in the schema tree.
	 * @return The allowed values for the requested path.
	 */
	std::vector<std::string> TemplateGetAllowed(const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * TemplateValidatePath() determine if the path is valid according to the schema.
	 * Template is an old name kept for historical reasons;
	 * this accesses the YANG schema tree.
	 * @param path The path in the schema tree.
	 * @return Whether the path is valid according to the schema.
	 */
	bool TemplateValidatePath(const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * TemplateValidatePathValues() determines if the path is valid according to the schema and validates all values according to the schema syntax.
	 * For historical reasons this is not the same as TemplateValidatePath, which
	 * does not evaluate syntax. Chances are this is the method one wants to use.
	 * Template is an old name kept for historical reasons;
	 * this accesses the YANG schema tree.
	 * @param path The path in the schema tree.
	 * @return Whether the path is valid according to the schema.
	 */
	bool TemplateValidateValues(const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * GetFeatures() Get map of schema ids and the corresponding
	 * enabled features.
	 * @return A map of schema features.
	 */
	std::map<std::string, std::vector<std::string> > GetFeatures(void) throw(CfgClientException);
	/**
	 * GetHelp() returns a map containing the children and their help strings
	 * @param path The path of the parent for which one wants help as a vector
	 * @param FromSchema informs configd to generate help from the schema
	 *        definition as well as from the data tree. If false help is from
	 *        data existing in the tree only.
	 */
	std::map<std::string, std::string> GetHelp(const std::vector<std::string> &path, bool FromSchema) throw(CfgClientException);

	// Node APIs
	/**
	 * Database defines what database to use for the query.
	 */
	enum Database {
		/**
		 * AUTO will select CANDIDATE when the client is attached to a session,
		 * RUNNING otherwise.
		 */
		AUTO,
		/**
		 * RUNNING is the current running configuration.
		 */
		RUNNING,
		/**
		 * CANDIDATE is the current session's temporary database.
		 * RUNNING if there is no session.
		 * In other words this tries to do the right thing.
		 */
		CANDIDATE,
		/**
		 * EFFECTIVE is DEPRECATED, do not use unless you know why you
		 * are using it.
		 * @deprecated This is treated as CANDIDATE but left for compatibility
		 * with older uses.
		 */
		EFFECTIVE
	};
	/**
	 * NodeExists() reports whether the path exists in the requested database.
	 * @param db  The database one wishes to query.
	 * @param path The path to the configuration represented as a vector.
	 * @return Whether the path exists in the requested database
	 */
	bool NodeExists(Database db, const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * NodeIsDefault() reports whether the path is a default in the requested database.
	 * @param db  The database one wishes to query.
	 * @param path The path to the configuration represented as a vector.
	 * @return Whether the path is a default in the requested database
	 */
	bool NodeIsDefault(Database db, const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * NodeGet() queries the database for the values at a given path.
	 * @param db  The database one wishes to query.
	 * @param path The path to the configuration represented as a vector.
	 * @return The children values of the given path.
	 */
	std::vector<std::string> NodeGet(Database db, const std::vector<std::string> &path) throw(CfgClientException);

	/**
	 * NodeStatus() represents the current change status of the node.
	 * When querying the RUNNING tree this will always be UNCHANGED.
	 * When querying the CANDIDATE tree this will be the change
	 * state when compared to the RUNNING tree.
	 */
	enum NodeStatus {
		/**
		 * UNCHANGED the values are the same in RUNNING and CANDIDATE
		 */
		UNCHANGED,
		/**
		 * CHANGED the values are different in RUNNING and CANDIDATE
		 */
		CHANGED,
		/**
		 * ADDED the value is new in the CANDIDATE tree
		 */
		ADDED,
		/**
		 * DELETED the value has been removed from the CANDIDATE tree.
		 */
		DELETED
	};
	/**
	 * NodeGetStatus() reports the status of a node in the tree.
	 * @param db  The database one wishes to query.
	 * @param path The path to the configuration represented as a vector.
	 * @return The status of the node see NodeStatus for descriptions.
	 */
	NodeStatus NodeGetStatus(Database db, const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * NodeType is DEPRECATED
	 * @deprecated The documenation here is for understanding old users of this method.
	 * NodeType represents the older Vyatta specific names for node types.
	 * These map roughly to the YANG types as follows.
	 * Lists in the Vyatta system are represented by the key being part of the
	 * path. These 'list entries' are also represented as containers.
	 *
	 * Note: There is not currently a direct replacement for this API instead
	 * we encourage the use of the TreeGet with JSON_ENCODING so that 
	 * the structure of the data returned reflects the schema information.
	 */
	enum NodeType {
		/**
		 * YANG leaf
		 */
		LEAF,
		/**
		 * YANG leaf-list
		 */
		MULTI,
		/**
		 * YANG container
		 */
		CONTAINER,
		/**
		 * YANG list
		 */
		TAG
	};
	/**
	 * NodeGetType() is DEPRECATED
	 * @deprecated The documenation here is for understanding old users of this method.
	 * @param path The path to the configuration represented as a vector.
	 * @return NodeType represents the older Vyatta specific names for node
	 *         types. See NodeType for information.
	 */
	NodeType NodeGetType(const std::vector<std::string> &path) throw(CfgClientException);

	/**
	 * The below APIs support the following strings as encodings:
	 *
	 * "json"
	 * JSON Encoding, similar to the proposed JSON YANG encoding but without
	 * the module names.
	 * Since this is an internal encoding module names are unimportant as nodes
	 * are unambiguous in the tree.
	 * https://tools.ietf.org/html/draft-ietf-netmod-yang-json-04
	 *
	 * "xml"
	 * NETCONF style XML encoding
	 *
	 * "internal"
	 * Vyatta's internal encoding with list entries represented as containers.
	 */

	/**
	 * TreeGetEncoding() provides access to sub-trees of the configuration database.
	 * These trees are encoded in the desired encoding and returned as a string.
	 * @param db The database to get the tree From.
	 * @param path The configuration path at which to root the sub-tree.
	 * @param encoding The desired encoding.
	 * @return A string in the desired encoding representing the tree at
	 *         the given location.
	 */
	std::string TreeGetEncoding(Database db, const std::vector<std::string> &path, const std::string &encoding) throw(CfgClientException);
	/**
	 * TreeGetFullEncoding() provides access to sub-trees of the operational datastore.
	 * The operational datastore consists of the configuration database and any
	 * modeled operational state data.
	 * These trees are encoded in the desired encoding and returned as a string.
	 * @param db The database to get the tree From.
	 * @param path The configuration path at which to root the sub-tree.
	 * @param encoding The desired encoding.
	 * @return A string in the desired encoding representing the tree at
	 *         the given location.
	 */
	std::string TreeGetFullEncoding(Database db, const std::vector<std::string> &path, const std::string &encoding) throw(CfgClientException);
	/**
	 * TreeGet() provides access to sub-trees of the configuration database.
	 * This call is equivalent to TreeGetEncoding with JSON_ENCODING.
	 * @param db The database to get the tree From.
	 * @param path The configuration path at which to root the sub-tree.
	 * @return A string in the desired encoding representing the tree at
	 *         the given location.
	 */
	std::string TreeGet(Database db, const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * TreeGetXML() provides access to sub-trees of the configuration database.
	 * This call is equivalent to TreeGetEncoding with XML_ENCODING.
	 * @param db The database to get the tree From.
	 * @param path The configuration path at which to root the sub-tree.
	 * @return A string in the desired encoding representing the tree at
	 *         the given location.
	 */
	std::string TreeGetXML(Database db, const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * TreeGetInternal() provides access to sub-trees of the configuration database.
	 * This call is equivalent to TreeGetEncoding with INTERNAL_ENCODING.
	 * @param db The database to get the tree From.
	 * @param path The configuration path at which to root the sub-tree.
	 * @return A string in the desired encoding representing the tree at
	 *         the given location.
	 */
	std::string TreeGetInternal(Database db, const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * TreeGetFull() provides access to sub-trees of the operational datastore.
	 * The operational datastore consists of the configuration database and any
	 * modeled operational state data.
	 * This call is equivalent to TreeGetFullEncoding with JSON_ENCODING.
	 * @param db The database to get the tree From.
	 * @param path The configuration path at which to root the sub-tree.
	 * @return A string in the desired encoding representing the tree at
	 *         the given location.
	 */
	std::string TreeGetFull(Database db, const std::vector<std::string> &path) throw(CfgClientException);
    /**
	 * TreeGetFullXML() provides access to sub-trees of the operational datastore.
	 * The operational datastore consists of the configuration database and any
	 * modeled operational state data.
	 * This call is equivalent to TreeGetFullEncoding with XML_ENCODING.
	 * @param db The database to get the tree From.
	 * @param path The configuration path at which to root the sub-tree.
	 * @return A string in the desired encoding representing the tree at
	 *         the given location.
	 */
	std::string TreeGetFullXML(Database db, const std::vector<std::string> &path) throw(CfgClientException);
	/**
	 * TreeGetFullInternal() provides access to sub-trees of the operational datastore.
	 * The operational datastore consists of the configuration database and any
	 * modeled operational state data.
	 * This call is equivalent to TreeGetFullEncoding with INTERNAL_ENCODING.
	 * @param db The database to get the tree From.
	 * @param path The configuration path at which to root the sub-tree.
	 * @return A string in the desired encoding representing the tree at
	 *         the given location.
	 */
	std::string TreeGetFullInternal(Database db, const std::vector<std::string> &path) throw(CfgClientException);

	/**
	 * CallRPC() Will call RPCs defined in yang data-models.
	 * @param ns The XML namespace for the model in which the RPC is defined.
	 * @param name The name of the RPC to call
	 * @param input The input definition of the schema defined in JSON_ENCODING.
	 * @return The output schema defined in the RPC in JSON_ENCODING.
	 */
	std::string CallRPC(const std::string ns, const std::string name, const std::string input) throw(CfgClientException);
	/**
	 * CallRPCXML() Will call RPCs defined in yang data-models.
	 * @param ns The XML namespace for the model in which the RPC is defined.
	 * @param name The name of the RPC to call
	 * @param input The input definition of the schema defined in XML_ENCODING.
	 * @return The output schema defined in the RPC in XML_ENCODING.
	 */
	std::string CallRPCXML(const std::string ns, const std::string name, const std::string input) throw(CfgClientException);
	/**
	 * EditConfigXML will perform a  RFC-6241 edit-config operation, see RFC-6241 for more information.
	 * @param target The target datastore, can be "candidate", "running".
	 * @param default_operation The operation to perform on any node that doesn't specify the operation, may be "merge", "replace", or "none".
	 * @param test_option The testing option, may be "test-then-set", "set", or "test-only".
	 * @param error_option The operation to perform when an error is encountered, may be "stop-on-error", "continue-on-error", or "rollback-on-error".
	 * @param config The XML data for an edit-config operation rooted at "<config>".
	 */
	std::string EditConfigXML(const std::string target, const std::string default_operation, const std::string test_option, const std::string error_option, const std::string config) throw(CfgClientException);

private:
	std::string _sessionid;
	struct configd_conn *_conn;
};

#endif
