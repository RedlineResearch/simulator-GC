#ifndef CLASSINFO_H
#define CLASSINFO_H

// ----------------------------------------------------------------------
//   Representation of class info
//   

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <deque>

using namespace std;

class Class;
class Method;
class Field;
class AllocSite;

typedef map<unsigned int, Method*> MethodMap;
typedef map<unsigned int, Field*> FieldMap;
typedef map<unsigned int, Class*> ClassMap;
typedef map<unsigned int, AllocSite*> AllocSiteMap;

typedef deque<unsigned int> DequeId_t;

// -- Global info, including the big routine to read the names file

typedef std::pair<Method *, Method *> ContextPair;
    // (f,g) where f is the caller (or returner)
    // and g is the current function

class ClassInfo
{
    public:
        // -- Contents of the names file
        static ClassMap TheClasses;
        // -- All methods (also in the classes)
        static MethodMap TheMethods;
        // -- All fields (also in the classes)
        static FieldMap TheFields;
        // -- Allocation sites
        static AllocSiteMap TheAllocSites;
        // -- Debug flag
        static bool debug_names;
        // -- Flag whether we check for the main function or not
        static bool main_flag;
        // -- Read the names file
        static void read_names_file( const char *filename,
                                     string main_package,
                                     string main_function );
        static void read_names_file_no_mainfunc( const char *filename );
        static void __read_names_file( const char *filename,
                                       string main_class,
                                       string main_function );
        static Method * get_main_method() {
            return _main_method;
        }
        static void set_main_flag() {
            ClassInfo::main_flag = true;
        }
        static void unset_main_flag() {
            ClassInfo::main_flag = false;
        }

    private:
        static Method *_main_method;
};

// -- Representation of classes

class Entity
{
    protected:
        unsigned int m_id;
        string m_name;

    public:
        Entity()
            : m_id(0)
            , m_name() {
        }

        Entity(unsigned int id, const string& name)
            : m_id(id)
            , m_name(name) {
        }

        unsigned int getId() const { return m_id; }
        const string& getName() const { return m_name; }
};

class Field : public Entity
{
    private:
        bool m_isStatic;
        Class* m_class;
        string m_descriptor;

    public:
        Field(unsigned int id, Class* cls, char* name, char* descriptor, bool isstatic)
            : Entity(id, name)
            , m_isStatic(isstatic)
            , m_class(cls)
            , m_descriptor(descriptor) {
        }

        Class* getClass() const { return m_class; }
};


class AllocSite : public Entity
{
    private:
        Method* m_method;
        string m_descriptor;
        int m_dimensions;

    public:
        AllocSite( unsigned int id,
                   Method* meth,
                   char* name,
                   char* descriptor,
                   unsigned int dimensions )
            : Entity(id, name)
            , m_method(meth)
            , m_descriptor(descriptor)
            , m_dimensions(dimensions) {
        }

        Method* getMethod() const { return m_method; }
        const string& getType() const { return m_descriptor; }

        string info();
};

class Method : public Entity
{
    private:
        Class* m_class;
        string m_descriptor;
        string m_flags;
        AllocSiteMap m_allocsites;

    public:
        Method( unsigned int id,
                Class *cls,
                char *name,
                char *descriptor,
                char *flags )
            : Entity(id, name)
            , m_class(cls)
            , m_descriptor(descriptor)
            , m_flags(flags) {
        }

        Class* getClass() const { return m_class; }

        void addAllocSite(AllocSite* a) { m_allocsites[a->getId()] = a; }

        string info();

        string getName();
};

class Class : public Entity
{
    private:
        unsigned int m_superclassId;
        bool m_is_interface;
        MethodMap m_methods;
        FieldMap m_fields;

    public:
        Class(unsigned int id, const string& name, bool is_interface)
            : Entity(id, name)
            , m_superclassId(0)
            , m_is_interface(is_interface) {
        }

        void setSuperclassId(unsigned int sid) { m_superclassId = sid; }

        void addMethod(Method* m) { m_methods[m->getId()] = m; }
        void addField(Field* f) { m_fields[f->getId()] = f; }

        string info() { return m_name; }
};

#endif
