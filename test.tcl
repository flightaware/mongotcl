

package require mongo

::mongo::mongo create m

m client 127.0.0.1 27017

::mongo::bson create b

#b init string "name" "Joe" int "age" 33 finish

#b print

#m insert "tutorial.persons" b

#puts "hi"

# vim: set ts=4 sw=4 sts=4 noet :
