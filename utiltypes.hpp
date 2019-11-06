#ifndef UTILTYPES_HPP
#define UTILTYPES_HPP

class Object;

typedef pair<int, int> GCRecord_t;
//      - first is timestamp, second is bytes

typedef std::set< Object * > ObjectSet_t;
typedef unsigned int ObjectId_t;

typedef std::set< ObjectId_t > ObjectIdSet_t;

#endif
