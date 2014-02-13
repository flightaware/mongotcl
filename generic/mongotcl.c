/*
 * mongotcl - Tcl interface to MongoDB
 *
 * Copyright (C) 2014 FlightAware LLC
 *
 * freely redistributable under the Berkeley license
 */

#include "mongotcl.h"
#include <assert.h>


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_setMongoError --
 *
 *      Set an error message and error code based on an error from mongo
 *
 * Results:
 *      The Tcl errorCodee thing is set and an error message is set.
 *
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_setMongoError (Tcl_Interp *interp, mongo *conn) {
    char *errorString = NULL;
    char *errorCode = NULL;

	switch (conn->err) {
		case MONGO_CONN_SUCCESS: {
			return TCL_OK;
		}

		case MONGO_CONN_NO_SOCKET: {
			errorCode = "CONN_NO_SOCKET";
			break;
		}

		case MONGO_CONN_FAIL: {
			errorCode = "CONN_FAIL";
			break;
		}

		case MONGO_CONN_ADDR_FAIL: {
			errorCode = "CONN_ADDR_FAIL";
			break;
		}

		case MONGO_CONN_NOT_MASTER: {
			errorCode = "CONN_NOT_MASTER";
			break;
		}

		case MONGO_CONN_BAD_SET_NAME: {
			errorCode = "CONN_BAD_SET_NAME";
			break;
		}

		case MONGO_CONN_NO_PRIMARY: {
			errorCode = "CONN_NO_PRIMARY";
			break;
		}

		case MONGO_IO_ERROR: {
			errorCode = "CONN_IO_ERROR";
			break;
		}

		case MONGO_SOCKET_ERROR: {
			errorCode = "CONN_SOCKET_ERROR";
			break;
		}

		case MONGO_READ_SIZE_ERROR: {
			errorCode = "CONN_READ_SIZE_ERROR";
			break;
		}

		case MONGO_COMMAND_FAILED: {
			errorCode = "COMMAND_FAILED";
			break;
		}

		case MONGO_WRITE_ERROR: {
			errorCode = "WRITE_ERROR";
			break;
		}

		case MONGO_NS_INVALID: {
			errorCode = "NS_INVALID";
			break;
		}

		case MONGO_BSON_INVALID: {
			errorCode = "BSON_INVALID";
			break;
		}

		case MONGO_BSON_NOT_FINISHED: {
			errorCode = "BSON_NOT_FINISHED";
			break;
		}

		case MONGO_BSON_TOO_LARGE: {
			errorCode = "BSON_TOO_LARGE";
			break;
		}

		case MONGO_WRITE_CONCERN_INVALID: {
			errorCode = "WRITE_CONCERN_INVALID";
			break;
		}
    }

    Tcl_SetErrorCode (interp, "MONGO", errorCode, NULL);

    if (*conn->errstr != '\0') {
		errorString = conn->errstr;
    } else {
		errorString = errorCode;
    }

    Tcl_SetObjResult (interp, Tcl_NewStringObj (errorString, -1));
    return TCL_ERROR;
}


/*
 *--------------------------------------------------------------
 *
 * mongotcl_mongoObjectDelete -- command deletion callback routine.
 *
 * Results:
 *      ...destroys the mongo connection handle.
 *      ...frees memory.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */
void
mongotcl_mongoObjectDelete (ClientData clientData)
{
    mongotcl_clientData *md = (mongotcl_clientData *)clientData;

    assert (md->mongo_magic == MONGOTCL_MONGO_MAGIC);

    mongo_destroy(md->conn);
    ckfree((char *)md->conn);
    ckfree((char *)clientData);
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_mongoObjectObjCmd --
 *
 *    dispatches the subcommands of a mongo object command
 *
 * Results:
 *    stuff
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_mongoObjectObjCmd(ClientData cData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int         optIndex;
	mongotcl_clientData *md = (mongotcl_clientData *)cData;

    static CONST char *options[] = {
        "insert",
        "update",
        "insert_batch",
        "cursor",
		"find",
        "count",
        "init",
		"last_error",
		"prev_error",
		"remove",
		"create_index",
        "set_op_timeout",
        "client",
        "reconnect",
        "disconnect",
        "check_connection",
		"is_master",
		"write_concern",
		"run_command",
        "replica_set_init",
        "replica_set_add_seed",
        "replica_set_client",
        "clear_errors",
        "authenticate",
        "add_user",
        "drop_collection",
        "drop_db",
        NULL
    };

    enum options {
        OPT_INSERT,
        OPT_UPDATE,
        OPT_INSERT_BATCH,
        OPT_CURSOR,
		OPT_MONGO_FIND,
		OPT_COUNT,
        OPT_INIT,
		OPT_GET_LAST_ERROR,
		OPT_GET_PREV_ERROR,
		OPT_REMOVE,
        OPT_CREATE_INDEX,
        OPT_SET_OP_TIMEOUT,
        OPT_CLIENT,
		OPT_RECONNECT,
		OPT_DISCONNECT,
		OPT_CHECK_CONNECTION,
		OPT_IS_MASTER,
		OPT_WRITE_CONCERN,
		OPT_RUN_COMMAND,
        OPT_REPLICA_SET_INIT,
        OPT_REPLICA_SET_ADD_SEED,
        OPT_REPLICA_SET_CLIENT,
        OPT_CLEAR_ERRORS,
		OPT_CMD_AUTHENTICATE,
		OPT_CMD_ADD_USER,
		OPT_CMD_DROP_COLLECTION,
		OPT_CMD_DROP_DB,
    };

    /* basic validation of command line arguments */
    if (objc < 2) {
        Tcl_WrongNumArgs (interp, 1, objv, "subcommand ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj (interp, objv[1], options, "option", TCL_EXACT, &optIndex) != TCL_OK) {
		return TCL_ERROR;
    }

    switch ((enum options) optIndex) {
		case OPT_INSERT: {
			bson *bson;

			if (objc != 4) {
				Tcl_WrongNumArgs (interp, 2, objv, "namespace bson");
				return TCL_ERROR;
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[3], &bson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (mongo_insert (md->conn, Tcl_GetString(objv[2]), bson, md->write_concern) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}

			break;
		}

		case OPT_UPDATE: {
			bson *condBson;
			bson *opBson;
			int   suboptIndex;
			int   updateType;

			static CONST char *subOptions[] = {
				"basic",
				"multi",
				"upsert",
				NULL
			};

			enum suboptions {
				SUBOPT_UPDATE_BASIC,
				SUBOPT_UPDATE_MULTI,
				SUBOPT_UPDATE_UPSERT
			};

			if (objc < 5 || objc > 6) {
				Tcl_WrongNumArgs (interp, 2, objv, "namespace condBson opBson ?updateType?");
				return TCL_ERROR;
			}

			if (objc == 5) {
				suboptIndex = SUBOPT_UPDATE_BASIC;
			} else {
				if (Tcl_GetIndexFromObj (interp, objv[5], subOptions, "updateType", TCL_EXACT, &suboptIndex) != TCL_OK) {
					return TCL_ERROR;
				}
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[3], &condBson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[4], &opBson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			switch ((enum suboptions)suboptIndex) {
				case SUBOPT_UPDATE_BASIC: {
					updateType = MONGO_UPDATE_BASIC;
					break;
				}

				case SUBOPT_UPDATE_MULTI: {
					updateType = MONGO_UPDATE_MULTI;
					break;
				}

				case SUBOPT_UPDATE_UPSERT: {
					updateType = MONGO_UPDATE_UPSERT;
					break;
				}
			}

			if (mongo_update (md->conn, Tcl_GetString(objv[2]), condBson, opBson, updateType, md->write_concern) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}

			break;
		}

		case OPT_REMOVE: {
			bson *bson;

			if (objc != 4) {
				Tcl_WrongNumArgs (interp, 2, objv, "namespace bson");
				return TCL_ERROR;
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[3], &bson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (mongo_remove (md->conn, Tcl_GetString(objv[2]), bson, md->write_concern) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}

			break;
		}

		case OPT_WRITE_CONCERN: {
			int   suboptIndex;
			int   arg;

			static CONST char *subOptions[] = {
				"ignore_errors",
				"unacknowledged",
				"acknowledged",
				"replica_acknowledged",
				"journaled",
				NULL
			};

			enum suboptions {
				SUBOPT_IGNORE_ERRORS,
				SUBOPT_UNACKNOWLEDGED,
				SUBOPT_ACKNOWLEDGED,
				SUBOPT_REPLICA_ACKNOWLEDGED,
				SUBOPT_JOURNALED
			};

			if (objc < 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "concern_type ?concern_type?");
				return TCL_ERROR;
			}

			mongo_write_concern_init (md->write_concern);

			for (arg = 2; arg < objc; arg++) {

				if (Tcl_GetIndexFromObj (interp, objv[arg], subOptions, "updateType", TCL_EXACT, &suboptIndex) != TCL_OK) {
				return TCL_ERROR;
				}

				switch ((enum suboptions)suboptIndex) {
					case SUBOPT_IGNORE_ERRORS: {
						md->write_concern->w = -1;
						break;
					}

					case SUBOPT_UNACKNOWLEDGED: {
						md->write_concern->w = 1;
						break;
					}

					case SUBOPT_ACKNOWLEDGED: {
						md->write_concern->w = 2;
						break;
					}

					case SUBOPT_REPLICA_ACKNOWLEDGED: {
						md->write_concern->j = 1;
						break;
					}

					case SUBOPT_JOURNALED: {
						md->write_concern->fsync = 1;
						break;
					}
				}
			}

			mongo_write_concern_finish (md->write_concern);
			break;
		}

		case OPT_RUN_COMMAND: {
			char *database;
			bson *commandBson;
			bson *outBson;

			if (objc != 5) {
				Tcl_WrongNumArgs (interp, 2, objv, "db commandBson outBson");
				return TCL_ERROR;
			}

			database = Tcl_GetString(objv[2]);

			if (mongotcl_cmdNameObjToBson (interp, objv[3], &commandBson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[4], &outBson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (mongo_run_command (md->conn, database, commandBson, outBson) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}

			break;
		}


		case OPT_INSERT_BATCH: {
			bson **bsonList;
			int listObjc;
			int i;
			Tcl_Obj **listObjv;
			int flags = 0;

			if (objc < 4 || objc > 5) {
				Tcl_WrongNumArgs (interp, 2, objv, "namespace bsonList ?continue_on_error?");
				return TCL_ERROR;
			}

			if (objc == 5) {
				if (strcmp (Tcl_GetString (objv[4]), "continue_on_error") != 0) {
					Tcl_SetObjResult (interp, Tcl_NewStringObj ("fifth argument is not 'continue_on_error'", -1));
					return TCL_ERROR;
				}
				flags = MONGO_CONTINUE_ON_ERROR;
			}

			/* retrieve the list of bson objects */
			if (Tcl_ListObjGetElements (interp, objv[3], &listObjc, &listObjv) == TCL_ERROR) {
				Tcl_AddErrorInfo (interp, "while getting bson objects from list");
				return TCL_ERROR;
			}

			bsonList = (bson **)ckalloc (sizeof (bson *) * listObjc);

			for (i = 0; i < listObjc; i++) {
				if (mongotcl_cmdNameObjToBson (interp, listObjv[i], &bsonList[i]) == TCL_ERROR) {
					return TCL_ERROR;
				}
			}

			if (mongo_insert_batch (md->conn, Tcl_GetString(objv[2]), bsonList, listObjc, md->write_concern, flags) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}

			break;
		}

		case OPT_CURSOR: {
			char *commandName;
			char *namespace;

			if (objc != 4) {
				Tcl_WrongNumArgs (interp, 2, objv, "name namespace");
				return TCL_ERROR;
			}

			commandName = Tcl_GetString(objv[2]);
			namespace = Tcl_GetString(objv[3]);

			return mongotcl_createCursorObjCmd(interp, md->conn, commandName, namespace);
			break;
		}

		case OPT_MONGO_FIND: {
			char *ns;
			bson *bsonQuery;
			bson *bsonFields;
			int limit;
			int skip;
			int listObjc;
			int i;
			Tcl_Obj **listObjv;
			int cursorFlags = 0;
			mongo_cursor *cursor;

			static CONST char *subOptions[] = {
				"tailable",
				"slave_ok",
				"no_timeout",
				"exhaust",
				"partial",
				NULL
			};

			enum suboptions {
				SUBOPT_CURSOR_TAILABLE,
				SUBOPT_CURSOR_SLAVE_OK,
				SUBOPT_CURSOR_NO_TIMEOUT,
				SUBOPT_CURSOR_AWAIT_DATA,
				SUBOPT_CURSOR_EXHAUST,
				SUBOPT_CURSOR_PARTIAL
			};

			if (objc != 8) {
				Tcl_WrongNumArgs (interp, 2, objv, "namespace bsonQuery bsonFields limit skip options");
				return TCL_ERROR;
			}

			ns = Tcl_GetString (objv[2]);

			if (mongotcl_cmdNameObjToBson (interp, objv[3], &bsonQuery) == TCL_ERROR) {
				Tcl_AddErrorInfo (interp, " while locating query bson");
				return TCL_ERROR;
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[4], &bsonFields) == TCL_ERROR) {
				Tcl_AddErrorInfo (interp, " while locating query bson");
				return TCL_ERROR;
			}

			if (Tcl_GetIntFromObj (interp, objv[5], &limit) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (Tcl_GetIntFromObj (interp, objv[6], &skip) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (Tcl_ListObjGetElements (interp, objv[7], &listObjc, &listObjv) == TCL_ERROR) {
				Tcl_AddErrorInfo (interp, "while examining option list");
				return TCL_ERROR;
			}

			for (i = 0; i < listObjc; i++) {
				int suboptIndex;

				if (Tcl_GetIndexFromObj (interp, listObjv[i], subOptions, "indexOption", TCL_EXACT, &suboptIndex) != TCL_OK) {
				return TCL_ERROR;
				}

				switch ((enum suboptions)suboptIndex) {
				case SUBOPT_CURSOR_TAILABLE:
					cursorFlags |= MONGO_TAILABLE;
					break;

				case SUBOPT_CURSOR_SLAVE_OK:
					cursorFlags |= MONGO_SLAVE_OK;
					break;

				case SUBOPT_CURSOR_NO_TIMEOUT:
					cursorFlags |= MONGO_NO_CURSOR_TIMEOUT;
					break;

				case SUBOPT_CURSOR_AWAIT_DATA:
					cursorFlags |= MONGO_AWAIT_DATA;
					break;

				case SUBOPT_CURSOR_EXHAUST:
					cursorFlags |= MONGO_EXHAUST;
					break;

				case SUBOPT_CURSOR_PARTIAL:
					cursorFlags |= MONGO_PARTIAL;
					break;
				}
			}

			if ((cursor = mongo_find (md->conn, ns, bsonQuery, bsonFields, limit, skip, cursorFlags)) == NULL) {
				return TCL_ERROR;
			}

			break;
		}

		case OPT_COUNT: {
			bson *query;
			int count;

			if (objc < 4 || objc > 5) {
				Tcl_WrongNumArgs (interp, 2, objv, "db collection ?bson?");
				return TCL_ERROR;
			}

			if (objc == 4) {
				query = NULL;
			} else {
				if (mongotcl_cmdNameObjToBson (interp, objv[3], &query) == TCL_ERROR) {
					return mongotcl_setMongoError (interp, md->conn);
				}
			}

			if ((count = mongo_count (md->conn, Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), query)) == MONGO_ERROR) {
				return mongotcl_setMongoError (interp, md->conn);
			}
			Tcl_SetObjResult (interp, Tcl_NewIntObj (count));
			break;
		}

		case OPT_INIT: {
			if (objc != 2) {
				Tcl_WrongNumArgs (interp, 1, objv, "init");
				return TCL_ERROR;
			}

			mongo_init (md->conn);
			break;
		}

		case OPT_GET_LAST_ERROR: {
			bson *out;

			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "db");
				return TCL_ERROR;
			}

			if (mongo_cmd_get_last_error (md->conn, Tcl_GetString(objv[2]), out) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}

			break;
		}

		case OPT_GET_PREV_ERROR: {
			bson *out;

			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "db");
				return TCL_ERROR;
			}

			if (mongo_cmd_get_prev_error (md->conn, Tcl_GetString(objv[2]), out) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}

			break;
		}

		case OPT_CREATE_INDEX: {
			bson *keyBson;
			bson *outBson;
			int   suboptIndex;
			int   updateFlags = 0;

			static CONST char *subOptions[] = {
				"unique",
				"drop_dups",
				"background",
				"sparse",
				NULL
			};

			enum suboptions {
				SUBOPT_INDEX_UNIQUE,
				SUBOPT_INDEX_DROP_DUPS,
				SUBOPT_INDEX_BACKGROUND,
				SUBOPT_INDEX_SPARSE
			};

			if (objc < 5 || objc > 6) {
				Tcl_WrongNumArgs (interp, 2, objv, "namespace keyBson outBson ?optionList?");
				return TCL_ERROR;
			}

			/* set updatesFlags to 0.  If a list of updateFlags is present,
			 * parse the list.  treat anything not found as an error,
			 * set bitfields in the updateFlags variable according
			 * to options specified.
			 */
			if (objc == 6) {
				int listObjc;
				int i;
				Tcl_Obj **listObjv;

				if (Tcl_ListObjGetElements (interp, objv[5], &listObjc, &listObjv) == TCL_ERROR) {
					Tcl_AddErrorInfo (interp, "while examining option list");
					return TCL_ERROR;
				}

				for (i = 0; i < listObjc; i++) {
					if (Tcl_GetIndexFromObj (interp, listObjv[i], subOptions, "indexOption", TCL_EXACT, &suboptIndex) != TCL_OK) {
						return TCL_ERROR;
					}

					switch ((enum suboptions)suboptIndex) {
						case SUBOPT_INDEX_UNIQUE: {
							updateFlags |= MONGO_INDEX_UNIQUE;
							break;
						}

						case SUBOPT_INDEX_DROP_DUPS: {
							updateFlags |= MONGO_INDEX_DROP_DUPS;
							break;
						}

						case SUBOPT_INDEX_BACKGROUND: {
							updateFlags |= MONGO_INDEX_BACKGROUND;
							break;
						}

						case SUBOPT_INDEX_SPARSE: {
							updateFlags |= MONGO_INDEX_SPARSE;
							break;
						}
					}
				}
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[3], &keyBson) == TCL_ERROR) {
				Tcl_AddErrorInfo (interp, " while locating key bson");
				return TCL_ERROR;
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[4], &outBson) == TCL_ERROR) {
				Tcl_AddErrorInfo (interp, " while locating ultson");
				return TCL_ERROR;
			}

			if (mongo_create_index (md->conn, Tcl_GetString(objv[2]), keyBson, updateFlags, outBson) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}

			break;
		}

		case OPT_SET_OP_TIMEOUT: {
			int ms;

			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "timeoutMS");
				return TCL_ERROR;
			}
	
			if (Tcl_GetIntFromObj (interp, objv[2], &ms) == TCL_ERROR) {
				return TCL_ERROR;
			}

			mongo_set_op_timeout (md->conn, ms);
			break;
		}

		case OPT_CLIENT: {
			char *address;
			int port;

			if (objc != 4) {
				Tcl_WrongNumArgs (interp, 2, objv, "address port");
				return TCL_ERROR;
			}

			address = Tcl_GetString (objv[2]);
	
			if (Tcl_GetIntFromObj (interp, objv[3], &port) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (mongo_client (md->conn, address, port) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}
			break;
		}

		case OPT_RECONNECT: {
			if (objc != 2) {
				Tcl_WrongNumArgs (interp, 1, objv, "reconnect");
				return TCL_ERROR;
			}

			mongo_reconnect (md->conn);
			break;
		  }

		case OPT_DISCONNECT: {
			if (objc != 2) {
				Tcl_WrongNumArgs (interp, 1, objv, "disconnect");
				return TCL_ERROR;
			}

			mongo_disconnect (md->conn);
			break;
		}

		case OPT_CHECK_CONNECTION: {
			if (objc != 2) {
				Tcl_WrongNumArgs (interp, 1, objv, "check_connection");
				return TCL_ERROR;
			}

			if (mongo_check_connection (md->conn) == MONGO_OK) {
				Tcl_SetObjResult (interp, Tcl_NewIntObj(1));
			} else {
				Tcl_SetObjResult (interp, Tcl_NewIntObj(0));
			}
			break;
		}

		case OPT_IS_MASTER: {
			bson *bsonResult;
			int status = 0;

			if (objc < 2 || objc > 3) {
				Tcl_WrongNumArgs (interp, 1, objv, "is_master ?bsonResult?");
				return TCL_ERROR;
			}

			if (objc == 2) {
				status = mongo_cmd_ismaster (md->conn, NULL);
			} else {
				if (mongotcl_cmdNameObjToBson (interp, objv[2], &bsonResult) == TCL_ERROR) {
					Tcl_AddErrorInfo (interp, " while locating bson result object");
					return TCL_ERROR;
				}

				status = mongo_cmd_ismaster (md->conn, bsonResult);
			}

			Tcl_SetObjResult (interp, Tcl_NewBooleanObj (status));

			break;
		}

		case OPT_REPLICA_SET_INIT: {
			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "setname");
				return TCL_ERROR;
			}

			mongo_replica_set_init (md->conn, Tcl_GetString(objv[2]));
			break;
		}

		case OPT_REPLICA_SET_ADD_SEED: {
			char *address;
			int port;

			if (objc != 4) {
				Tcl_WrongNumArgs (interp, 2, objv, "address port");
				return TCL_ERROR;
			}

			address = Tcl_GetString (objv[2]);
	
			if (Tcl_GetIntFromObj (interp, objv[3], &port) == TCL_ERROR) {
				return TCL_ERROR;
			}

			mongo_replica_set_add_seed (md->conn, address, port);
			break;
		}

		case OPT_REPLICA_SET_CLIENT: {
			if (objc != 2) {
				Tcl_WrongNumArgs (interp, 1, objv, "replica_set_client");
				return TCL_ERROR;
			}

			if (mongo_replica_set_client (md->conn) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}
			break;
		}

		case OPT_CLEAR_ERRORS: {
			if (objc != 2) {
				Tcl_WrongNumArgs (interp, 1, objv, "clear_errors");
				return TCL_ERROR;
			}

			mongo_clear_errors (md->conn);
			break;
		}

		case OPT_CMD_AUTHENTICATE: {
			if (objc != 5) {
				Tcl_WrongNumArgs (interp, 2, objv, "db user pass");
				return TCL_ERROR;
			}

			if (mongo_cmd_authenticate (md->conn, Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), Tcl_GetString(objv[4])) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}
			break;
		}

		case OPT_CMD_ADD_USER: {
			if (objc != 5) {
				Tcl_WrongNumArgs (interp, 2, objv, "db user pass");
				return TCL_ERROR;
			}

			if (mongo_cmd_add_user (md->conn, Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), Tcl_GetString(objv[4])) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}
			break;
		}

		case OPT_CMD_DROP_COLLECTION: {
			bson *out;

			if (objc != 4) {
				Tcl_WrongNumArgs (interp, 2, objv, "db collect");
				return TCL_ERROR;
			}

			if (mongo_cmd_drop_collection (md->conn, Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), out) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}
			break;
		}

		case OPT_CMD_DROP_DB: {
			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "db");
				return TCL_ERROR;
			}

			if (mongo_cmd_drop_db (md->conn, Tcl_GetString(objv[2])) != MONGO_OK) {
				return mongotcl_setMongoError (interp, md->conn);
			}
			break;
		}
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_mongoObjCmd --
 *
 *      Create a mongo object...
 *
 *      mongo create my_mongo
 *      mongo create #auto
 *
 * The created object is invoked to do things with a MongoDB
 *
 * Results:
 *      A standard Tcl result.
 *
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
int
mongotcl_mongoObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    mongotcl_clientData *md;
    int                 optIndex;
    char               *commandName;
    int                 autoGeneratedName;

    static CONST char *options[] = {
        "create",
        NULL
    };

    enum options {
        OPT_CREATE
    };

    // basic command line processing
    if (objc != 3) {
        Tcl_WrongNumArgs (interp, 1, objv, "create name");
        return TCL_ERROR;
    }

    // argument must be one of the subOptions defined above
    if (Tcl_GetIndexFromObj (interp, objv[1], options, "option",
        TCL_EXACT, &optIndex) != TCL_OK) {
        return TCL_ERROR;
    }

    // allocate one of our mongo client data objects for Tcl and configure it
    md = (mongotcl_clientData *)ckalloc (sizeof (mongotcl_clientData));

    md->conn = (mongo *)ckalloc(sizeof(mongo));
    md->interp = interp;
    md->mongo_magic = MONGOTCL_MONGO_MAGIC;
    md->write_concern = (mongo_write_concern *)ckalloc(sizeof(mongo_write_concern));

    mongo_init (md->conn);

    mongo_write_concern_init (md->write_concern);
    md->write_concern->w = 1;
    mongo_write_concern_finish (md->write_concern);

    commandName = Tcl_GetString (objv[2]);

    // if commandName is #auto, generate a unique name for the object
    autoGeneratedName = 0;
    if (strcmp (commandName, "#auto") == 0) {
        static unsigned long nextAutoCounter = 0;
        char *objName;
        int    baseNameLength;

        objName = Tcl_GetStringFromObj (objv[0], &baseNameLength);
        baseNameLength += snprintf (NULL, 0, "%lu", nextAutoCounter) + 1;
        commandName = ckalloc (baseNameLength);
        snprintf (commandName, baseNameLength, "%s%lu", objName, nextAutoCounter++);
        autoGeneratedName = 1;
    }

    // create a Tcl command to interface to mongo
    md->cmdToken = Tcl_CreateObjCommand (interp, commandName, mongotcl_mongoObjectObjCmd, md, mongotcl_mongoObjectDelete);
    Tcl_SetObjResult (interp, Tcl_NewStringObj (commandName, -1));
    if (autoGeneratedName == 1) {
        ckfree(commandName);
    }
    return TCL_OK;
}

/* vim: set ts=4 sw=4 sts=4 noet : */
