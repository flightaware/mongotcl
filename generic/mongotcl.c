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
 *--------------------------------------------------------------
 *
 * mongotcl_bsonObjectDelete -- command deletion callback routine.
 *
 * Results:
 *      ...frees the bson object.
 *      ...frees memory
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */
void
mongotcl_bsonObjectDelete (ClientData clientData)
{
    mongotcl_bsonClientData *bd = (mongotcl_bsonClientData *)clientData;

    assert (bd->bson_magic == MONGOTCL_BSON_MAGIC);

    bson_destroy(bd->bson);
    ckfree((char *)bd->bson);
    ckfree((char *)clientData);
}


/*
 *--------------------------------------------------------------
 *
 * mongotcl_setBsonError -- command deletion callback routine.
 *
 * Results:
 *      ...create an error message based on bson object error fields.
 *      ...set errorCode based on the same bson object error fields.
 *
 *      return TCL_ERROR
 *
 *--------------------------------------------------------------
 */
int
mongotcl_setBsonError (Tcl_Interp *interp, bson *bson) {
    Tcl_Obj *list = Tcl_NewObj();
    Tcl_Obj *errorCodeList = Tcl_NewObj();

    if (bson->err & BSON_NOT_UTF8) {
	Tcl_AddErrorInfo (interp, "bson not utf8");
	Tcl_ListObjAppendElement (interp, list, Tcl_NewStringObj("NOT_UTF8",-1));
    }

    if (bson->err & BSON_FIELD_HAS_DOT) {
	Tcl_AddErrorInfo (interp, "bson field has dot");
	Tcl_ListObjAppendElement (interp, list, Tcl_NewStringObj("HAS_DOT",-1));
    }

    if (bson->err & BSON_FIELD_INIT_DOLLAR) {
	Tcl_AddErrorInfo (interp, "bson field has initial dollar sign");
	Tcl_ListObjAppendElement (interp, list, Tcl_NewStringObj("INIT_DOLLAR",-1));
    }

    if (bson->err & BSON_ALREADY_FINISHED) {
	Tcl_AddErrorInfo (interp, "bson already finished");
	Tcl_ListObjAppendElement (interp, list, Tcl_NewStringObj("ALREADY_FINISHED",-1));
    }

    Tcl_ListObjAppendElement(interp, errorCodeList, Tcl_NewStringObj("BSON",-1));
    Tcl_ListObjAppendElement(interp, errorCodeList, list);

    Tcl_SetObjErrorCode (interp, errorCodeList);

    return TCL_ERROR;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_cmdNameObjToBson --
 *
 *    Take a command name, find the Tcl command info structure, return
 *    a pointer to the bson embedded in the clientData of the object.
 *
 *----------------------------------------------------------------------
 */
static int
mongotcl_cmdNameObjToBson (Tcl_Interp *interp, Tcl_Obj *commandNameObj, bson **bson) {
    Tcl_CmdInfo	cmdInfo;

    if (!Tcl_GetCommandInfo (interp, Tcl_GetString(commandNameObj), &cmdInfo)) {
	return TCL_ERROR;
    }

    if (cmdInfo.objClientData == NULL || ((mongotcl_bsonClientData *)cmdInfo.objClientData)->bson_magic != MONGOTCL_BSON_MAGIC) {
	Tcl_AppendResult (interp, "Error: '", Tcl_GetString (commandNameObj), "' is not a bson object", NULL);
	return TCL_ERROR;
    }

    *bson = ((mongotcl_bsonClientData *)cmdInfo.objClientData)->bson;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_bsonObjectObjCmd --
 *
 *    dispatches the subcommands of a bson object command
 *
 * Results:
 *    stuff
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_bsonObjectObjCmd(ClientData cData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int         optIndex;
    mongotcl_bsonClientData *bd = (mongotcl_bsonClientData *)cData;

    static CONST char *options[] = {
        "init",
        "string",
        "int",
	"kvlist",
        "start_array",
        "finish_array",
	"start_object",
	"finish_object",
	"new_oid",
	"finish",
	"print",
        NULL
    };

    enum options {
        OPT_INIT,
        OPT_APPEND_STRING,
        OPT_APPEND_INT,
	OPT_APPEND_KVLIST,
        OPT_APPEND_START_ARRAY,
        OPT_APPEND_FINISH_ARRAY,
        OPT_APPEND_START_OBJECT,
        OPT_APPEND_FINISH_OBJECT,
	OPT_APPEND_NEW_OID,
        OPT_FINISH,
	OPT_PRINT
    };

    /* basic validation of command line arguments */
    if (objc < 2) {
        Tcl_WrongNumArgs (interp, 1, objv, "subcommand ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj (interp, objv[1], options, "option",
	TCL_EXACT, &optIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) optIndex) {
      case OPT_INIT: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "init");
	    return TCL_ERROR;
	}

	bson_init (bd->bson);
	break;
      }

      case OPT_APPEND_STRING: {
	if (objc != 4) {
	    Tcl_WrongNumArgs (interp, 2, objv, "key value");
	    return TCL_ERROR;
	}

	if (bson_append_string (bd->bson, Tcl_GetString (objv[2]), Tcl_GetString (objv[3])) != BSON_OK) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
	break;
      }

      case OPT_APPEND_INT: {
	int num;

	if (objc != 4) {
	    Tcl_WrongNumArgs (interp, 2, objv, "key value");
	    return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj (interp, objv[3], &num) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	if (bson_append_int (bd->bson, Tcl_GetString (objv[2]), num) != BSON_OK) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
	break;
      }

      case OPT_APPEND_KVLIST: {
	int listObjc;
	int i;
	Tcl_Obj **listObjv;

	if (objc != 3) {
	    Tcl_WrongNumArgs (interp, 2, objv, "list");
	    return TCL_ERROR;
	}

	if (Tcl_ListObjGetElements (interp, objv[2], &listObjc, &listObjv) == TCL_ERROR) {
	    Tcl_AddErrorInfo (interp, "while getting bson objects from list");
	    return TCL_ERROR;
	}

	if (listObjc & 1) {
	    Tcl_SetObjResult (interp, Tcl_NewStringObj ("list must have even number of elements", -1));
	    return TCL_ERROR;
	}

	for (i = 0; i < listObjc; i += 2) {
	    if (bson_append_string (bd->bson, Tcl_GetString (listObjv[i]), Tcl_GetString (listObjv[i + 1])) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	}

	break;
      }

      case OPT_APPEND_START_ARRAY: {
	if (objc != 3) {
	    Tcl_WrongNumArgs (interp, 2, objv, "name");
	    return TCL_ERROR;
	}

	if (bson_append_start_array (bd->bson, Tcl_GetString (objv[2])) != BSON_OK) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_APPEND_FINISH_ARRAY: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "finish_array");
	    return TCL_ERROR;
	}

	if (bson_append_finish_array (bd->bson) != BSON_OK) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_APPEND_START_OBJECT: {
	if (objc != 3) {
	    Tcl_WrongNumArgs (interp, 2, objv, "name");
	    return TCL_ERROR;
	}

	if (bson_append_start_object (bd->bson, Tcl_GetString (objv[2])) != BSON_OK) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_APPEND_FINISH_OBJECT: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "finish_object");
	    return TCL_ERROR;
	}

	if (bson_append_finish_object (bd->bson) != BSON_OK) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_APPEND_NEW_OID: {
	if (objc != 3) {
	    Tcl_WrongNumArgs (interp, 2, objv, "name");
	    return TCL_ERROR;
	}

	if (bson_append_new_oid (bd->bson, Tcl_GetString (objv[2])) != BSON_OK) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_FINISH: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "finish");
	    return TCL_ERROR;
	}

	if (bson_append_finish_object (bd->bson) != BSON_OK) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_PRINT: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "print");
	    return TCL_ERROR;
	}

	bson_print (bd->bson);
        break;
      }
    }

    return TCL_OK;
}


/*
 *--------------------------------------------------------------
 *
 * mongotcl_cursorObjectDelete -- command deletion callback routine.
 *
 * Results:
 *      ...frees the mongo cursor object.
 *      ...frees memory
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */
void
mongotcl_cursorObjectDelete (ClientData clientData)
{
    mongotcl_cursorClientData *mc = (mongotcl_cursorClientData *)clientData;

    assert (mc->cursor_magic == MONGOTCL_CURSOR_MAGIC);

    mongo_cursor_destroy(mc->cursor);
    ckfree((char *)mc->cursor);
    ckfree((char *)clientData);
}


/*
 *--------------------------------------------------------------
 *
 * mongotcl_setCursorError -- command deletion callback routine.
 *
 * Results:
 *      ...create an error message based on mongo cursor object error fields.
 *      ...set errorCode based on the same mongo cursor object error fields.
 *
 *      return TCL_ERROR
 *
 *--------------------------------------------------------------
 */
int
mongotcl_setCursorError (Tcl_Interp *interp, mongo_cursor *cursor) {
    char *errorCode = NULL;

    switch (cursor->err) {
	case MONGO_CURSOR_EXHAUSTED: {
	    errorCode = "CURSOR_EXHAUSTED";
	    break;
	}

	case MONGO_CURSOR_INVALID: {
	    errorCode = "CURSOR_INVALID";
	    break;
	}

	case MONGO_CURSOR_PENDING: {
	    errorCode = "CURSOR_PENDING";
	    break;
	}

	case MONGO_CURSOR_QUERY_FAIL: {
	    errorCode = "CURSOR_QUERY_FAIL";
	    break;
	}

	case MONGO_CURSOR_BSON_ERROR: {
	    errorCode = "CURSOR_BSON_ERROR";
	    break;
	}
    }

    Tcl_SetErrorCode (interp, "MONGO", errorCode, NULL);

    Tcl_SetObjResult (interp, Tcl_NewStringObj (errorCode, -1));
    return TCL_ERROR;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_cmdNameObjToCursor --
 *
 *    Take a command name, find the Tcl command info structure, return
 *    a pointer to the bson embedded in the clientData of the object.
 *
 *----------------------------------------------------------------------
 */
static int
mongotcl_cmdNameObjToCursor (Tcl_Interp *interp, Tcl_Obj *commandNameObj, mongo_cursor **cursor) {
    Tcl_CmdInfo	cmdInfo;

    if (!Tcl_GetCommandInfo (interp, Tcl_GetString(commandNameObj), &cmdInfo)) {
	return TCL_ERROR;
    }

    if (cmdInfo.objClientData == NULL || ((mongotcl_cursorClientData *)cmdInfo.objClientData)->cursor_magic != MONGOTCL_CURSOR_MAGIC) {
	Tcl_AppendResult (interp, "Error: '", Tcl_GetString (commandNameObj), "' is not a mongo cursor object", NULL);
	return TCL_ERROR;
    }

    *cursor = ((mongotcl_cursorClientData *)cmdInfo.objClientData)->cursor;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_cursorObjectObjCmd --
 *
 *    dispatches the subcommands of a mongo cursor object command
 *
 * Results:
 *    stuff
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_cursorObjectObjCmd(ClientData cData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int         optIndex;
    mongotcl_cursorClientData *mc = (mongotcl_cursorClientData *)cData;

    static CONST char *options[] = {
        "init",
        "set_query",
        "set_fields",
        "set_skip",
        "set_limit",
	"set_options",
	"data",
	"bson",
	"next",
        NULL
    };

    enum options {
        OPT_CURSOR_INIT,
        OPT_CURSOR_SET_QUERY,
        OPT_CURSOR_SET_FIELDS,
        OPT_CURSOR_SET_SKIP,
        OPT_CURSOR_SET_LIMIT,
        OPT_CURSOR_SET_OPTIONS,
        OPT_CURSOR_DATA,
	OPT_CURSOR_BSON,
        OPT_CURSOR_NEXT
    };

    /* basic validation of command line arguments */
    if (objc < 2) {
        Tcl_WrongNumArgs (interp, 1, objv, "subcommand ?args?");
        return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj (interp, objv[1], options, "option",
	TCL_EXACT, &optIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) optIndex) {
      case OPT_CURSOR_INIT: {
	char *ns;

	if (objc != 3) {
	    Tcl_WrongNumArgs (interp, 2, objv, "namespace");
	    return TCL_ERROR;
	}

	ns = Tcl_GetString (objv[2]);
	mongo_cursor_init (mc->cursor, mc->conn, ns);
	break;
      }

      case OPT_CURSOR_SET_QUERY: {
	break;
      }

      case OPT_CURSOR_SET_FIELDS: {
	break;
      }

      case OPT_CURSOR_SET_SKIP: {
	break;
      }

      case OPT_CURSOR_SET_LIMIT: {
	break;
      }

      case OPT_CURSOR_SET_OPTIONS: {
	break;
      }

      case OPT_CURSOR_DATA: {
	break;
      }

      case OPT_CURSOR_BSON: {
	break;
      }

      case OPT_CURSOR_NEXT: {
	break;
      }

    }

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_bsonObjCmd --
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
mongotcl_bsonObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    mongotcl_bsonClientData *bd;
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
    bd = (mongotcl_bsonClientData *)ckalloc (sizeof (mongotcl_bsonClientData));

    bd->bson = (bson *)ckalloc(sizeof(bson));
    bd->interp = interp;

    bson_init (bd->bson);

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
    bd->cmdToken = Tcl_CreateObjCommand (interp, commandName, mongotcl_bsonObjectObjCmd, bd, mongotcl_bsonObjectDelete);
    Tcl_SetObjResult (interp, Tcl_NewStringObj (commandName, -1));
    if (autoGeneratedName == 1) {
        ckfree(commandName);
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_createCursorObjCmd --
 *
 *      Create a mongo cursor object...
 *
 *      mongo_cursor create my_mongo
 *      mongo_cursor create #auto
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
mongotcl_createCursorObjCmd(Tcl_Interp *interp, mongo *conn, char *commandName)
{
    mongotcl_cursorClientData *mc;
    int                 autoGeneratedName;

    // allocate one of our mongo client data objects for Tcl and configure it
    mc = (mongotcl_cursorClientData *)ckalloc (sizeof (mongotcl_cursorClientData));

    mc->interp = interp;
    mc->conn = conn;
    mc->cursor = (mongo_cursor *)ckalloc(sizeof(mongo_cursor));

    // if commandName is #auto, generate a unique name for the object
    autoGeneratedName = 0;
    if (strcmp (commandName, "#auto") == 0) {
        static unsigned long nextAutoCounter = 0;
        char *objName;
        int    baseNameLength;

        baseNameLength = strlen(commandName) + snprintf (NULL, 0, "%lu", nextAutoCounter) + 1;
        commandName = ckalloc (baseNameLength);
        snprintf (commandName, baseNameLength, "%s%lu", objName, nextAutoCounter++);
        autoGeneratedName = 1;
    }

    // create a Tcl command to interface to mongo
    mc->cmdToken = Tcl_CreateObjCommand (interp, commandName, mongotcl_cursorObjectObjCmd, mc, mongotcl_cursorObjectDelete);
    Tcl_SetObjResult (interp, Tcl_NewStringObj (commandName, -1));
    if (autoGeneratedName == 1) {
        ckfree(commandName);
    }
    return TCL_OK;
}


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
	"create_index",
        "set_op_timeout",
        "client",
        "reconnect",
        "disconnect",
        "check_connection",
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
        OPT_CURSOR_INIT,
	OPT_MONGO_FIND,
	OPT_COUNT,
        OPT_INIT,
	OPT_GET_LAST_ERROR,
	OPT_GET_PREV_ERROR,
        OPT_CREATE_INDEX,
        OPT_SET_OP_TIMEOUT,
        OPT_CLIENT,
	OPT_RECONNECT,
	OPT_DISCONNECT,
	OPT_CHECK_CONNECTION,
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

    if (Tcl_GetIndexFromObj (interp, objv[1], options, "option",
	TCL_EXACT, &optIndex) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) optIndex) {
      case OPT_INSERT: {
	bson *bson;

	if (objc != 4) {
	    Tcl_WrongNumArgs (interp, 1, objv, "namespace bson");
	    return TCL_ERROR;
	}

	if (mongotcl_cmdNameObjToBson (interp, objv[3], &bson) == TCL_ERROR) {
	    return TCL_ERROR;
	}

	if (mongo_insert (md->conn, Tcl_GetString(objv[2]), bson, 0) != MONGO_OK) {
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
	    case SUBOPT_UPDATE_BASIC:
		updateType = MONGO_UPDATE_BASIC;
		break;

	    case SUBOPT_UPDATE_MULTI:
		updateType = MONGO_UPDATE_MULTI;
		break;

	    case SUBOPT_UPDATE_UPSERT:
		updateType = MONGO_UPDATE_UPSERT;
		break;
	}

	if (mongo_update (md->conn, Tcl_GetString(objv[2]), condBson, opBson, updateType, 0) != MONGO_OK) {
	    return mongotcl_setMongoError (interp, md->conn);
	}

	break;
      }

      case OPT_INSERT_BATCH: {
	bson **bsonList;
	int listObjc;
	int i;
	Tcl_Obj **listObjv;

	if (objc != 4) {
	    Tcl_WrongNumArgs (interp, 1, objv, "namespace bsonList");
	    return TCL_ERROR;
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

	if (mongo_insert_batch (md->conn, Tcl_GetString(objv[2]), bsonList, listObjc, NULL, 0) != MONGO_OK) {
	    return mongotcl_setMongoError (interp, md->conn);
	}

        break;
      }

      case OPT_CURSOR_INIT: {
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
	    Tcl_AddErrorInfo (interp, "while locating query bson");
	    return TCL_ERROR;
	}

	if (mongotcl_cmdNameObjToBson (interp, objv[4], &bsonFields) == TCL_ERROR) {
	    Tcl_AddErrorInfo (interp, "while locating query bson");
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

	if (objc != 4) {
	    Tcl_WrongNumArgs (interp, 2, objv, "db collection");
	    return TCL_ERROR;
	}

	if (mongo_count (md->conn, Tcl_GetString(objv[2]), Tcl_GetString(objv[3]), query) != MONGO_OK) {
	    return mongotcl_setMongoError (interp, md->conn);
		return mongotcl_setMongoError (interp, md->conn);
	}
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
	    Tcl_WrongNumArgs (interp, 1, objv, "db");
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
	    Tcl_WrongNumArgs (interp, 1, objv, "db");
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
		    case SUBOPT_INDEX_UNIQUE:
			updateFlags |= MONGO_INDEX_UNIQUE;
			break;

		    case SUBOPT_INDEX_DROP_DUPS:
			updateFlags |= MONGO_INDEX_DROP_DUPS;
			break;

		    case SUBOPT_INDEX_BACKGROUND:
			updateFlags |= MONGO_INDEX_BACKGROUND;
			break;

		    case SUBOPT_INDEX_SPARSE:
			updateFlags |= MONGO_INDEX_SPARSE;
			break;
		}
	    }
	}

	if (mongotcl_cmdNameObjToBson (interp, objv[3], &keyBson) == TCL_ERROR) {
	    Tcl_AddErrorInfo (interp, "while locating key bson");
	    return TCL_ERROR;
	}

	if (mongotcl_cmdNameObjToBson (interp, objv[4], &outBson) == TCL_ERROR) {
	    Tcl_AddErrorInfo (interp, "while locating ultson");
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

    mongo_init (md->conn);

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

