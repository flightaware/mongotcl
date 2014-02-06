This is MongoTcl, a Tcl C extension providing an interface to the MongoDB C driver.
===

MongoTcl provides a Tcl interface to the MongoDB C API.

FA Note
---
If pursued far enough, when this is solid enough it will be open sourced.

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

status: constructing bson is solid.  deconstructing it is untested to nonexistent.

BSON stands for Binary JSON, is a binary-encoded serialization of JSON-like documents.  It has a JSON-like structure but is extended to support data types beyond the JSON spec, like it has a binary data type.

It's intended to be lightweight, traversable and efficient and it's the primary data representation for MongoDB.

More about bson at http://bsonspec.org/

MongoDB use of BSON at http://docs.mongodb.org/meta-driver/latest/legacy/bson/

MongoTcl has a bson object and the bson creator is invoked to create bson objects, siilarly to iTcl objects

```tcl
    ::mongo::bson create
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

Append a key and value to the bson object where the value is a number.

* $bson kvlist $list

Import a list of key-value pairs.  Values are encoed as strings.  

List must be ccontain an even number of elements.

Typical usage

```tcl
    $bson kvlist [array get arrayName]
```

* $bson start_array

Begin an array.

Note that the docs say that it's still key value but the keys need to be 0, 1, 2, etc, so for now you have to roll your own although this could easily be coded as a proc or C method.

* $bson finish_array

* $bson new_oid $field

* $bson start_object

* $bson end_object

* $bson finish

* $bson print

Print is for debugging only, it sort of shows you what's in the bson object.



Methods of mongo, the MongoDB interface object
---

```tcl
    set mongo [::mongo::mongo create #auto]
```

* $mongo init

Initialize or reinitialize the mongo object.  Like bson, it's initialize upon creation.

* $mongo insert $namespace $bson

* $mongo update $namespace $condBson $opBson ?updateType?

* $mongo insert_batch $namespace $bsonObjectList

There's a compile warning on this.  It's probably coredump.

* $mongo cursor

* $mongo find $namespace $bsonQuery $bsonFields $limit $skip $options

* $mongo count $db $collection

Return a count of object in the collection.

* $mongo last_error $db

Return the last error.

* $mongo prev_error $db

Return the previous error.

* $mongo create_index $namespace $keyBson $outBson ?optionList?

Create an index.  This can easily done from some CLI that comes with MongoDB, anyway.

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

Example
---

```tcl
    $bson init
    $bson new_oid _id
    $bson string "name" "Joe"
    $bson int "age" 33
    $bson finish

    $mongo insert "tutorial.persons" $bson
```

When you're done using the bson object, destroy it by doing a

    rename $bson ""

