

#include "mongotcl.h"
#include <assert.h>

static void
append_list_type_object (Tcl_Interp *interp, Tcl_Obj *listObj, char *type, const char *key, Tcl_Obj *object) {
    Tcl_ListObjAppendElement (interp, listObj, Tcl_NewStringObj (type, -1));
    Tcl_ListObjAppendElement (interp, listObj, Tcl_NewStringObj (key, -1));
    Tcl_ListObjAppendElement (interp, listObj, object);
}

Tcl_Obj *
mongotcl_bsontolist_raw (Tcl_Interp *interp, Tcl_Obj *listObj, const char *data , int depth) {
    bson_iterator i;
    const char *key;
    bson_timestamp_t ts;
    char oidhex[25];
    bson scope;

	if (data == NULL) {
		return listObj;
	}

    bson_iterator_from_buffer(&i, data);

    while (bson_iterator_next (&i)) {
        bson_type t = bson_iterator_type (&i);
        if (t == 0) {
            break;
		}

        key = bson_iterator_key (&i);

		switch (t) {
			case BSON_DOUBLE: {
				append_list_type_object (interp, listObj, "double", key, Tcl_NewDoubleObj (bson_iterator_double (&i)));
				break;
			}

			case BSON_STRING: {
				append_list_type_object (interp, listObj, "string", key, Tcl_NewStringObj (bson_iterator_string (&i), -1));
				break;
			}

			case BSON_SYMBOL: {
				append_list_type_object (interp, listObj, "symbol", key, Tcl_NewStringObj (bson_iterator_string (&i), -1));
				break;
			}

			case BSON_OID: {
				bson_oid_to_string( bson_iterator_oid( &i ), oidhex );
				append_list_type_object (interp, listObj, "oid", key, Tcl_NewStringObj (oidhex, -1));
				break;
			}

			case BSON_BOOL: {
			append_list_type_object (interp, listObj, "bool", key, Tcl_NewBooleanObj (bson_iterator_bool (&i)));
				break;
			}

			case BSON_DATE: {
				append_list_type_object (interp, listObj, "date", key, Tcl_NewLongObj ((long) bson_iterator_date(&i)));
				break;
			}

			case BSON_BINDATA: {
				unsigned char *bindata = (unsigned char *)bson_iterator_bin_data (&i);
				int binlen = bson_iterator_bin_len (&i);

				append_list_type_object (interp, listObj, "bin", key, Tcl_NewByteArrayObj (bindata, binlen));
				break;
			}

			case BSON_UNDEFINED: {
				append_list_type_object (interp, listObj, "undefined", key, Tcl_NewObj ());
				break;
			}

			case BSON_NULL: {
				append_list_type_object (interp, listObj, "null", key, Tcl_NewObj ());
				break;
			}

			case BSON_REGEX: {
				append_list_type_object (interp, listObj, "regex", key, Tcl_NewStringObj (bson_iterator_regex (&i), -1));
				break;
			}

			case BSON_CODE: {
				append_list_type_object (interp, listObj, "code", key, Tcl_NewStringObj (bson_iterator_code (&i), -1));
				break;
			}

			case BSON_CODEWSCOPE: {
				bson_printf( "BSON_CODE_W_SCOPE: %s", bson_iterator_code( &i ) );
				/* bson_init( &scope ); */ /* review - stepped on by bson_iterator_code_scope? */
				bson_iterator_code_scope( &i, &scope );
				bson_printf( "\n\t SCOPE: " );
				bson_print( &scope );
				/* bson_destroy( &scope ); */ /* review - causes free error */
				break;
			}

			case BSON_INT: {
				append_list_type_object (interp, listObj, "int", key, Tcl_NewIntObj (bson_iterator_int (&i)));
				break;
			}

			case BSON_LONG: {
				append_list_type_object (interp, listObj, "long", key, Tcl_NewLongObj ((uint64_t)bson_iterator_long (&i)));
				break;
			}

			case BSON_TIMESTAMP: {
				char string[64];

				ts = bson_iterator_timestamp (&i);
				snprintf(string, sizeof(string), "%d:%d", ts.i, ts.t);
				append_list_type_object (interp, listObj, "timestamp", key, Tcl_NewStringObj (bson_iterator_string (&i), -1));
				break;
			}

			case BSON_ARRAY: {
				Tcl_Obj *subList = Tcl_NewObj ();

				subList = mongotcl_bsontolist_raw (interp, subList, bson_iterator_value (&i), depth + 1);
				append_list_type_object (interp, listObj, "array", key, subList);
				break;
			}

			case BSON_OBJECT: {
				Tcl_Obj *subList = Tcl_NewObj ();

				subList = mongotcl_bsontolist_raw (interp, subList, bson_iterator_value (&i), depth + 1);
				append_list_type_object (interp, listObj, "object", key, subList);
				break;
			}

			default: {
				append_list_type_object (interp, listObj, "unknown", key, Tcl_NewIntObj (t));
				break;
			}
		}
    }
    return listObj;
}

Tcl_Obj *
mongotcl_bsontolist(Tcl_Interp *interp, const bson *b) {
    Tcl_Obj *listObj = Tcl_NewObj();
    return mongotcl_bsontolist_raw (interp, listObj, b->data , 0);
}

int
mongotcl_bsontoarray_raw (Tcl_Interp *interp, char *arrayName, char *typeArrayName, const char *data , int depth) {
    bson_iterator i;
    const char *key;
    bson_timestamp_t ts;
    char oidhex[25];
	Tcl_Obj *obj;
	char *type;

	if (data == NULL) {
		return TCL_OK;
	}

    bson_iterator_from_buffer(&i, data);

    while (bson_iterator_next (&i)) {
        bson_type t = bson_iterator_type (&i);
        if (t == 0) {
            break;
		}

        key = bson_iterator_key (&i);

        switch (t) {
			case BSON_DOUBLE: {
				obj = Tcl_NewDoubleObj (bson_iterator_double (&i));
				type = "double";
				break;
		}

			case BSON_SYMBOL: {
				obj = Tcl_NewStringObj (bson_iterator_string (&i), -1);
				type = "symbol";
				break;
			}

			case BSON_STRING: {
				obj = Tcl_NewStringObj (bson_iterator_string (&i), -1);
				type = "string";
				break;
			}

			case BSON_OID: {
				bson_oid_to_string( bson_iterator_oid( &i ), oidhex );
				obj = Tcl_NewStringObj (oidhex, -1);
				type = "oid";
				break;
			}

			case BSON_BOOL: {
				obj = Tcl_NewBooleanObj (bson_iterator_bool (&i));
				type = "bool";
				break;
			}

			case BSON_DATE: {
				obj = Tcl_NewLongObj ((long) bson_iterator_date(&i));
				type = "date";
				break;
			}

			case BSON_BINDATA: {
				unsigned char *bindata = (unsigned char *)bson_iterator_bin_data (&i);
				int binlen = bson_iterator_bin_len (&i);

				obj = Tcl_NewByteArrayObj (bindata, binlen);
				type = "bin";
				break;
			}

			case BSON_UNDEFINED: {
				obj = Tcl_NewObj ();
				type = "undefined";
				break;
			}

			case BSON_NULL: {
				obj = Tcl_NewObj ();
				type = "null";
				break;
			}

			case BSON_REGEX: {
				obj = Tcl_NewStringObj (bson_iterator_regex (&i), -1);
				type = "regex";
				break;
			}

			case BSON_CODE: {
				obj = Tcl_NewStringObj (bson_iterator_code (&i), -1);
				type = "code";
				break;
			}

			case BSON_CODEWSCOPE: {
				// bson_printf( "BSON_CODE_W_SCOPE: %s", bson_iterator_code( &i ) );
				/* bson_init( &scope ); */ /* review - stepped on by bson_iterator_code_scope? */
				// bson_iterator_code_scope( &i, &scope );
				// bson_printf( "\n\t SCOPE: " );
				// bson_print( &scope );
				/* bson_destroy( &scope ); */ /* review - causes free error */
				break;
			}

			case BSON_INT: {
				obj = Tcl_NewIntObj (bson_iterator_int (&i));
				type = "int";
				break;
			}

			case BSON_LONG: {
				obj = Tcl_NewLongObj ((uint64_t)bson_iterator_long (&i));
				type = "long";
				break;
			}

			case BSON_TIMESTAMP: {
				char string[64];

				ts = bson_iterator_timestamp (&i);
				snprintf(string, sizeof(string), "%d:%d", ts.i, ts.t);
				obj = Tcl_NewStringObj (bson_iterator_string (&i), -1);
				type = "timestamp";
				break;
			}

			case BSON_ARRAY: {
				obj = Tcl_NewObj();
				obj = mongotcl_bsontolist_raw (interp, obj, bson_iterator_value (&i), depth + 1);
				type = "array";

				break;
			}

			case BSON_OBJECT: {
				Tcl_Obj *subList = Tcl_NewObj ();

				obj = mongotcl_bsontolist_raw (interp, subList, bson_iterator_value (&i), depth + 1);
				type = "object";
				break;
			}

			default: {
				obj = Tcl_NewIntObj (t);
				type = "unknown";
				break;
			}
		}

		if (Tcl_SetVar2Ex (interp, arrayName, key, obj, TCL_LEAVE_ERR_MSG) == NULL) {
			return TCL_ERROR;
		}

		if (typeArrayName != NULL) {
			if (Tcl_SetVar2Ex (interp, typeArrayName, key, Tcl_NewStringObj (type, -1), TCL_LEAVE_ERR_MSG) == NULL) {
				return TCL_ERROR;
			}
		}
    }
	return TCL_OK; 
}

int
mongotcl_bsontoarray(Tcl_Interp *interp, char *arrayName, char *typeArrayName, const bson *b) {
    return mongotcl_bsontoarray_raw (interp, arrayName, typeArrayName, b->data , 0);
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
		goto lookup_error;
    }

    if (cmdInfo.objClientData == NULL || ((mongotcl_bsonClientData *)cmdInfo.objClientData)->bson_magic != MONGOTCL_BSON_MAGIC) {
	  lookup_error:
		Tcl_AppendResult (interp, "Error: '", Tcl_GetString (commandNameObj), "' is not a bson object", NULL);
		return TCL_ERROR;
    }

    *bson = ((mongotcl_bsonClientData *)cmdInfo.objClientData)->bson;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_cmdNameObjSetBson --
 *
 *    Take a command name and a pointer to a bson structure, find the Tcl 
 *    command info structure, confirm it's bson, destroy the old bson
 *    structure there and point the command to the passed-in bson
 *    structure.
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_cmdNameObjSetBson (Tcl_Interp *interp, Tcl_Obj *commandNameObj, bson *newBson) {
    Tcl_CmdInfo	cmdInfo;
    mongotcl_bsonClientData *bd;

    if (!Tcl_GetCommandInfo (interp, Tcl_GetString(commandNameObj), &cmdInfo)) {
		return TCL_ERROR;
    }

    if (cmdInfo.objClientData == NULL || ((mongotcl_bsonClientData *)cmdInfo.objClientData)->bson_magic != MONGOTCL_BSON_MAGIC) {
		Tcl_AppendResult (interp, "Error: '", Tcl_GetString (commandNameObj), "' is not a bson object", NULL);
		return TCL_ERROR;
    }

    bd = (mongotcl_bsonClientData *)cmdInfo.objClientData;

    bson_destroy(bd->bson);
	bd->bson = newBson;
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_appendBsonFromObject --
 *
 *    Appends a Tcl object to a BSON object using the specified native
 *    BSON datatype.
 *
 * Results:
 *    stuff
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_appendBsonFromObject(Tcl_Interp *interp, bson *bs, bson_type bsonType, enum bson_binary_subtype_t bsonBinarySubtype, CONST char *key, Tcl_Obj *CONST valueObj) {
	switch (bsonType) {
		case BSON_STRING: {
			char *value;

			value = Tcl_GetString (valueObj);

			if (bson_append_string (bs, key, value) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		}

		case BSON_INT: {
			int num;

			if (Tcl_GetIntFromObj (interp, valueObj, &num) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (bson_append_int (bs, key, num) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		  }

		case BSON_LONG: {
			long num;

			if (Tcl_GetLongFromObj (interp, valueObj, &num) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (bson_append_long (bs, key, num) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		  }

		case BSON_DOUBLE: {
			double num;

			if (Tcl_GetDoubleFromObj (interp, valueObj, &num) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (bson_append_double (bs, key, num) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		}

		case BSON_BOOL: {
			int bool;

			if (Tcl_GetBooleanFromObj (interp, valueObj, &bool) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (bson_append_bool (bs, key, bool) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		}

		case BSON_DATE: {
			long clock;
			if (Tcl_GetLongFromObj (interp, valueObj, &clock) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (bson_append_time_t (bs, key, (time_t)clock) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		}

		case BSON_NULL: {
			if (bson_append_null (bs, key) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		}

		case BSON_UNDEFINED: {
			if (bson_append_undefined (bs, key) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		}

		case BSON_BINDATA: {
			unsigned char *binary;
			int binaryLength;

			binary = Tcl_GetByteArrayFromObj (valueObj, &binaryLength);

			if (bson_append_binary (bs, key, bsonBinarySubtype, (char *)binary, binaryLength) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		}

		case BSON_OBJECT: {
			bson *valBson;

			if (mongotcl_cmdNameObjToBson (interp, valueObj, &valBson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			if (bson_append_bson (bs, key, valBson) != BSON_OK) {
				return mongotcl_setBsonError (interp, bs);
			}
			break;
		}

		default: {
			Tcl_SetObjResult (interp, Tcl_NewStringObj ("unknown or unimplement BSON type", -1));
			return TCL_ERROR;
		}
	}
	return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_appendBsonFromObjects --
 *
 *    Given a Tcl interp, pointer to a bson object, a Tcl_Obj
 *    containing a type name, a key string, a Tcl obj
 *    containing a value and an option binary subtype Tcl_Obj,
 *    append the value to the bson object according to the types
 *    specified.
 *
 * Results:
 *    stuff
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_appendBsonFromObjects(Tcl_Interp *interp, bson *bson, Tcl_Obj *CONST bsonTypeObj, CONST char *key, Tcl_Obj *CONST valueObj)
{
	int typeIndex = 0;
	int binaryType = 0;

    static CONST char *data_types[] = {
        "string",
        "int",
		"long",
        "double",
        "bool",
		"date",
		"null",
		"undefined",
		"binary_generic",
		"binary_function",
		"binary_uuid",
		"binary_md5",
		"binary_user_defined",
		"bson",
		NULL
    };

    enum data_types {
		OPT_APPEND_STRING,
		OPT_APPEND_INT,
		OPT_APPEND_LONG,
		OPT_APPEND_DOUBLE,
		OPT_APPEND_BOOL,
		OPT_APPEND_DATE,
		OPT_APPEND_NULL,
		OPT_APPEND_UNDEFINED,
		OPT_APPEND_BINARY_GENERIC,
		OPT_APPEND_BINARY_FUNCTION,
		OPT_APPEND_BINARY_UUID,
		OPT_APPEND_BINARY_MD5,
		OPT_APPEND_BINARY_USER_DEFINED,
		OPT_APPEND_BSON
    };

	if (Tcl_GetIndexFromObj (interp, bsonTypeObj, data_types, "data_type",
		TCL_EXACT, &typeIndex) != TCL_OK) {
		return TCL_ERROR;
	}

	switch ((enum data_types) typeIndex) {
		case OPT_APPEND_STRING: {
			return mongotcl_appendBsonFromObject(interp, bson, BSON_STRING, 0, key, valueObj);
		}

		case OPT_APPEND_INT: {
			return mongotcl_appendBsonFromObject(interp, bson, BSON_INT, 0, key, valueObj);
		  }

		case OPT_APPEND_LONG: {
			return mongotcl_appendBsonFromObject(interp, bson, BSON_LONG, 0, key, valueObj);
		  }

		case OPT_APPEND_DOUBLE: {
			return mongotcl_appendBsonFromObject(interp, bson, BSON_DOUBLE, 0, key, valueObj);
		}

		case OPT_APPEND_BOOL: {
			return mongotcl_appendBsonFromObject(interp, bson, BSON_BOOL, 0, key, valueObj);
		}

		case OPT_APPEND_DATE: {
			return mongotcl_appendBsonFromObject(interp, bson, BSON_DATE, 0, key, valueObj);
		}

		case OPT_APPEND_NULL: {
			return mongotcl_appendBsonFromObject(interp, bson, BSON_NULL, 0, key, NULL);
	  }

		case OPT_APPEND_UNDEFINED: {
			return mongotcl_appendBsonFromObject(interp, bson, BSON_UNDEFINED, 0, key, NULL);
		}

		case OPT_APPEND_BINARY_GENERIC: {
			binaryType = BSON_BIN_BINARY;
		  append_binary:
			return mongotcl_appendBsonFromObject(interp, bson, BSON_BINDATA, binaryType, key, valueObj);
		}

		case OPT_APPEND_BINARY_FUNCTION: {
			binaryType = BSON_BIN_FUNC;
			goto append_binary;
		}

		case OPT_APPEND_BINARY_UUID: {
			binaryType = BSON_BIN_UUID;
			goto append_binary;
		}

		case OPT_APPEND_BINARY_MD5: {
			binaryType = BSON_BIN_MD5;
			goto append_binary;
		}

		case OPT_APPEND_BINARY_USER_DEFINED: {
			binaryType = BSON_BIN_USER;
			goto append_binary;
		}

		case OPT_APPEND_BSON: {
			if (mongotcl_cmdNameObjToBson (interp, valueObj, &bson) == TCL_ERROR) {
				return TCL_ERROR;
			}

			return mongotcl_appendBsonFromObject(interp, bson, BSON_OBJECT, binaryType, key, valueObj);
		}
	}
	return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_arraytobson --
 *
 *    Given a Tcl interp, a Tcl object containing a list of
 *    key-value pairs, the name of an optional array mapping
 *    fields to bson datatypes, and some bson,
 *
 *    append each key-value pair to the bson object as strings unless
 *    the datatype for the field is found in the array name, in which
 *    case use the type that's the value of the element in the type
 *    array.
 *
 * Results:
 *    stuff
 *
 *----------------------------------------------------------------------
 */
int
mongotcl_arraytobson(Tcl_Interp *interp, Tcl_Obj *listObj, char *typeArrayName, bson *mybson) {
	int listObjc;
	int i;
	Tcl_Obj **listObjv;

	if (Tcl_ListObjGetElements (interp, listObj, &listObjc, &listObjv) == TCL_ERROR) {
		return TCL_ERROR;
	}

	if (listObjc & 1) {
		Tcl_SetObjResult (interp, Tcl_NewStringObj ("list must have even number of elements", -1));
		return TCL_ERROR;
	}

	for (i = 0; i < listObjc; i += 2) {
		char *key = Tcl_GetString (listObjv[i]);
		Tcl_Obj *valueObj = listObjv[i+1];
		Tcl_Obj *typeObj;

		// if typeArrayName is null, append element as a string
		if (typeArrayName == NULL) {
		  handle_string_type:
			if (bson_append_string (mybson, key, Tcl_GetString (valueObj)) != BSON_OK) {
				return mongotcl_setBsonError (interp, mybson);
			}
		} else {
			// lookup the key (field name) in the type array, if it's
			// not there, append element as a string
			if ((typeObj = Tcl_GetVar2Ex (interp, typeArrayName, key, 0)) == NULL) {
				goto handle_string_type;
			}

			// it is there, append element according to the type specified
			// in the type array
			if (mongotcl_appendBsonFromObjects(interp, mybson, typeObj, key, valueObj) != TCL_OK) {
				return mongotcl_setBsonError (interp, mybson);
			}
		}
	}

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
        "date",
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
		"to_array",
		"array_set",
		"finish",
		"delete",
		"print",
		NULL
    };

    enum options {
        OPT_INIT,
        OPT_APPEND_STRING,
        OPT_APPEND_INT,
        OPT_APPEND_DOUBLE,
        OPT_APPEND_BOOL,
        OPT_APPEND_DATE,
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
		OPT_TO_ARRAY,
		OPT_ARRAY_SET,
        OPT_FINISH,
		OPT_DELETE,
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
				bson_destroy (bd->bson);
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
				  field_error:
					Tcl_AddErrorInfo(interp, " while processing field '");
					Tcl_AppendObjToErrorInfo (interp, objv[arg-1]);
					Tcl_AddErrorInfo(interp, "'");
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
					goto field_error;
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
					goto field_error;
				}

				if (bson_append_bool (bd->bson, key, bool) != BSON_OK) {
					return mongotcl_setBsonError (interp, bd->bson);
				}
				break;
			}

			case OPT_APPEND_DATE: {
				long clock;
				char *key;

				if (arg + 2 >= objc) {
					Tcl_WrongNumArgs (interp, 1, objv, "clock key epoch");
					return TCL_ERROR;
				}

				key = Tcl_GetString (objv[++arg]);

				if (Tcl_GetLongFromObj (interp, objv[++arg], &clock) == TCL_ERROR) {
					goto field_error;
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

				static CONST char *binaryTypes[] = {
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
					Tcl_WrongNumArgs (interp, 1, objv, "binaryType key binaryData");
					return TCL_ERROR;
				}

				if (Tcl_GetIndexFromObj (interp, objv[++arg], binaryTypes, "binary_type", TCL_EXACT, &suboptIndex) != TCL_OK) {
					return TCL_ERROR;
				}

				key = Tcl_GetString (objv[++arg]);

				binary = Tcl_GetByteArrayFromObj (objv[++arg], &binaryLength);

				switch ((enum binary_types)suboptIndex) {
					case BINARY_TYPE_GENERIC: {
						binaryType = BSON_BIN_BINARY;
						break;
					}

					case BINARY_TYPE_FUNCTION: {
						binaryType = BSON_BIN_FUNC;
						break;
					}

					case BINARY_TYPE_UUID: {
						binaryType = BSON_BIN_UUID;
						break;
					}

					case BINARY_TYPE_MD5: {
						binaryType = BSON_BIN_MD5;
						break;
					}

					case BINARY_TYPE_USER_DEFINED: {
						binaryType = BSON_BIN_USER;
						break;
					}
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
				if (arg + 1 >= objc) {
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

			case OPT_TO_ARRAY: {
				char *arrayName;
				char *typeArrayName;

				if (objc < 3 || objc > 4) {
					Tcl_WrongNumArgs (interp, 1, objv, "to_array arrayName ?typeArrayName?");
					return TCL_ERROR;
				}

				arrayName = Tcl_GetString (objv[2]);

				if (objc == 3) {
					typeArrayName = NULL;
				} else {
					typeArrayName = Tcl_GetString (objv[3]);
				}

				return mongotcl_bsontoarray(interp, arrayName, typeArrayName, bd->bson);
				break;
			}

			case OPT_ARRAY_SET: {
				char *typeArrayName;

				if (objc < 3 || objc > 4) {
					Tcl_WrongNumArgs (interp, 1, objv, "array_set kvList ?typeArrayName?");
					return TCL_ERROR;
				}

				if (objc == 3) {
					typeArrayName = NULL;
				} else {
					typeArrayName = Tcl_GetString (objv[3]);
				}

				return mongotcl_arraytobson(interp, objv[2], typeArrayName, bd->bson);
				break;
			}

			case OPT_FINISH: {
				if (bson_finish (bd->bson) != BSON_OK) {
					return mongotcl_setBsonError (interp, bd->bson);
				}
				break;
			}

			case OPT_DELETE: {
				Tcl_DeleteCommandFromToken (interp, bd->cmdToken);
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
 * mongotcl_create_bson_command --
 *
 *      Internal routine to create a mongo bson object...
 *
 *      call with interp, command name, and a pointer to a bson structure.
 *
 *      if pointer is null, creates and initializes it.
 *
 * Results:
 *      A standard Tcl result.
 *
 *
 *----------------------------------------------------------------------
 */

int
mongotcl_create_bson_command (Tcl_Interp *interp, char *commandName, CONST bson *bsonObj)
{
    int autoGeneratedName = 0;
    // allocate one of our mongo client data objects for Tcl and configure it
    mongotcl_bsonClientData *bd = (mongotcl_bsonClientData *)ckalloc (sizeof (mongotcl_bsonClientData));

    if (bsonObj == NULL) {
		bd->bson = (bson *)ckalloc(sizeof(bson));
		bson_init (bd->bson);
    } else {
		bd->bson = (bson *)bsonObj;
    }
    bd->interp = interp;
    bd->bson_magic = MONGOTCL_BSON_MAGIC;


    // if commandName is #auto, generate a unique name for the object
    if (strcmp (commandName, "#auto") == 0) {
        static unsigned long nextAutoCounter = 0;
        int    baseNameLength;

        baseNameLength = strlen("bson") + snprintf (NULL, 0, "%lu", nextAutoCounter) + 1;
        commandName = ckalloc (baseNameLength);
        snprintf (commandName, baseNameLength, "bson%lu", nextAutoCounter++);
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
 * mongotcl_bsonObjCmd --
 *
 *      Create a bson object...
 *
 *      ::mongo::bson create my_mongo
 *      ::mongo::bson create #auto
 *
 * The created object is invoked to do things with a bson object like
 *   constructing one and enumerating one.
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
    int                 optIndex;
    char               *commandName;

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

    commandName = Tcl_GetString (objv[2]);
    return mongotcl_create_bson_command (interp, commandName, NULL);
}

/* vim: set ts=4 sw=4 sts=4 noet : */
