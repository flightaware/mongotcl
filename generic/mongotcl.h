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

#include <mongo.h>

// MONGO_HAVE_STDINT, MONGO_HAVE_UNISTD, MONGO_USE__INT64, or MONGO_USE_LONG_LONG_INT.

extern int
mongotcl_mongoObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objvp[]);

typedef struct mongotcl_clientData
{
    Tcl_Interp *interp;
    mongo *conn;
    Tcl_Command cmdToken;
} mongotcl_clientData;

