#include "memorymgr.h"
#include "heap.h"

#include <utility>

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

//---------------------------------------------------------------------------------
//===========[ MemoryMgr ]=========================================================

// Initialize all the memory and regions
// Takes a std::vector list of sizes.
// Assuming index of size corresponds to level
bool MemoryMgr::initialize_memory( unsigned long int heapsize )
{
    // Do I send in a vector of NAMES for the regions?
    this->m_alloc_region = this->new_region( MemoryMgr::ALLOC,
                                             0 ); // Level 0 is required.
    this->m_size = heapsize;
    this->m_free = heapsize;

    if (this->m_alloc_region == NULL) {
        cerr << "Unable to allocate in our simulator. REAL OOM in your system. Quitting." << endl;
        exit(1);
    }

    return true;
}


// TODO // Do a garbage collection
// TODO // Returns number of bytes collected
// TODO int MemoryMgr::do_collection()
// TODO {
// TODO     return 0;
// TODO }

// Returns true if GC threshold has been exceeded
//         false otherwise.
bool MemoryMgr::should_do_collection()
{
    assert(false);
    // NOT NEEDED NOW.
    // Assume that threshold is always valid.
    return (this->m_liveSize >= this->m_GC_byte_threshold);
}

// Returns true if allocation caused garbage collection.
//         false otherwise.
bool MemoryMgr::allocate( Object *object,
                          unsigned int create_time,
                          unsigned int new_alloc_time )
{
    assert(this->m_alloc_region);
    int collected = 0; // Amount collected
    bool GCdone = false;

    this->m_alloc_time = new_alloc_time;
    ObjectSet_t::iterator iter = this->m_live_set.find( object );
    if (iter != this->m_live_set.end()) {
        // Found a dupe.
        // Always return true, but ignore the actual allocation.
        return true;
    }
    // Decisions for collection should be done here at the MemoryMgr level.
    unsigned int objSize = object->getSize();
    if (objSize > this->m_free) {
        // Not enough free space.
        // 1. We collect on a failed allocation.
        collected = this->m_alloc_region->collect( create_time, new_alloc_time );
        // Record how many edges were traced and add to total
        this->m_mark_nonregion_total += this->m_nonregion_edges.size();
        // Add the freed space back to free.
        this->m_free += collected;
        GCdone = true;
        if (objSize > this->m_free) {
            // Out Of Memory. Game over.
            cerr << "OOM: free = " << this->m_free
                 << " | objsize = " << objSize
                 << " | collected = " << collected << endl;
            return false;
        }
    }
    this->m_alloc_region->allocate( object, create_time );
    this->m_free -= objSize; // Free goes down.
    this->m_used += objSize; // Used goes up.
    unsigned long int temp = this->m_liveSize + object->getSize();
    // Max live size calculation
    // We silently peg to ULONG_MAX the wraparound.
    // TODO: Maybe we should just error here as this probably isn't possible.
    if (temp < this->m_liveSize) {
        cerr << "ERROR: Wraparound " << temp << " < " << this->m_liveSize << endl;
    }
    this->m_liveSize = temp;
    // Add to live set.
    this->m_live_set.insert( object );
    // Keep tally of what our maximum live size is for the program run
    if (this->m_maxLiveSize < this->m_liveSize) {
        this->m_maxLiveSize = this->m_liveSize;
    }
    // NOT NEEDED NOW: if (!GCdone && this->should_do_collection()) {
    // NOT NEEDED NOW:     collected += this->m_alloc_region->collect( create_time );
    // NOT NEEDED NOW:     GCdone = true;
    // NOT NEEDED NOW: }
    if (GCdone) {
        // Increment the GC count
        this->m_times_GC++;
    }
    return true;
}

// Create new region with the given name.
// Returns a reference to the region.
Region *MemoryMgr::new_region( string &region_name,
                               int level )
{
    // Debug
    cerr << "Creating a new region[ " << region_name << " ]: "
         << "  | level = " << level << endl;
    RegionMap_t::iterator iter = this->m_region_map.find(region_name);
    // Blow up if we create a new region with the same name.
    assert(iter == this->m_region_map.end());
    assert(level >= 0); // TODO make this more informative
    Region *regptr = new Region( region_name, level );
    assert(regptr); // TODO make this more informative
    this->m_region_map[region_name] = regptr;
    return regptr;
}

bool MemoryMgr::makeDead( Object *object, unsigned int death_time )
{
    // Which region? Since we only have one region in this basic MemmoryMgr:
    unsigned long int temp = this->m_liveSize - object->getSize();
    if (temp > this->m_liveSize) {
        // OVERFLOW, underflow?
        this->m_liveSize = 0;
        cout << "UNDERFLOW of substraction." << endl;
        // TODO If this happens, maybe we should think about why it happens.
    } else {
        // All good. Fight on.
        this->m_liveSize = temp;
    }
    bool result = this->m_alloc_region->makeDead( object );
    if (!object->isDead()) {
        object->makeDead( death_time, this->m_alloc_time );
    }
    // TODO: Think about whether calling object->makeDead is better in region::makeDead
    return result;
}

// On an U(pdate) event
void MemoryMgr::add_edge( ObjectId_t src,
                          ObjectId_t tgt )
{
    // DEBUG cout << "Adding edge (" << src << "," << tgt << ")" << endl;
    //----------------------------------------------------------------------
    // Add to edge maps
    // Source is NOT in special group because there IS no special group.
    // Target is NOT in special group for the same reason.
    ObjectId2SetMap_t::iterator itmp = this->m_nonregion_edges.find(src);
    if (itmp != this->m_nonregion_edges.end()) {
        // Already in the map
        ObjectIdSet_t &myset = itmp->second;
        myset.insert(tgt);
    } else {
        // Not in the map
        ObjectIdSet_t myset;
        myset.insert(tgt);
        this->m_nonregion_edges[src] = myset;
    }
    this->m_nonregion_edges_count++;
    //----------------------------------------------------------------------
    // Add to look up maps
    // Src map
    ObjectId2SetMap_t::iterator miter = this->m_srcidmap.find(src);
    if (miter != this->m_srcidmap.end()) {
        // Already exists
        this->m_srcidmap[src].insert(tgt);
    } else {
        // Doesn't exist
        ObjectIdSet_t tmpset;
        tmpset.insert(tgt);
        this->m_srcidmap[src] = tmpset;
    }
    //----------------------------------------------------------------------
    // Tgt map
    miter = this->m_tgtidmap.find(tgt);
    if (miter != this->m_tgtidmap.end()) {
        // Already exists
        this->m_tgtidmap[src].insert(src);
    } else {
        // Doesn't exist
        ObjectIdSet_t tmpset;
        tmpset.insert(src);
        this->m_tgtidmap[tgt] = tmpset;
    }
} 

void MemoryMgr::remove_edge( ObjectId_t src,
                             ObjectId_t oldTgtId )
{
    // DEBUG cout << "Remove edge (" << src << "," << oldTgtId << ")" << endl;
    ObjectId2SetMap_t::iterator iter;
    this->m_attempts_edges_removed++;
    //----------------------------------------------------------------------
    // Remove edge from region maps
    // Look in nonregion because we know everything else is empty
    // in the base MemoryMgr.
    iter = this->m_nonregion_edges.find(src);
    if (iter != this->m_nonregion_edges.end()) {
        // Found in nonregion
        ObjectIdSet_t &myset = iter->second;
        myset.erase(oldTgtId);
        this->m_edges_removed++;
    } else {
        // TODO: DEBUG for edges not found?
        // TODO: At least keep track of how many edges weren't found.
    }
    return;
}

// - TODO
void MemoryMgr::remove_from_srcidmap( ObjectId_t src,
                                      ObjectId_t oldTgtId )
{
    //----------------------------------------------------------------------
    // Remove from appropriate lookup maps
    ObjectId2SetMap_t::iterator miter = this->m_srcidmap.find(src);
    if (miter != this->m_srcidmap.end()) {
        // Found the source.
        // Look for the old target id in the set
        ObjectIdSet_t &myset = miter->second;
        ObjectIdSet_t::iterator itmp = myset.find(oldTgtId);
        if (itmp != myset.end()) {
            // Remove old target
            myset.erase(itmp);
        }
        // DEBUG
        // itmp = myset.find(oldTgtId);
        // assert( itmp == myset.end() );
    }
}

// - TODO
void MemoryMgr::remove_from_tgtidmap( ObjectId_t src,
                                      ObjectId_t tgtId )
{
    ObjectId2SetMap_t::iterator miter = this->m_tgtidmap.find(tgtId);
    if (miter != this->m_tgtidmap.end()) {
        // Found the old target.
        // Look for the source id in the set
        ObjectIdSet_t &myset = miter->second;
        ObjectIdSet_t::iterator itmp = myset.find(src);
        if (itmp != myset.end()) {
            myset.erase(itmp);
        }
        // DEBUG
        // itmp = myset.find(src);
        // assert( itmp == myset.end() );
    }
}

// - TODO Documentation
void MemoryMgr::remove_object( ObjectId_t objId )
{
    ObjectId2SetMap_t::iterator iter;
    //------------------------------------------------------------
    // Look for objId in m_srcidmap
    iter = this->m_srcidmap.find(objId);
    if (iter != this->m_srcidmap.end()) {
        // Found the objId as source.
        // Go through the and remove all outgoing edges:
        ObjectIdSet_t &myset = iter->second;
        ObjectIdSet_t::iterator siter = myset.begin();
        while (siter != myset.end()) {
            // Remove target (src, *siter)
            this->remove_edge( objId, *siter );
            siter++;
        }
        this->m_srcidmap.erase(iter);
    }
    //------------------------------------------------------------
    // Look for objId in m_tgtidmap
    iter = this->m_tgtidmap.find(objId);
    if (iter != this->m_tgtidmap.end()) {
        // Found the objId as target.
        // Go through the and remove all incoming edges:
        ObjectIdSet_t &myset = iter->second;
        ObjectIdSet_t::iterator siter = myset.begin();
        while (siter != myset.end()) {
            // Remove target (src, *siter)
            this->remove_edge( *siter, objId );
            siter++;
        }
        this->m_tgtidmap.erase(iter);
    }
}

deque<GCRecord_t> MemoryMgr::get_GC_history()
{
    deque<GCRecord_t> result;
    for ( RegionMap_t::iterator iter = this->m_region_map.begin();
          iter != this->m_region_map.end();
          ++iter ) {
        Region *ptr = iter->second;
        deque<GCRecord_t> myhist = ptr->get_GC_history();
        result.insert( result.end(), myhist.begin(), myhist.end() );
    }
    return result;
}

bool MemoryMgr::is_in_live_set( Object *object )
{
    ObjectSet_t::iterator iter = this->m_live_set.find( object );
    return (iter != this->m_live_set.end());
}

//==============================================================================
// Debug functions
void Region::print_status()
{
    cout << "Region[ " << this->m_name << " ]" << endl
         << "    - live   : " << this->m_live << endl
         << "    - garbage: " << this->m_garbage << endl;
}

void MemoryMgr::print_status()
{
}

unsigned long int MemoryMgr::get_num_GC_attempts( bool printflag )
{
    // For every region that we manage:
    //      get the number of GC attempts per region.
    //      if printflag: print
    // TODO.
    return 0;
}

