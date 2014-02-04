/*
 * mongotcl
 */

#include "mongotcl.h"



/*
 *--------------------------------------------------------------
 *
 * mongotcl_mongoObjectDelete -- command deletion callback routine.
 *
 * Results:
 *      ...frees the mongo generator handle if it exists.
 *      ...frees the Tcl Dynamic string we use to build up the JSON.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */
void
mongotcl_mongoObjectDelete (ClientData clientData)
{
    mongotcl_clientData *mongoData = (mongotcl_clientData *)clientData;

    mongo_destroy(mongoData->conn);
    ckfree(mongoData->conn);
    ckfree(clientData);
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
    int         arg;
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

    if (Tcl_GetIndexFromObj (interp, objv[arg], options, "option",
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
	
	if (Tcl_GetIntFromObj (interp, objv[3], &ms) == TCL_ERROR) {
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
	
	if (Tcl_GetIntFromObj (interp, objv[3], &ms) == TCL_ERROR) {
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
    mongotcl_clientData *mongoData;
    int                 optIndex;
    int                 suboptIndex;
    int                 i;
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
    mongoData = (mongotcl_clientData *)ckalloc (sizeof (mongotcl_clientData));

    mongoData->conn = ckalloc(sizeof(mongodata));
    mongoData->interp = interp;

    mongo_init (mongoData->conn);

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
    mongoData->cmdToken = Tcl_CreateObjCommand (interp, commandName, mongotcl_mongoObjectObjCmd, mongoData, mongotcl_mongoObjectDelete);
    Tcl_SetObjResult (interp, Tcl_NewStringObj (commandName, -1));
    if (autoGeneratedName == 1) {
        ckfree(commandName);
    }
    return TCL_OK;
}

