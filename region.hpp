#ifndef REGION_H
#define REGION_H

#include <set>

#include "heap.hpp"

class Object;

//
// A map:
// key: edge id
//     -> val: edge id set
typedef std::map< ObjectId_t, ObjectIdSet_t > ObjectId2SetMap_t;

class Region
{
public:
    // Debug flag 
    static bool debug;

    // Constructor
    Region( string &name,
            int level )
        : m_name(name)
        , m_live(0)
        , m_garbage(0)
        , m_level(level)
        , m_live_set()
        // TODO , m_region_edges()
        // TODO , m_in_edges()
        // TODO , m_out_edges()
        , m_dtime(0)
        , m_garbage_waiting()
        , m_gc_history() {
    }

    ~Region() {
    }

    // Returns true if there was space and thus successful.
    //         false otherwise.
    bool allocate( Object *object,
                   unsigned int create_time );
    // The following three functions:
    // return true if allocation was successful.
    //        false otherwise.
    // bool remove( Object *object );
    bool makeDead( Object *object );
    void add_to_garbage_set( Object *object );

    int collect( unsigned int timestamp, unsigned int timestamp_alloc );

    //----------------------------------------------------------------------
    int getLevel() const  { return this->m_level; }

    int getLive() const { return m_live; }
    int getGarbage() const { return m_garbage; }
    unsigned int long get_num_GC_attempts() const {
        return this->GC_attempts;
    }

    void set_region_deathtime( unsigned int dtime ) {
        this->m_dtime = dtime;
    }

    unsigned int get_region_deathtime() {
        return this->m_dtime;
    }

    deque<GCRecord_t> get_GC_history() const { return this->m_gc_history; }

    // Get region name
    string get_name() const { return this->m_name; }

    // Debug functions
    void print_status();

protected:
    // TODO // Edge sets and remember sets
    // TODO //     * edges where source and target are in the region
    // TODO ObjectId2SetMap_t m_region_edges;
    // TODO //     * edges where source is outside and target is in the region
    // TODO ObjectId2SetMap_t m_in_edges;
    // TODO //     * edges where source is inside and target is outside the region
    // TODO ObjectId2SetMap_t m_out_edges;

    // Expected death time in allocation byte time
    unsigned int m_dtime;

private:
    string m_name;

    void addToGarbage( int add );
    int setGarbage( int newval );

    // The following fields are in bytes.
    int m_live; // live space (reachable, not garbage)
    int m_garbage; // garbage

    int m_number_of_collections;

    int m_level;
    // Signifies the level in the hierarchy of regional generations.
    // Level 0 - where the memory manager allocates from
    // Level 1 - promotions from Level 0 go here.
    // ...
    // Level n - promotions from Level n-1 go here.
    ObjectSet_t m_garbage_waiting; // TODO What is m_garbage_waiting?
    ObjectSet_t m_live_set;

    // Collection history
    deque<GCRecord_t> m_gc_history;
    unsigned long int GC_attempts;

};


#endif
