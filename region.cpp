#include "region.hpp"

// -- Global flags
bool Region::debug = false;

// TODO using namespace boost;

//---------------------------------------------------------------------------------
//===========[ Region ]============================================================

// Returns true if allocation was successful.
//         false otherwise.
bool Region::allocate( Object *object,
                       unsigned int create_time )
{
    // Check to see if there's space
    unsigned int objSize = object->getSize();
    // Duplicate allocates are a problem.
    // We do this check in the MemoryMgr as the object MAY have
    // been allocated in a different region.
    this->m_live_set.insert( object );

    return true;
    // TODO: do i need create_time? And if yes, how do I save it?
}

// TODO bool Region::remove( Object *object )
// TODO {
// TODO     ObjectSet_t::iterator iter = this->m_live_set.find(object);
// TODO     if (iter == this->m_live_set.end()) {
// TODO         return false;
// TODO     }
// TODO     unsigned int objSize = object->getSize();
// TODO     this->m_live_set.erase(iter);
// TODO     this->m_free += objSize; // Free goes up.
// TODO     this->m_used -= objSize; // Used goes down.
// TODO     this->m_live -= objSize; // Live goes down.
// TODO     assert(this->m_free <= this->m_size);
// TODO     assert(this->m_used >= 0);
// TODO     return true;
// TODO }

void Region::add_to_garbage_set( Object *object )
{
    unsigned int objSize = object->getSize();
    // Livesize goes down.
    this->m_live -= objSize;
    // Add to garbage waiting set
    this->m_garbage_waiting.insert(object);
    // Remove from live set
    this->m_live_set.erase(object);
    // Keep a running total of how much garbage there is.
    this->m_garbage += objSize;
    // TODO DEBUG cerr << "    add_to_garbage_set <= " << this->m_garbage << endl;
    // Set the flag. This is the ONLY place this flag is set.
    object->setGarbageFlag();
}

bool Region::makeDead( Object *object )
{
    // TODO DEBUG cerr << "Region::makeDead" << endl;
    // Found object.
    // Remove from m_live_set and put into m_garbage_waiting
    bool flag = false;
    ObjectSet_t::iterator iter = this->m_live_set.find(object);
    if (iter == this->m_live_set.end()) {
        // Not in live set.
        // cout << "ERROR: makeDead on object Id[ " << object->getId()
        //      << " ] but can not find in live set.";
        return false;
    }
    if (!object->isGarbage()) {
        this->add_to_garbage_set( object );
        flag = true;
        // If flag (result) is false, that means I tried to add to 
        // the garbage set but didn't find it there.
    } else {
        // What do we do if the object was already garbage?
        // cout << "ERROR: makeDead on object Id[ " << object->getId()
        //      << " ] already set to garbage.";
    }
    // Remove from live_set regardless.
    this->m_live_set.erase(object);
    // Note that we don't adjust the sizes here. This is done in
    // 'add_to_garbage_set'.
    // return whether or not we were able to make the object garbage.
    return flag;
}

int Region::collect( unsigned int timestamp,
                     unsigned int timestamp_alloc )
{
    // Clear the garbage waiting set and return the space to free.
    int collected = this->m_garbage;
    this->GC_attempts++;
    this->m_garbage_waiting.clear(); // this is the waiting set of garbage
    // Garbage in this region is now 0.
    this->setGarbage(0);
    // The GC record printed out
    cout << "GC[" << this->get_name() << ","
        << this->GC_attempts << ","
         << timestamp << ","
         << timestamp_alloc << ","
         << collected << "]" << endl;
    // TODO TODO: ADD THE GC NUMBER
    // Record this collection
    GCRecord_t rec = make_pair( timestamp_alloc, collected );
    this->m_gc_history.push_back( rec );
    // Return how much we collected
    return collected;
}

int Region::setGarbage( int newval )
{
    this->m_garbage = newval;
    return newval;
}

