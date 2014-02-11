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

#define MONGOTCL_BSON_MAGIC 0xf33df00d

#define MONGOTCL_MONGO_MAGIC 0xf33dd00d

#define MONGOTCL_CURSOR_MAGIC 0xf33dc00c

#include <mongo.h>

// MONGO_HAVE_STDINT, MONGO_HAVE_UNISTD, MONGO_USE__INT64, or MONGO_USE_LONG_LONG_INT.

extern int
mongotcl_cmdNameObjToBson (Tcl_Interp *interp, Tcl_Obj *commandNameObj, bson **bson);

extern int
mongotcl_mongoObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objvp[]);

extern int
mongotcl_bsonObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objvp[]);

extern int
mongotcl_setMongoError (Tcl_Interp *interp, mongo *conn);

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
} mongotcl_cursorClientData;

