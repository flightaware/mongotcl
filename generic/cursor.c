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
		"next",
		"to_list",
        "init",
        "set_query",
        "set_fields",
        "set_skip",
        "set_limit",
		"set_options",
		"data",
        NULL
    };

    enum options {
        OPT_CURSOR_NEXT,
		OPT_CURSOR_TO_LIST,
        OPT_CURSOR_INIT,
        OPT_CURSOR_SET_QUERY,
        OPT_CURSOR_SET_FIELDS,
        OPT_CURSOR_SET_SKIP,
        OPT_CURSOR_SET_LIMIT,
        OPT_CURSOR_SET_OPTIONS,
        OPT_CURSOR_DATA
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

			if (mongotcl_cmdNameObjToBson (interp, objv[3], &bson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			mongo_cursor_set_query (mc->cursor, bson);

			break;
		}

		case OPT_CURSOR_SET_FIELDS: {
			bson *bson;

			if (objc != 3) {
				Tcl_WrongNumArgs (interp, 2, objv, "bson");
				return TCL_ERROR;
			}

			if (mongotcl_cmdNameObjToBson (interp, objv[3], &bson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			mongo_cursor_set_fields (mc->cursor, bson);

			break;
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
			int cursorFlags;
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

		case OPT_CURSOR_NEXT: {
			if (mongo_cursor_next (mc->cursor) != MONGO_OK) {
				return mongotcl_setMongoError (interp, mc->conn);
			}
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
