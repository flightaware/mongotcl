

package require mongo

::mongo::mongo create m

m client 127.0.0.1 27017

::mongo::bson create b

b init
b string "name" "Joe"
b int "age" 33
b finish

b print

m insert "tutorial.persons" b

puts "hi"


