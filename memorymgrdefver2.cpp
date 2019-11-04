//==============================================================================
// MemoryMgrDefVer2
//==============================================================================
#include "heap.h"
#include "memorymgrdefver2.hpp"

string MemoryMgrDefVer2::SPECIAL_VER2= "SPECIAL_VER2";

bool MemoryMgrDefVer2::allocate( Object *object,
                                 unsigned int create_time,
                                 unsigned int new_alloc_time )
{
    assert(this->m_alloc_region);
    int collected_regular = 0; // Amount collected from the REGULAR region
    int collected_special = 0; // Amount collected from the SPECIAL region

    this->m_alloc_time = new_alloc_time;
    // Decisions for collection should be done here at the MemoryMgr level.
    // Check if we have space
    unsigned int objSize = object->getSize();
    if (objSize > this->m_free) {
        // Not enough free space.
        // 1. We collect the REGULAR region first on a failed allocation.
        collected_regular = this->m_alloc_region->collect( create_time, new_alloc_time );
        // Increment the GC count
        this->m_times_GC++;
        // Count the mark count
        this->m_mark_nonregion_total += ( this->m_nonregion_edges.size() +
                                          this->m_in_edges_p->size() +
                                          this->m_in_edges_p->size() );
        // Add back the collected 
        this->m_free += collected_regular;
        if (objSize > this->m_free) {
            // 2. Try again with the SPECIAL region.
            collected_special += this->m_defregion_p->collect( create_time, new_alloc_time );
            // Add back the SPECIAL collected 
            this->m_free += collected_special;
            // Count the mark cost since we had to collect the SPECIAL region
            this->m_mark_region_total += this->m_nonregion_edges.size();
            if (objSize > this->m_free) {
                // Out Of Memory.
                cerr << "OOM: free = " << this->m_free
                     << " | objsize = " << objSize
                     << " | collected regular = " << collected_regular
                     << " | collected special = " << collected_special << endl;
                return false;
            }
        } else {
            // This is the mark savings.
            this->m_mark_saved_total += this->m_region_edges_p->size();
        }
    }
    // This has to be true because of the collections
    assert( objSize <= this->m_free );
    // Check for duplicates
    auto iter_live = this->m_live_set.find( object );
    if (iter_live != this->m_live_set.end()) {
        // Found a dupe.
        // Always return true, but ignore the actual allocation.
        return true;
    }
    // Check which region to allocate into
    ObjectId_t objId = object->getId();
    auto iter_sgroup = this->m_specgroup.find(objId);
    if (iter_sgroup != this->m_specgroup.end()) {
        // allocate into SPECIAL (aka DEFERRED) group
        this->m_defregion_p->allocate( object, create_time );
    } else {
        // allocate into REGULAR group
        this->m_alloc_region->allocate( object, create_time );
    }
    unsigned long int temp = this->m_liveSize + object->getSize();
    // Max live size calculation
    // We silently peg to ULONG_MAX the wraparound.
    // TODO: Maybe we should just error here as this probably isn't possible.
    this->m_liveSize = ( (temp < this->m_liveSize) ? ULONG_MAX : temp );
    // Add to live set.
    this->m_live_set.insert( object );
    // Keep tally of what our maximum live size is for the program run
    if (this->m_maxLiveSize < this->m_liveSize) {
        this->m_maxLiveSize = this->m_liveSize;
    }
    this->m_free -= objSize;
    return true;
}

// On an U(pdate) event
void MemoryMgrDefVer2::add_edge( ObjectId_t src,
                                 ObjectId_t tgt )
{
    // DEBUG
    // cout << "Adding edge (" << src << "," << tgt << ")";
    ObjectIdSet_t::iterator iter = this->m_specgroup.find(src);
    ObjectIdSet_t::iterator tgt_iter = this->m_specgroup.find(tgt);
    //----------------------------------------------------------------------
    // Add to edge maps
    if (iter != this->m_specgroup.end()) {
        // Source is in special group
        if (tgt_iter != this->m_specgroup.end()) {
            // DEBUG: cout << "..[src in SPECIAL] ";
            // Target is in special group
            ObjectId2SetMap_t::iterator itmp = this->m_region_edges_p->find(src);
            if (itmp != this->m_region_edges_p->end()) {
                // Already in the map
                ObjectIdSet_t &myset = itmp->second;
                myset.insert(tgt);
            } else {
                // Not in the map
                ObjectIdSet_t myset;
                myset.insert(tgt);
                (*this->m_region_edges_p)[src] = myset;
            }
            // // DEBUG ONLY
            // itmp = this->m_region_edges_p->find(src);
            // assert(itmp != this->m_region_edges_p->end());
            // ObjectIdSet_t &myset = itmp->second;
            // ObjectIdSet_t::iterator tmpiter = myset.find(tgt);
            // assert(tmpiter != myset.end());
            // // END DEBUG
            this->m_region_edges_count++;
        } else {
            // Target is NOT in special group
            // cout << "..[src in REGULAR] ";
            ObjectId2SetMap_t::iterator itmp = this->m_out_edges_p->find(src);
            if (itmp != this->m_out_edges_p->end()) {
                // Already in the map
                ObjectIdSet_t &myset = itmp->second;
                myset.insert(tgt);
            } else {
                // Not in the map
                ObjectIdSet_t myset;
                myset.insert(tgt);
                (*this->m_out_edges_p)[src] = myset;
            }
            this->m_out_edges_count++;
        }
    } else {
        // Source is NOT in special group
        // cout << "..[src in REGULAR] ";
        if (tgt_iter != this->m_specgroup.end()) {
            // Target is in special group
            // DEBUG: cout << "..[tgt in SPECIAL] ";
            ObjectId2SetMap_t::iterator itmp = this->m_in_edges_p->find(src);
            if (itmp != this->m_in_edges_p->end()) {
                // Already in the map
                ObjectIdSet_t &myset = itmp->second;
                myset.insert(tgt);
            } else {
                // Not in the map
                ObjectIdSet_t myset;
                myset.insert(tgt);
                (*this->m_in_edges_p)[src] = myset;
            }
            this->m_in_edges_count++;
        } else {
            // cout << "..[tgt in REGULAR] ";
            // Target is NOT in special group
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
        }
    }
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

void MemoryMgrDefVer2::remove_edge( ObjectId_t src,
                                    ObjectId_t oldTgtId )
{
    // DEBUG
    // cout << "DEF: Remove edge (" << src << "," << oldTgtId << ")" << endl;
    ObjectId2SetMap_t::iterator iter;
    this->m_attempts_edges_removed++;
    //----------------------------------------------------------------------
    // Remove edge from region maps
    // Look in the special region
    iter = this->m_region_edges_p->find(src);
    if (iter != this->m_region_edges_p->end()) {
        // Found in region
        ObjectIdSet_t &myset = iter->second;
        myset.erase(oldTgtId);
        this->m_edges_removed++;
        goto END;
    }
    // Look outside the special region
    iter = this->m_nonregion_edges.find(src);
    if (iter != this->m_nonregion_edges.end()) {
        // Found in nonregion
        ObjectIdSet_t &myset = iter->second;
        myset.erase(oldTgtId);
        this->m_edges_removed++;
        goto END;
    }
    // Look in the in to out 
    iter = this->m_in_edges_p->find(src);
    if (iter != this->m_in_edges_p->end()) {
        // Found in IN region 
        ObjectIdSet_t &myset = iter->second;
        myset.erase(oldTgtId);
        this->m_edges_removed++;
        goto END;
    }
    // Look in the out to in 
    iter = this->m_out_edges_p->find(src);
    if (iter != this->m_out_edges_p->end()) {
        // Found in IN region 
        ObjectIdSet_t &myset = iter->second;
        myset.erase(oldTgtId);
        this->m_edges_removed++;
        goto END;
        // Well this isn't needed but symmetric.
        // If ever there's any new code after this, this makes it less likely
        // that a bug's introduced.
    }
    END:
    return;
}

bool MemoryMgrDefVer2::initialize_memory( unsigned long int heapsize )
{
    // Do I send in a vector of NAMES for the regions?
    MemoryMgr::initialize_memory( heapsize );
    this->m_defregion_p = this->new_region( MemoryMgrDefVer2::SPECIAL_VER2,
                                            1 );
    // The super-edge sets
    this->m_region_edges_p = new ObjectId2SetMap_t();
    this->m_in_edges_p = new ObjectId2SetMap_t();
    this->m_out_edges_p = new ObjectId2SetMap_t();
    return true;
}

// Initialize the grouped region of objects
bool MemoryMgrDefVer2::initialize_special_group( string &group_filename,
                                                 int numgroups )
{
    cout << "initialize_special_group: ";
    std::ifstream infile( group_filename );
    string line;
    string s;
    this->m_numgroups = numgroups;
    // assert( numgroups == 1 );
    int group_count = 0;
    // The file is a CSV file with the following header:
    //    groupId,number,death_time,list
    // First line is a header:
    std::getline(infile, line);
    // TODO: Maybe make sure we have the right file?
    while (std::getline(infile, line)) {
        size_t pos = 0;
        string token;
        unsigned long int num;
        int count = 0;
        //------------------------------------------------------------
        // Get the group Id
        pos = line.find(",");
        assert( pos != string::npos );
        s = line.substr(0, pos);
        // DEBUG: cout << "GID: " << s << endl;
        int groupId = std::stoi(s);
        line.erase(0, pos + 1);
        //------------------------------------------------------------
        // Get the number of objects in the group
        pos = line.find(",");
        assert( pos != string::npos );
        s = line.substr(0, pos);
        // DEBUG: cout << "NUM: " << s << endl;
        int total = std::stoi(s);
        line.erase(0, pos + 1);
        //------------------------------------------------------------
        // Get the deathtime
        pos = line.find(",");
        assert( pos != string::npos );
        s = line.substr(0, pos);
        // DEBUG: cout << "DTIME: " << s << endl;
        int dtime = std::stoi(s);
        // TODO TODO TODO TODO
        // This assumes that there's only one region here, so it doesn't work if
        // there are multiple regions. To do multiple regions, we need a map from
        // region number to region. This way it should match the groups text file
        // we are getting the death groups information.
        this->m_defregion_p->set_region_deathtime(dtime);
        line.erase(0, pos + 1);
        //------------------------------------------------------------
        // Get the object Ids
        while ((pos = line.find(",")) != string::npos) {
            token = line.substr(0, pos);
            num = std::stoi(token);
            line.erase(0, pos + 1);
            ++count;
            this->m_specgroup.insert(num);
        }
        // Get the last number
        num = std::stoi(line);
        this->m_specgroup.insert(num);
        ++count;
        ++group_count;
        // DEBUG
        cout << "Special group[ " << groupId << " ]:  "
             << "total objects read in = " << total << " | "
             << "set size = " << this->m_specgroup.size() << endl;
        // END DEBUG
        if (group_count >= numgroups) {
            break;
        }
    }
    return true;
}

// TODO DOC
bool MemoryMgrDefVer2::makeDead( Object *object, unsigned int death_time )
{
    // Check which region the object belongs to:
    bool result;
    ObjectId_t objId = object->getId();
    auto iter_sgroup = this->m_specgroup.find(objId);
    if (iter_sgroup != this->m_specgroup.end()) {
        // In the SPECIAL (aka DEFERRED) group
        result = this->m_defregion_p->makeDead( object );
    } else {
        // In the REGULAR group
        result = this->m_alloc_region->makeDead( object );
    }
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
    if (!object->isDead()) {
        object->makeDead( death_time, this->m_alloc_time );
    }
    return result;
}

