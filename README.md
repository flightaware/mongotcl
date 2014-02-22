MongoTcl, a Tcl C extension providing a Tcl interface to the MongoDB database
===

MongoTcl provides a Tcl interface to the MongoDB C API.

License
---

Open source under the permissive Berkeley copyright, see file LICENSE

Requirements
---
Requires the MongoDB C driver to be installed.  Currently builds against version 0.8.1.

Building
---

    autoconf
    configure
    make
    sudo make install

For FreeBSD, something like

    ./configure --with-tcl=/usr/local/lib/tcl8.5  --mandir=/usr/local/man --enable-symbols
 
Accessing from Tcl
---

    package require mongo


MongoTcl objects
---

MongoTcl provides three object creation commands...

* ::mongo::mongo, to access MongoDB databases, query and update them
* ::mongo::bson, to create and manipulate bson objects
* $mongo cursor, to create a cursor object from a MongoDB object

BSON object
---

BSON stands for Binary JSON, is a binary-encoded serialization of JSON-like documents.  It has a JSON-like structure but is extended to support data types beyond the JSON spec, like it has several binary data types.

BSON is intended to be lightweight, traversable and efficient and it's the primary data representation for MongoDB.

More about bson at http://bsonspec.org/

More on MongoDB's use of BSON at http://docs.mongodb.org/meta-driver/latest/legacy/bson/

MongoTcl has a bson object and the bson creator is invoked to create bson objects, simiarly to iTcl objects

```tcl
    ::mongo::bson create name
```

or

```tcl
    set obj [::mongo::bson create #auto]
```

Methods of the BSON object
---

* $bson init

Initialize or reinitialize the bson object.  It's initialized upon creation.

* $bson string $key $value

Append a key and value to the bson object.

* $bson int $key $value

Append a key and value to the bson object where the value is an integer.

* $bson long $key $value

Append a key and value to the bson object where the value is a long.

* $bson double $key $value

Append a key and value to the bson object where the value is a double-precision floating point value.

* $bson bool $key $value

Append a key and a boolean value to the bson object.

* $bson date $key $epoch

Append a key and epoch to the bson object.  Stored in milliseconds but program multiplies by 1000.  Probably shouldn't and you should use clock clicks -milliseconds for current time.

* $bson null $key

Append a key and a null.

* $bson undefined $key

Append a key and an undefined.

* $bson array_set $list ?typeArray?

Import a list of key-value pairs.  Values are encoded as strings by default.

If the typeArray is specified then for each field, see if the field can
be found in the type array.  If it is found, the corresponding array element
specifies the bson data type to be encoded, from the following list:

* string
* int
* long
* double
* bool
* date
* null
* undefined
* binary_generic
* binary_function
* binary_uuid
* binary_md5
* binary_user_defined
* bson

Example usage

```tcl
	$bson array_set [array get row] typeArray
```

* $bson binary type key $binaryData

Append a key and binary data.  Type can be ''generic'', ''function'', ''uuid'', ''md5'', ''user_defined''.

Keys and values being appended can be expressed in a single statement.

* $bson bson key bsonObject

Append a key and contents of a bson object to the bson object.

* $bson start_array

Begin an array.

Note that the docs say that it's still key value but the keys need to be 0, 1, 2, etc, so for now you have to roll your own although this could easily be coded as a proc or C method.

* $bson finish_array

* $bson new_oid $field

* $bson start_object

Start a subobject.

* $bson end_object

End a subobject.

* $bson finish

Finish the bson object.  I guess this rounds it out and completes it.

* $bson new_oid $key

Append a key and a bson-library-generated oid to the bson object.

* $bson to_list

Enumerate bson object as a list.

* $bson to_array arrayName ?typeArrayName?

Enumerate bson object as an array of key-value pairs.  Embedded bson arrays and objects are set to contain subordinate bson in list format.

if typeArrayName is specified, for each key of the key-value pairs, an element is inserted into typeArray for the same key with the value being the name of the bson datatype such as int, double, string, oid, etc.

* $bson delete

Delete the bson object.

* $bson print

Print is for debugging only, it sort of shows you what's in the bson object.

Most of the methods can be combined in a single command, for example:

```tcl
$bson init string "name" "Joe" int "age" 33 finish
```

Example usage

* Storing binary data read from a file into a bson object:

```tcl
	set bson [::mongo::bson #auto]
	$bson init
    $bson array_set [array get arrayName]
	set fp [open $file]
	fconfigure $fp -translation binary -encoding binary
	$bson generic png_data [read $fp]
	close $fp
	$bson finish
```

The ''to_array'' approach doesn't provide all of the capabilities for composing bson on its own, but it is easy to use while providing high performance.

Methods of mongo, the MongoDB database interface object
---

```tcl
    set mongo [::mongo::mongo create #auto]
```

* $mongo init

Initialize or reinitialize the mongo object.  Like bson, it's initialize upon creation.

* $mongo insert $namespace $bson

Insert the specified bson object in the database with the specified namespace.

* $mongo update $namespace $condBson $opBson ?updateType?

Update the specified bson object.  condBson is the update query in bson.  opBson is the bson update data.  The update type can be ''basic'', ''multi'', ''upsert''.  ''basic'' is used if update type isn't specified.

* $mongo insert_batch $namespace $bsonObjectList

Theoretically this will insert a list of bson objects in the specified namespace (a namespace like '''tutorial.persons''') and have higher performance than calling it one row at a time.

* $mongo remove $namespace $bson

Removes a document from a MongoDB server.  bson is the bson query.

* $mongo cursor name namespace

Create a cursor for this MongoDB connection.  Name is the name of the object created.  If name is #auto, a unique name will be automatically generated and returned.

Cursor methods can then be invoked to move through the query results (or all rows, whatever).  

It is expected that people will mainly use the ''search'' composite method defined in ''mongo.tcl'' and documented below.

* $mongo find $namespace $bsonQuery $bsonFields $limit $skip $options

* $mongo count $db $collection

Return a count of object in the collection.

* $mongo last_error $db

Return the last error.

* $mongo prev_error $db

Return the previous error.

* $mongo write_concern concern_option ?concern_option?

The write_concern method takes one or more options.  ''ignore_errors'' says to ignore errors. Webpages at http://docs.mongodb.org/manual/core/write-concern/ warn not to use the write concern that ignores errors in normal operation.

''unacknowledged'', the default, says to write unacknowledged, while ''acknowledged'' says to write acknowledged.  Acknowledged causes mongod to confirm the receipt of the write operation.  This write concern allows clients to catch errors such as network, duplicate key, and others.

In addition to the above options, which are one-of-three, 
''journaled'' requires the data to have been committed to the journal before returning.
''replica_acknowledged'' requires the write to have propagated to the members of a replica set before returning.

* $mongo create_index $namespace $keyBson $outBson ?optionList?

Create an index.  This can easily done from some CLI that comes with MongoDB, anyway.

The option list contains zero or more instances of the following keywords:
    * unique - reject documents that contain a duplicate value for the indexed field.
    * drop_dups - indexing will fail if you ask for a unique index on a field that already has duplicate values.  This option will index the first occurrence of a value for the key and delete all subsequent values.
    * sparse - make the index only contain entries for documents that contain the indexed field, i.e. if the field is null for a document then the document doesn't appear in the index if this option is specified.
    * background - During index creation the database holding the collection is unavailable for read or write operations by default.  If the background option is set then indexing will run in the background, allowing other database operations to run while the index is being created.

* $mongo set_op_timeout $ms

Set operation timeout in milliseconds.

* $mongo client $address $port

Define a connection to an address and port.  A later C API (yet to be seen on FreeBSD ports and not configuring cleanly natively) supports a URL-type structure.

* $mongo reconnect

Reconnect to the database.

* $mongo disconnect

Disconnect from the database.

* $mongo check_connection

Check the database connection status.  Returns 0 or 1.

* $mongo is_master ?bson?

Return 1 if we are connected to the master, 0 otherwise.  bson, if present, is the name of a bson object that will receive detailed information about the database.

* $mongo replica_set_init

* $mongo replica_set_add_seed $address $port

* $mongo replica_set_client

* $mongo clear_errors

Clear errors.

* $mongo authenticate $db $user $pass

Authenticate to the named database.  MongoDB can be run without authentication.

* $mongo add_user $db $user $pass

Add a user and specify their password.  Again the CLI may be better for this.

* $mongo drop_collection $db $collect

Drop a collection.

* $mongo drop_db $db

Drop a database.

* $mongo delete

Delete the mongo object.  Can also be done by doing a

```tcl
	rename $mongo ""
```

Cursor Methods
---

* $mongo cursor name namespace

Createa a mongo cursor object named ''name'' that will access the requested namespace.

```tcl
	set cursor [$mongo cursor #auto daystream.controlstream]
```

* $cursor init $namespace

Initialize or reinitialize a cursor.

* $cursor next

Move the cursor to the next row.  Returns true if there is a next row, false if the cursor is exhausted.  You have to use ''next'' to get to the first row.

Any error condition (CURSOR_INVALID, CURSOR_PENDING, CURSOR_QUERY_FAIL, CURSOR_BSON_ERROR), it generates a Tcl error and sets the error code to a list consisting of MONGO and the aforementioned condition code.

* $cursor to_list

Return the bson object of the current row as a list of datatypes, keys and usually, values.

* $cursor to_array arrayName ?typeArrayName?

Set an array and possibly typeArray similarly to bson to_array.

* $cursor set_query $bson

Set a cursor's query with a configured bson object.

* $cursor set_skip skipCount

Set the cursor's skip count.

* $cursor set_limit $limit

Set a limit on the number of rows returned.

* $cursor set_options optionList

optonList contains a list zero or more elements:

** tailable

Cursor is tailable.  This keeps the query open and returns new rows as they are added to the namespace.

** slave_ok

Queries are allowed on non-primary nodes.  This probably means the data you get may be a little stale.

** no_timeout

Disable cursor timeouts.

** await_data

Momentarily block for more data.

** exhaust

Streem in multiple 'more' packages... (?)

** partial

Allow reads even if a shard is down.

* $cursor set_fields fieldList

Set what fields are to be returned.  It's useful not to pull fields you don't need, obviously.  fieldList is a list of field names with 1 or 0.  1 says to include the field, 0 says to exclude it.  The fieldList is sticky for future queries.  This may change.  See http://docs.mongodb.org/manual/tutorial/project-fields-from-query-results/ for how the 1/0 thing works.

* $cursor delete

Delete the cursor object.

Search
---

* $mongo search ?-namespace namespace? ?-fields fieldList? ?-array arrayName? ?-typearray typeArrayName? ?-list listVar? ?-offset offset? ?-limit limit? ?-comparebson bson? ?-sort fieldList? ?-code code?

Create a cursor against the specified namespace.  

* If -fields is present, fieldList is a list of fieldNames.  Fields returned are restricted to the named fields.  If a field name starts with a dash it indicates that that field is to be explicitly suppressed. "-_id" might be a fairly common usage if you don't need the oid.

* If -array is present, arrayName is the name of an array set in the caller's context containing elements for the fields of each row returned.

* If -typearray is present it's the name of an array set in the caller's context containing elements for the field names of each row returned with the values being the bson data type.

* If -list is present, the name of a variable that will receive the bson list.

* If -offset is present, the first offset rows of the result are skipped.

* If -limit is present it specifies the maximum number of rows that can be returned.

* If -comparebson is present it specifies a bson object that contains an encoded query to return only rows matching the query specification.

* If -sort is present it contains a list of fields to sort by, from most significant to least significant.  if the first character of the field name is a dash that indicates sorting in reverse order.

* If -code is present, it specifies a code body that is executed for each row returned


Example
---

* create a bson object and insert it into a MongoDB database

```tcl
    $bson init new_oid _id string "name" "Joe" int "age" 33 finish

    $mongo insert "tutorial.persons" $bson
```

When you're done using the bson object, destroy it by doing a

```tcl
    $bson delete
```
or, if you prefer...

```tcl
    rename $bson ""
```

When you're done using the mongodb object, destroy it similarly.

* create an index on a MongoDB namespace

```tcl
	set keyBson [::mongo::bson create #auto]
	$keyBson init int flight_id 1 finish
	set resultBson [::mongo::bson create #auto]
	$mongo create_index daystream.controlstream $keyBson $bsonResult
```

The bson result object can be examined for the status.

To build the same index in the background, append the '''background''' option to the command:

```tcl
	$mongo create_index daystream.controlstream $keyBson $bsonResult background
```

* Perform a complex query

```tcl
	bson create query
	query init start_object \$query int age 24 finish_object start_object \$orderby int name 1 finish_object finish

	$cursor init $namespace
	$cursor set_query $query

	while {[$cursor next]} {
		unset -nocomplain row
		$cursor to_array row
	}
```

Bugs
---

The code is currently early beta quality so there could be quite a few bugs including ones that trigger a coredump.

There are almost for sure some memory leaks so until those are all tracked down expect long-running jobs' memory footprint to grow and plan accordingly.

