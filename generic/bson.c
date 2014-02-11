

#include "mongotcl.h"
#include <assert.h>

static void
append_list_type_object (Tcl_Interp *interp, Tcl_Obj *listObj, char *type, Tcl_Obj *object) {
    Tcl_ListObjAppendElement (interp, listObj, Tcl_NewStringObj (type, -1));
    Tcl_ListObjAppendElement (interp, listObj, object);
}

Tcl_Obj *
mongotcl_bsontolist_raw (Tcl_Interp *interp, Tcl_Obj *listObj, const char *data , int depth) {
    bson_iterator i;
    const char *key;
    bson_timestamp_t ts;
    char oidhex[25];
    bson scope;
    bson_iterator_from_buffer(&i, data);

    while (bson_iterator_next (&i)) {
        bson_type t = bson_iterator_type (&i);
        if (t == 0)
            break;

        key = bson_iterator_key (&i);
	append_list_type_object (interp, listObj, "key", Tcl_NewStringObj (key, -1));

        switch (t) {
        case BSON_DOUBLE:
	    append_list_type_object (interp, listObj, "double", Tcl_NewDoubleObj (bson_iterator_double (&i)));
            break;

        case BSON_STRING:
            append_list_type_object (interp, listObj, "string", Tcl_NewStringObj (bson_iterator_string (&i), -1));
            break;

        case BSON_SYMBOL:
            append_list_type_object (interp, listObj, "symbol", Tcl_NewStringObj (bson_iterator_string (&i), -1));
            break;

        case BSON_OID:
            bson_oid_to_string( bson_iterator_oid( &i ), oidhex );
            append_list_type_object (interp, listObj, "oid", Tcl_NewStringObj (oidhex, -1));
            break;

        case BSON_BOOL:
	    append_list_type_object (interp, listObj, "bool", Tcl_NewBooleanObj (bson_iterator_bool (&i)));
            break;

        case BSON_DATE:
            append_list_type_object (interp, listObj, "date", Tcl_NewLongObj ((long) bson_iterator_date(&i)));
            break;

        case BSON_BINDATA: {
	    unsigned char *bindata = (unsigned char *)bson_iterator_bin_data (&i);
	    int binlen = bson_iterator_bin_len (&i);

	    append_list_type_object (interp, listObj, "bin", Tcl_NewByteArrayObj (bindata, binlen));
            break;
	}

        case BSON_UNDEFINED:
	    Tcl_ListObjAppendElement (interp, listObj, Tcl_NewStringObj ("undefined", -1));
            break;

        case BSON_NULL:
	    Tcl_ListObjAppendElement (interp, listObj, Tcl_NewStringObj ("null", -1));
            break;

        case BSON_REGEX:
	    append_list_type_object (interp, listObj, "regex", Tcl_NewStringObj (bson_iterator_regex (&i), -1));
            break;

        case BSON_CODE:
	    append_list_type_object (interp, listObj, "code", Tcl_NewStringObj (bson_iterator_code (&i), -1));
            break;

        case BSON_CODEWSCOPE:
            bson_printf( "BSON_CODE_W_SCOPE: %s", bson_iterator_code( &i ) );
            /* bson_init( &scope ); */ /* review - stepped on by bson_iterator_code_scope? */
            bson_iterator_code_scope( &i, &scope );
            bson_printf( "\n\t SCOPE: " );
            bson_print( &scope );
            /* bson_destroy( &scope ); */ /* review - causes free error */
            break;

        case BSON_INT:
	    append_list_type_object (interp, listObj, "int", Tcl_NewIntObj (bson_iterator_int (&i)));
            break;

        case BSON_LONG:
	    append_list_type_object (interp, listObj, "long", Tcl_NewLongObj ((uint64_t)bson_iterator_long (&i)));
            break;

        case BSON_TIMESTAMP: {
	    char string[64];
            ts = bson_iterator_timestamp (&i);
            snprintf(string, sizeof(string), "%d:%d", ts.i, ts.t);
            append_list_type_object (interp, listObj, "timestamp", Tcl_NewStringObj (bson_iterator_string (&i), -1));
            break;
	}

        case BSON_ARRAY: {
	    Tcl_Obj *subList = Tcl_NewObj ();

	    subList = mongotcl_bsontolist_raw (interp, subList, bson_iterator_value (&i), depth + 1);
	    append_list_type_object (interp, listObj, "array", subList);
	    break;
	}

        case BSON_OBJECT: {
	    Tcl_Obj *subList = Tcl_NewObj ();

	    subList = mongotcl_bsontolist_raw (interp, subList, bson_iterator_value (&i), depth + 1);
	    append_list_type_object (interp, listObj, "object", subList);
	    break;
	}

        default:
	    append_list_type_object (interp, listObj, "unknown", Tcl_NewIntObj (t));
            break;
        }
    }
    return listObj;
}

Tcl_Obj *
mongotcl_bsontolist(Tcl_Interp *interp, const bson *b) {
    Tcl_Obj *listObj = Tcl_NewObj();
    return mongotcl_bsontolist_raw (interp, listObj, b->data , 0);
}


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
	Tcl_SetObjResult (interp, Tcl_NewStringObj ("bson already finished", -1));
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
int
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
    int         arg;
    int         optIndex;
    mongotcl_bsonClientData *bd = (mongotcl_bsonClientData *)cData;

    static CONST char *options[] = {
        "init",
        "string",
        "int",
        "double",
        "bool",
        "clock",
	"null",
	"undefined",
	"kvlist",
	"binary",
	"bson",
        "start_array",
        "finish_array",
	"start_object",
	"finish_object",
	"new_oid",
	"to_list",
	"finish",
	"print",
        NULL
    };

    enum options {
        OPT_INIT,
        OPT_APPEND_STRING,
        OPT_APPEND_INT,
        OPT_APPEND_DOUBLE,
        OPT_APPEND_BOOL,
        OPT_APPEND_CLOCK,
        OPT_APPEND_NULL,
        OPT_APPEND_UNDEFINED,
	OPT_APPEND_KVLIST,
	OPT_APPEND_BINARY,
	OPT_APPEND_BSON,
        OPT_APPEND_START_ARRAY,
        OPT_APPEND_FINISH_ARRAY,
        OPT_APPEND_START_OBJECT,
        OPT_APPEND_FINISH_OBJECT,
	OPT_APPEND_NEW_OID,
	OPT_TO_LIST,
        OPT_FINISH,
	OPT_PRINT
    };

    /* basic validation of command line arguments */
    if (objc < 2) {
        Tcl_WrongNumArgs (interp, 1, objv, "subcommand ?args? ?subcommand ?args??...");
        return TCL_ERROR;
    }

    for (arg = 1; arg < objc; arg++) {
	if (Tcl_GetIndexFromObj (interp, objv[arg], options, "option",
	    TCL_EXACT, &optIndex) != TCL_OK) {
	    return TCL_ERROR;
	}

	switch ((enum options) optIndex) {
	  case OPT_INIT: {
	    bson_init (bd->bson);
	    break;
	  }

	  case OPT_APPEND_STRING: {
	    char *key;
	    char *value;

	    if (arg + 2 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "string key value");
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);
	    value = Tcl_GetString (objv[++arg]);

	    if (bson_append_string (bd->bson, key, value) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_INT: {
	    int num;
	    char *key;

	    if (arg + 2 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "int key number");
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);

	    if (Tcl_GetIntFromObj (interp, objv[++arg], &num) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    if (bson_append_int (bd->bson, key, num) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_DOUBLE: {
	    double num;
	    char *key;

	    if (arg + 2 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "int key number");
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);

	    if (Tcl_GetDoubleFromObj (interp, objv[++arg], &num) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    if (bson_append_double (bd->bson, key, num) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_BOOL: {
	    int bool;
	    char *key;

	    if (arg + 2 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "bool key boolVal");
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);

	    if (Tcl_GetBooleanFromObj (interp, objv[++arg], &bool) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    if (bson_append_bool (bd->bson, key, bool) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_CLOCK: {
	    long clock;
	    char *key;

	    if (arg + 2 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "clock key epoch");
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);

	    if (Tcl_GetLongFromObj (interp, objv[++arg], &clock) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    if (bson_append_time_t (bd->bson, key, (time_t)clock) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_NULL: {
	    char *key;

	    if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "null key");
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);

	    if (bson_append_null (bd->bson, key) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_UNDEFINED: {
	    char *key;

	    if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "null key");
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);

	    if (bson_append_undefined (bd->bson, key) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_KVLIST: {
	    int listObjc;
	    int i;
	    Tcl_Obj **listObjv;

	    if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "kvlist list");
		return TCL_ERROR;
	    }

	    if (Tcl_ListObjGetElements (interp, objv[++arg], &listObjc, &listObjv) == TCL_ERROR) {
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

	  case OPT_APPEND_BINARY: {
	    char *key;
	    unsigned char *binary;
	    int binaryLength;
	    int   suboptIndex;
	    int binaryType;

	    static CONST char *subTypes[] = {
		"generic",
		"function",
		"uuid",
		"md5",
		"user_defined",
		NULL
	    };

	    enum binary_types {
		BINARY_TYPE_GENERIC,
		BINARY_TYPE_FUNCTION,
		BINARY_TYPE_UUID,
		BINARY_TYPE_MD5,
		BINARY_TYPE_USER_DEFINED
	    };


	    if (arg + 3 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "binary key type value");
		return TCL_ERROR;
	    }

	    if (Tcl_GetIndexFromObj (interp, objv[++arg], subTypes, "subtype", TCL_EXACT, &suboptIndex) != TCL_OK) {
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);

	    binary = Tcl_GetByteArrayFromObj (objv[++arg], &binaryLength);

	    switch ((enum binary_types)suboptIndex) {
		case BINARY_TYPE_GENERIC:
		    binaryType = BSON_BIN_BINARY;
		    break;

		case BINARY_TYPE_FUNCTION:
		    binaryType = BSON_BIN_FUNC;
		    break;

		case BINARY_TYPE_UUID:
		    binaryType = BSON_BIN_UUID;
		    break;

		case BINARY_TYPE_MD5:
		    binaryType = BSON_BIN_MD5;

		    break;
		case BINARY_TYPE_USER_DEFINED:
		    binaryType = BSON_BIN_USER;
		    break;
	    }

	    if (bson_append_binary (bd->bson, key, binaryType, (char *)binary, binaryLength) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_BSON: {
	    char *key;
	    bson *bson = NULL;


	    if (arg + 2 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "bson key bson");
		return TCL_ERROR;
	    }

	    key = Tcl_GetString (objv[++arg]);

	    if (mongotcl_cmdNameObjToBson (interp, objv[++arg], &bson) == TCL_ERROR) {
		return TCL_ERROR;
	    }

	    if (bson_append_bson (bd->bson, key, bson) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_START_ARRAY: {
	    if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "start_array name");
		return TCL_ERROR;
	    }

	    if (bson_append_start_array (bd->bson, Tcl_GetString (objv[++arg])) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_FINISH_ARRAY: {
	    if (bson_append_finish_array (bd->bson) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_START_OBJECT: {
	    if (arg + 1 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "name");
		return TCL_ERROR;
	    }

	    if (bson_append_start_object (bd->bson, Tcl_GetString (objv[++arg])) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_FINISH_OBJECT: {
	    if (bson_append_finish_object (bd->bson) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_APPEND_NEW_OID: {
	    if (arg + 2 >= objc) {
		Tcl_WrongNumArgs (interp, 1, objv, "new_oid name");
		return TCL_ERROR;
	    }

	    if (bson_append_new_oid (bd->bson, Tcl_GetString (objv[++arg])) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_TO_LIST: {
	    Tcl_SetObjResult (interp, mongotcl_bsontolist(interp, bd->bson));
	    break;
	  }

	  case OPT_FINISH: {
	    if (bson_finish (bd->bson) != BSON_OK) {
		return mongotcl_setBsonError (interp, bd->bson);
	    }
	    break;
	  }

	  case OPT_PRINT: {
	    bson_print (bd->bson);
	    break;
	  }
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
    bd->bson_magic = MONGOTCL_BSON_MAGIC;

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

