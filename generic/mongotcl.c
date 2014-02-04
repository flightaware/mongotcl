/*
 * mongotcl
 */

#include "mongotcl.h"


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

    bson_destroy(bd->bson);
    ckfree((char *)bd->bson);
    ckfree((char *)clientData);
}

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
        "start_array",
        "finish_array",
	"start_object",
	"finish_object",
	"finish",
        NULL
    };

    enum options {
        OPT_INIT,
        OPT_APPEND_STRING,
        OPT_APPEND_INT,
        OPT_START_ARRAY,
        OPT_FINISH_ARRAY,
        OPT_START_OBJECT,
        OPT_FINISH_OBJECT,
        OPT_FINISH
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

	if (bson_append_string (bd->bson, Tcl_GetString (objv[2]), Tcl_GetString (objv[3])) == BSON_ERROR) {
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

	if (bson_append_int (bd->bson, Tcl_GetString (objv[2]), num) == BSON_ERROR) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
	break;
      }

      case OPT_START_ARRAY: {
	if (objc != 3) {
	    Tcl_WrongNumArgs (interp, 2, objv, "name");
	    return TCL_ERROR;
	}

	if (bson_append_start_array (bd->bson, Tcl_GetString (objv[2])) == BSON_ERROR) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_FINISH_ARRAY: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "finish_array");
	    return TCL_ERROR;
	}

	if (bson_append_finish_array (bd->bson) == BSON_ERROR) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_START_OBJECT: {
        break;
      }

      case OPT_FINISH_OBJECT: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "finish_object");
	    return TCL_ERROR;
	}

	if (bson_append_finish_object (bd->bson) == BSON_ERROR) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
        break;
      }

      case OPT_FINISH: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "finish");
	    return TCL_ERROR;
	}

	if (bson_append_finish_object (bd->bson) == BSON_ERROR) {
	    return mongotcl_setBsonError (interp, bd->bson);
	}
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
	"cursor_next",
	"cursor_init",
	"cursor_destroy",
	"cursor_set_query",
        "init",
	"create_index",
        "set_op_timeout",
        "client",
        "destroy",
        "replica_set_init",
        "replica_set_add_seed",
        "replica_set_client",
        NULL
    };

    enum options {
        OPT_INSERT,
        OPT_UPDATE,
        OPT_INSERT_BATCH,
        OPT_CURSOR,
        OPT_CURSOR_NEXT,
        OPT_CURSOR_INIT,
        OPT_CURSOR_DESTROY,
        OPT_CURSOR_SET_QUERY,
        OPT_INIT,
        OPT_CREATE_INDEX,
        OPT_SET_OP_TIMEOUT,
        OPT_CLIENT,
        OPT_DESTROY,
        OPT_REPLICA_SET_INIT,
        OPT_REPLICA_SET_ADD_SEED,
        OPT_REPLICA_SET_CLIENT
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
	  break;
      }

      case OPT_UPDATE: {
	break;
      }

      case OPT_INSERT_BATCH: {
        break;
      }

      case OPT_CURSOR: {
        break;
      }

      case OPT_CURSOR_NEXT: {
        break;
      }

      case OPT_CURSOR_INIT: {
        break;
      }

      case OPT_CURSOR_DESTROY: {
        break;
      }

      case OPT_CURSOR_SET_QUERY: {
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

      case OPT_CREATE_INDEX: {
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

	mongo_client (md->conn, address, port);
        break;
      }

      // NB OPT_DESTROY this is probably not needed and a bad idea
      case OPT_DESTROY: {
	if (objc != 2) {
	    Tcl_WrongNumArgs (interp, 1, objv, "destroy");
	    return TCL_ERROR;
	}

	mongo_destroy (md->conn);
        break;
      }

      case OPT_REPLICA_SET_INIT: {
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

	mongo_replica_set_client (md->conn);
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

