#ifndef MEMORYMGR_DEF_H
#define MEMORYMGR_DEF_H

#include "region.hpp"
#include "memorymgr.h"

class MemoryMgrDef : public MemoryMgr
{
private:
    // static string ALLOC;
    static string SPECIAL;

public:
    MemoryMgrDef()
        : m_region_edges_p( NULL )
        , m_in_edges_p( NULL )
        , m_out_edges_p( NULL )
        , m_defregion_p( NULL ) {
        }
    // Returns true if allocation caused garbage collection.
    //         false otherwise.
    virtual bool allocate( Object *object,
                           unsigned int create_time,
                           unsigned int new_alloc_time );

    // On a D(eath) event
    virtual bool makeDead( Object *object, unsigned int death_time );

    // Edges (adding and removing)
    virtual void add_edge( ObjectId_t src, ObjectId_t tgt );
    virtual void remove_edge( ObjectId_t src, ObjectId_t oldTgtId );
    //
    // Initializes all the regions. This should contain all knowledge
    // of how things are laid out.
    virtual bool initialize_memory( unsigned long int heapsize );

    // Initialize the grouped region of objects
    virtual bool initialize_special_group( string &group_filename,
                                           int numgroups );

    // Mark count getter functions
    // Mark related getters
    virtual unsigned int get_mark_saved_total() const
    {
        return this->m_mark_saved_total;
    }

    virtual unsigned int get_mark_region_total() const
    {
        return this->m_mark_region_total;
    }

protected:
    // Edge sets and remember sets
    //     * edges where source and target are in the region
    ObjectId2SetMap_t *m_region_edges_p;
    //     * edges where source is outside and target is in the region
    ObjectId2SetMap_t *m_in_edges_p;
    //     * edges where source is inside and target is outside the region
    ObjectId2SetMap_t *m_out_edges_p;

    Region *m_defregion_p;

};

#endif
