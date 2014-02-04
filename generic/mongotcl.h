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
#include <mongo.h>

extern int
mongotcl_mongoObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objvp[]);

typedef struct mongotcl_clientData
{
    Tcl_Interp *interp;
    mongo conn;
    Tcl_Command cmdToken;
} mongotcl_clientData;

