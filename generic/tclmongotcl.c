/*
 * mongo_Init and mongo_SafeInit
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
 * Mongo_Init --
 *
 *	Initialize the mongotcl extension.  The string "mongo" 
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
Mongo_Init(Tcl_Interp *interp)
{
    Tcl_Namespace *namespace;
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

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    namespace = Tcl_CreateNamespace (interp, "::mongo", NULL, NULL);

    /* Create the bson command  */
    Tcl_CreateObjCommand(interp, "::mongo::bson", (Tcl_ObjCmdProc *) mongotcl_bsonObjCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);


    /* Create the mongo command  */
    Tcl_CreateObjCommand(interp, "::mongo::mongo", (Tcl_ObjCmdProc *) mongotcl_mongoObjCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    Tcl_Export (interp, namespace, "*", 0);

    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * mongo_SafeInit --
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
Mongo_SafeInit(Tcl_Interp *interp)
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

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Create the mongo command  */
    Tcl_CreateObjCommand(interp, "mongo", (Tcl_ObjCmdProc *) mongotcl_mongoObjCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}

