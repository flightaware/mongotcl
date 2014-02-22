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

	if (mc->fieldsBson != NULL) {
		bson_destroy (mc->fieldsBson);
		ckfree ((char *)mc->fieldsBson);
	}

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
int
mongotcl_cmdNameObjToCursor (Tcl_Interp *interp, Tcl_Obj *commandNameObj, mongo_cursor **cursor) {
    Tcl_CmdInfo	cmdInfo;

    if (!Tcl_GetCommandInfo (interp, Tcl_GetString(commandNameObj), &cmdInfo)) {
		goto lookup_error;
    }

    if (cmdInfo.objClientData == NULL || ((mongotcl_cursorClientData *)cmdInfo.objClientData)->cursor_magic != MONGOTCL_CURSOR_MAGIC) {
	  lookup_error:
		Tcl_AppendResult (interp, "Error: '", Tcl_GetString (commandNameObj), "' is not a mongo cursor object", NULL);
		return TCL_ERROR;
    }

    *cursor = ((mongotcl_cursorClientData *)cmdInfo.objClientData)->cursor;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_tcllist_to_cursor_fields --
 *
 *      Takes a Tcl list that should contain pairs of field names and
 *      0/1 values and a mongotcl cursor clientdata structure.
 *
 *      If successful, sets a bson object in the cursor client data to
 *      contain the equivalent, appropriate bson for passing to 
 *      mongo_cursor_set_fields
 *
 *      If unsuccessful, returns TCL_ERROR and sets the bson pointer
 *      to NULL.
 *
 * Results:
 *      A standard Tcl result.
 *
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_tcllist_to_cursor_fields (Tcl_Interp *interp, Tcl_Obj *fieldList, mongotcl_cursorClientData *mc) {
	Tcl_Obj **listObjv;
	int listObjc;
	int i;

	if (Tcl_ListObjGetElements (interp, fieldList, &listObjc, &listObjv) == TCL_ERROR) {
		Tcl_AddErrorInfo (interp, "while reading field list");
		return TCL_ERROR;
	}

	if (listObjc & 1) {
		Tcl_SetObjResult (interp, Tcl_NewStringObj ("field list must have even number of elements", -1));
		return TCL_ERROR;
	}

	if (mc->fieldsBson == NULL) {
		mc->fieldsBson = (bson *)ckalloc(sizeof(bson));
	}
	bson_init(mc->fieldsBson);

	for (i = 0; i < listObjc; i += 2) {
		int want;
		char *key = Tcl_GetString (listObjv[i]);

		if (Tcl_GetIntFromObj (interp, listObjv[i+1], &want) == TCL_ERROR) {
		  bson_error:
			return mongotcl_setBsonError (interp, mc->fieldsBson);
		}

		if (bson_append_int (mc->fieldsBson, key, want) != BSON_OK) {
			goto bson_error;
		}
	}

	if (bson_finish (mc->fieldsBson) != BSON_OK) {
		goto bson_error;
	}

	mongo_cursor_set_fields (mc->cursor, mc->fieldsBson);

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
		"next",
		"to_list",
		"to_array",
        "init",
        "set_query",
        "set_fields",
        "set_skip",
        "set_limit",
		"set_options",
		"data",
		"delete",
        NULL
    };

    enum options {
        OPT_CURSOR_NEXT,
		OPT_CURSOR_TO_LIST,
		OPT_CURSOR_TO_ARRAY,
        OPT_CURSOR_INIT,
        OPT_CURSOR_SET_QUERY,
        OPT_CURSOR_SET_FIELDS,
        OPT_CURSOR_SET_SKIP,
        OPT_CURSOR_SET_LIMIT,
        OPT_CURSOR_SET_OPTIONS,
        OPT_CURSOR_DATA,
		OPT_CURSOR_DELETE
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
			bson *bson;

			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "bson");
				return TCL_ERROR;
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[2], &bson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			mongo_cursor_set_query (mc->cursor, bson);

			break;
		}

		case OPT_CURSOR_SET_FIELDS: {
			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "list");
				return TCL_ERROR;
			}

			return mongotcl_tcllist_to_cursor_fields (interp, objv[2], mc);
		}

		case OPT_CURSOR_SET_SKIP: {
			int skip;

			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "skip");
				return TCL_ERROR;
			}
	
			if (Tcl_GetIntFromObj (interp, objv[2], &skip) == TCL_ERROR) {
				return TCL_ERROR;
			}

			mongo_cursor_set_skip (mc->cursor, skip);
			break;
		}

		case OPT_CURSOR_SET_LIMIT: {
			int limit;

			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "limit");
				return TCL_ERROR;
			}
	
			if (Tcl_GetIntFromObj (interp, objv[2], &limit) == TCL_ERROR) {
				return TCL_ERROR;
			}

			mongo_cursor_set_limit (mc->cursor, limit);
			break;
		}

		case OPT_CURSOR_SET_OPTIONS: {
			int listObjc;
			int cursorFlags = 0;
			int i;
			Tcl_Obj **listObjv;

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

			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "optionList");
				return TCL_ERROR;
			}
	

			if (Tcl_ListObjGetElements (interp, objv[2], &listObjc, &listObjv) == TCL_ERROR) {
				Tcl_AddErrorInfo (interp, "while examining option list");
				return TCL_ERROR;
			}

			for (i = 0; i < listObjc; i++) {
				int suboptIndex;

				if (Tcl_GetIndexFromObj (interp, listObjv[i], subOptions, "indexOption", TCL_EXACT, &suboptIndex) != TCL_OK) {
					return TCL_ERROR;
				}

				switch ((enum suboptions)suboptIndex) {
					case SUBOPT_CURSOR_TAILABLE: {
						cursorFlags |= MONGO_TAILABLE;
						break;
					}

					case SUBOPT_CURSOR_SLAVE_OK: {
						cursorFlags |= MONGO_SLAVE_OK;
						break;
					}

					case SUBOPT_CURSOR_NO_TIMEOUT: {
						cursorFlags |= MONGO_NO_CURSOR_TIMEOUT;
						break;
					}

					case SUBOPT_CURSOR_AWAIT_DATA: {
						cursorFlags |= MONGO_AWAIT_DATA;
						break;
					}

					case SUBOPT_CURSOR_EXHAUST: {
						cursorFlags |= MONGO_EXHAUST;
						break;
					}

					case SUBOPT_CURSOR_PARTIAL: {
						cursorFlags |= MONGO_PARTIAL;
						break;
					}
				}

				mongo_cursor_set_options (mc->cursor, cursorFlags);
			}

			break;
		}

		case OPT_CURSOR_DATA: {
			break;
		}

		case OPT_CURSOR_TO_LIST: {
			if (objc != 2) {
				Tcl_WrongNumArgs (interp, 1, objv, "to_list");
				return TCL_ERROR;
			}

			Tcl_SetObjResult (interp, mongotcl_bsontolist (interp, mongo_cursor_bson (mc->cursor)));
			break;
		}

		case OPT_CURSOR_TO_ARRAY: {
			char *arrayName;
			char *typeArrayName;

			if (objc < 3 || objc > 4) {
				Tcl_WrongNumArgs (interp, 1, objv, "to_array array ?typeArray?");
				return TCL_ERROR;
			}

			arrayName = Tcl_GetString (objv[2]);

			if (objc == 3) {
				typeArrayName = NULL;
			} else {
				typeArrayName = Tcl_GetString (objv[3]);
			}

			return mongotcl_bsontoarray (interp, arrayName, typeArrayName, mongo_cursor_bson (mc->cursor));
		}

		case OPT_CURSOR_NEXT: {
			if (mongo_cursor_next (mc->cursor) == MONGO_OK) {
				Tcl_SetObjResult (interp, Tcl_NewBooleanObj (1));
			} else {
				if (mc->cursor->err == MONGO_CURSOR_EXHAUSTED) {
					Tcl_SetObjResult (interp, Tcl_NewBooleanObj (0));
				} else {
					return mongotcl_setCursorError (interp, mc->cursor);
				}
			}
			break;
		}

		case OPT_CURSOR_DELETE: {
			Tcl_DeleteCommandFromToken (interp, mc->cmdToken);
			break;
		}
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
 *      Give it an interp, mongo connection, command name to be
 *      created, and a MongoDB namespace to be a cursor for
 *
 *		If successful, creates a new Tcl command.
 *
 * Results:
 *      A standard Tcl result.
 *
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
int
mongotcl_createCursorObjCmd(Tcl_Interp *interp, mongo *conn, char *commandName, char *namespace)
{
    mongotcl_cursorClientData *mc;
    int                 autoGeneratedName;

    // allocate one of our mongo client data objects for Tcl and configure it
    mc = (mongotcl_cursorClientData *)ckalloc (sizeof (mongotcl_cursorClientData));

    mc->interp = interp;
    mc->conn = conn;
    mc->cursor = (mongo_cursor *)ckalloc(sizeof(mongo_cursor));
	mc->cursor_magic = MONGOTCL_CURSOR_MAGIC;
	mc->fieldsBson = NULL;

	mongo_cursor_init (mc->cursor, conn, namespace);

    // if commandName is #auto, generate a unique name for the object
    autoGeneratedName = 0;
    if (strcmp (commandName, "#auto") == 0) {
        static unsigned long nextAutoCounter = 0;
        int    baseNameLength;

        baseNameLength = strlen("cursor") + snprintf (NULL, 0, "%lu", nextAutoCounter) + 1;
        commandName = ckalloc (baseNameLength);
        snprintf (commandName, baseNameLength, "cursor%lu", nextAutoCounter++);
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


/* vim: set ts=4 sw=4 sts=4 noet : */
