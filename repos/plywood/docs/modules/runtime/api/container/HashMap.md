<% setClassScope ply::HashMap %>
<% title "HashMap" %>

A template class suitable for creating hash maps and hash sets. Internally, the hash map/set uses [leapfrog probing](https://preshing.com/20160314/leapfrog-probing/). A hash map/set created using `HashMap` is not thread-safe, so if you manipulate it from multiple threads, you must enforce mutual exclusion yourself.

See [HashMap Traits](HashMapTraits).

<% dumpExtractedMembers ply::HashMap %>
