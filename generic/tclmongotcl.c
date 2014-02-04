/*
 * mongotcl_Init and mongotcl_SafeInit
 *
 * Copyright (C) 2010 FlightAware
 *
 * Freely redistributable under the Berkeley copyright.  See license.terms
 * for details.
 */

#include <tcl.h>
#include "mongotcl.h"

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT


/*
 *----------------------------------------------------------------------
 *
 * Mongotcl_Init --
 *
 *	Initialize the mongotcl extension.  The string "mongotcl" 
 *      in the function name must match the PACKAGE declaration at the top of
 *	configure.in.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	One new command "mongo" is added to the Tcl interpreter.
 *
 *----------------------------------------------------------------------
 */

EXTERN int
Mongotcl_Init(Tcl_Interp *interp)
{
    /*
     * This may work with 8.0, but we are using strictly stubs here,
     * which requires 8.1.
     */
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgRequire(interp, "Tcl", "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgProvide(interp, "mongotcl", PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Create the bson command  */
    Tcl_CreateObjCommand(interp, "bson", (Tcl_ObjCmdProc *) mongotcl_bsonObjCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);


    /* Create the mongo command  */
    Tcl_CreateObjCommand(interp, "mongo", (Tcl_ObjCmdProc *) mongotcl_mongoObjCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongotcl_SafeInit --
 *
 *	Initialize the mongotcl in a safe interpreter.
 *
 * Results:
 *	A standard Tcl result
 *
 * Side effects:
 *	One new command "mongo" is added to the Tcl interpreter.
 *
 *----------------------------------------------------------------------
 */

EXTERN int
Mongotcl_SafeInit(Tcl_Interp *interp)
{
    /*
     * This may work with 8.0, but we are using strictly stubs here,
     * which requires 8.1.
     */
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgRequire(interp, "Tcl", "8.1", 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgProvide(interp, "mongotcl", PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Create the mongo command  */
    Tcl_CreateObjCommand(interp, "mongo", (Tcl_ObjCmdProc *) mongotcl_mongoObjCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}

