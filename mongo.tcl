#
# mongo support functions
#
#
#

package require mongo

namespace eval ::mongo {

#
# _search - search routine called by search method of tcl mongodb objects
#
# m search -namespace tutorial.persons -array row -code {parray row; puts ""}
#
proc _search {args} {
	set obj [lindex $args 0]
	set args [lrange $args 1 end]

	if {[llength $args] & 1} {
		error "list of key-value pairs must have an even number of arguments"
	}

	foreach "key value" $args {
		if {[string index $key 0] != "-"} {
			error "key '$key' of key-value pairs doesn't start with a -"
		}

		switch -exact -- $key {
			"-fields" {
				set fields $value
			}

			"-code" {
				set code $value
			}

			"-array" {
				set arrayName $value
				upvar $arrayName array
			}

			"-typearray" {
				set typeArrayName $value
				upvar $typeArrayName typeArray
			}

			"-list" {
				set listName $value
				upvar $listName listVar
			}

			"-offset" {
				set offset $value
			}

			"-limit" {
				set limit $value
			}

			"-comparebson" {
				set compareBson $value
			}

			"-sort" {
				set sort $value
			}

			"-namespace" {
				set namespace $value
			}

			default {
				error "unknown key $key: must be one of -namespace, -fields, -code, -array, -typearray, -list, -offset, -limit"
			}
		}
	}

	if {![info exists namespace]} {
		error "required field '-namespace' is missing"
	}

	set cursor [$obj cursor #auto $namespace]

	if {[info exists fields]} {
		set fieldList [list]
		foreach field $fields {
			if {[string index $field 0] == "-"} {
				lappend fieldList [string range $field 1 end] 0
			} else {
				lappend fieldList $field 1
			}
		}
		$cursor set_fields $fieldList
	}

	if {[info exists limit]} {
		$cursor set_limit $limit
	}

	if {[info exists offset]} {
		$cursor set_skip $offset
	}

	# generate the query
	set queryBson [::mongo::bson create #auto]

	if {[info exists compareBson]} {
		$queryBson start_object {$query} bson $compareBson finish_object

	}

	# if there's a sort option, append it to the query
	if {[info exists sort]} {
		$queryBson start_object {$orderby}
		foreach field $sort {
			if {[string index $field 0] == "-"} {
				$queryBson int [string range $field 1 end] 0
			} else {
				$queryBson int $field 1
			}
		}
		$queryBson finish_object
	}

	$cursor set_query $queryBson

	#
	# iterate over the matching rows. 
	# you have to invoke "next" to get the first row, by the way
	#
	while {[$cursor next]} {
		# pull the data out of the row, get types too if a type array
		# is specified
		if {[info exists arrayName]} {
			if {![info exists typeArrayName]} {
				unset -nocomplain array
				$cursor to_array array
			} else {
				unset -nocomplain array typeArray
				$cursor to_array array typeArray
			}
		}

		# if they specified -list, give them their list of type triplets
		if {[info exists listName]} {
			set listVar [$cursor to_list]
		}

		# if they specified a code block, execute it
		if {[info exists code]} {
			uplevel $code
		}
	}

	$cursor destroy
}

} ;# namespace ::mongo

# vim: set ts=4 sw=4 sts=4 noet :
