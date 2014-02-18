/*
 *
 * Include file for mongotcl package
 *
 * Copyright (C) 2014 by FlightAware, All Rights Reserved
 *
 * Freely redistributable under the Berkeley copyright, see license.terms
 * for details.
 */

#include <tcl.h>

#define MONGO_HAVE_STDINT

#define MONGOTCL_BSON_MAGIC 0xf33df007

#define MONGOTCL_MONGO_MAGIC 0xf33db007

#define MONGOTCL_CURSOR_MAGIC 0xf33dc007

#include <mongo.h>

// MONGO_HAVE_STDINT, MONGO_HAVE_UNISTD, MONGO_USE__INT64, or MONGO_USE_LONG_LONG_INT.

extern int
mongotcl_cmdNameObjToBson (Tcl_Interp *interp, Tcl_Obj *commandNameObj, bson **bson);

extern int
mongotcl_cmdNameObjSetBson (Tcl_Interp *interp, Tcl_Obj *commandNameObj, bson *newBson);

extern Tcl_Obj * 
mongotcl_bsontolist(Tcl_Interp *interp, const bson *b);

extern int
mongotcl_bsontoarray(Tcl_Interp *interp, char *arrayName, char *typeArrayName, const bson *b);

extern int
mongotcl_mongoObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objvp[]);

extern int
mongotcl_bsonObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objvp[]);

extern int
mongotcl_setMongoError (Tcl_Interp *interp, mongo *conn);

extern int
mongotcl_create_bson_command (Tcl_Interp *interp, char *commandName, CONST bson *bsonObj);

extern int
mongotcl_setBsonError (Tcl_Interp *interp, bson *bson);

extern int
mongotcl_createCursorObjCmd(Tcl_Interp *interp, mongo *conn, char *commandName, char *namespace); ;

typedef struct mongotcl_clientData
{
    int mongo_magic;
    Tcl_Interp *interp;
    mongo *conn;
    Tcl_Command cmdToken;
    mongo_write_concern *write_concern;
} mongotcl_clientData;

typedef struct mongotcl_bsonClientData
{
    int bson_magic;
    Tcl_Interp *interp;
    bson *bson;
    Tcl_Command cmdToken;
} mongotcl_bsonClientData;

typedef struct mongotcl_cursorClientData
{
    int cursor_magic;
    mongo *conn;
    Tcl_Interp *interp;
    mongo_cursor *cursor;
    Tcl_Command cmdToken;
	bson *fieldsBson;
} mongotcl_cursorClientData;

/* vim: set ts=4 sw=4 sts=4 noet : */
