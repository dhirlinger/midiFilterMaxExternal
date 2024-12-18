/**
    @file
    midi filter - patch midi filter
    doug hirlinger - doughirlinger@gmail.com

    @ingroup    examples
*/

#include "ext.h"
#include "ext_obex.h"
#include "ext_strings.h"
#include "ext_common.h"
#include "ext_systhread.h"

#include <vector>
using namespace std;

// a wrapper for cpost() only called for debug builds on Windows
// to see these console posts, run the DbgView program (part of the SysInternals package distributed by Microsoft)
#if defined( NDEBUG ) || defined( MAC_VERSION )
#define DPOST
#else
#define DPOST cpost
#endif


// a c++ class representing a number, and types for a vector of those numbers
class number {
private:
    double value;
public:
    number(double &newValue)
    {
        value = newValue;
    }

    void setValue(const double &newValue)
    {
        value = newValue;
    }

    void getValue(double &retrievedValue)
    {
        retrievedValue = value;
    }
};
typedef std::vector<number>        numberVector;
typedef numberVector::iterator    numberIterator;


// max object instance data
typedef struct _midiFilter {
    t_object            c_box;
    numberVector        *c_vector;    // note: you must store this as a pointer and not directly as a member of the object's struct
    numberVector        *c_vector2;    // note: you must store this as a pointer and not directly as a member of the object's struct
    void                *c_outlet;
    t_systhread_mutex    c_mutex;
} t_midiFilter;


// prototypes
void    *midiFilter_new(t_symbol *s, long argc, t_atom *argv);
void    midiFilter_free(t_midiFilter *x);
void    midiFilter_assist(t_midiFilter *x, void *b, long m, long a, char *s);
void    midiFilter_bang(t_midiFilter *x);
void    midiFilter_vector2(t_midiFilter *x); //same as bang but for list2
void    midiFilter_count(t_midiFilter *x);
void    midiFilter_int(t_midiFilter *x, long value);
void    midiFilter_float(t_midiFilter *x, double value);
void    midiFilter_list(t_midiFilter *x, t_symbol *msg, long argc, t_atom *argv);
void    midiFilter_list2(t_midiFilter *x, t_symbol *msg, long argc, t_atom *argv);
void    midiFilter_clear(t_midiFilter *x);


// globals
static t_class    *s_midiFilter_class = NULL;

/************************************************************************************/

void ext_main(void *r)
{
    t_class    *c = class_new("midiFilter",
                           (method)midiFilter_new,
                           (method)midiFilter_free,
                           sizeof(t_midiFilter),
                           (method)NULL,
                           A_GIMME,
                           0);

    class_addmethod(c, (method)midiFilter_bang,    "bang",            0);
    class_addmethod(c, (method)midiFilter_vector2,    "vector2",            0);
    class_addmethod(c, (method)midiFilter_int,        "int",            A_LONG,    0);
    class_addmethod(c, (method)midiFilter_float,    "float",        A_FLOAT,0);
    class_addmethod(c, (method)midiFilter_list,    "list",            A_GIMME,0);
    class_addmethod(c, (method)midiFilter_list2,    "list2",            A_GIMME,0);
    class_addmethod(c, (method)midiFilter_clear,    "clear",        0);
    class_addmethod(c, (method)midiFilter_count,    "count",        0);
    class_addmethod(c, (method)midiFilter_assist,    "assist",        A_CANT, 0);
    class_addmethod(c, (method)stdinletinfo,    "inletinfo",    A_CANT, 0);

    class_register(CLASS_BOX, c);
    s_midiFilter_class = c;
}


/************************************************************************************/
// Object Creation Method

void *midiFilter_new(t_symbol *s, long argc, t_atom *argv)
{
    t_midiFilter    *x;

    x = (t_midiFilter *)object_alloc(s_midiFilter_class);
    if (x) {
        systhread_mutex_new(&x->c_mutex, 0);
        x->c_outlet = outlet_new(x, NULL);
        x->c_vector = new numberVector;
        x->c_vector->reserve(10);
        midiFilter_list(x, gensym("list"), argc, argv);
        x->c_vector2 = new numberVector;
        x->c_vector2->reserve(10);
        midiFilter_list2(x, gensym("list2"), argc, argv);
    }
    return(x);
}


void midiFilter_free(t_midiFilter *x)
{
    systhread_mutex_free(x->c_mutex);
    delete x->c_vector;
    delete x->c_vector2;
}


/************************************************************************************/
// Methods bound to input/inlets

void midiFilter_assist(t_midiFilter *x, void *b, long msg, long arg, char *dst)
{
    if (msg==1)
        strcpy(dst, "input");
    else if (msg==2)
        strcpy(dst, "output");
}


void midiFilter_bang(t_midiFilter *x)
{
    numberIterator iter, begin, end;
    int i = 0;
    long ac = 0;
    t_atom *av = NULL;
    double value;

    DPOST("head\n");
    systhread_mutex_lock(x->c_mutex);
    ac = x->c_vector->size();

    DPOST("ac=%ld\n", ac);
    if (ac)
        av = new t_atom[ac];

    if (ac && av) {
        DPOST("assigning begin and end\n");
        begin = x->c_vector->begin();
        end = x->c_vector->end();

        DPOST("assigning iter\n");
        iter = begin;

        DPOST("entering for\n", ac);
        for (;;) {
            DPOST("i=%i\n", i);
            (*iter).getValue(value);
            atom_setfloat(av+i, value);

            DPOST("incrementing\n");
            i++;
            iter++;

            DPOST("comparing\n");
            if (iter == end)
                break;
        }
        systhread_mutex_unlock(x->c_mutex);    // must unlock before calling _clear() or we will deadlock

//        DPOST("about to clear\n", ac);
//        midiFilter_clear(x);

        DPOST("about to outlet\n", ac);
        outlet_anything(x->c_outlet, gensym("list"), ac, av); // don't want to call outlets in mutexes either

        DPOST("about to delete\n", ac);
        delete[] av;
    }
    else
        systhread_mutex_unlock(x->c_mutex);
}

void midiFilter_vector2(t_midiFilter *x)
{
    numberIterator iter, begin, end;
    int i = 0;
    long ac = 0;
    t_atom *av = NULL;
    double value;

    DPOST("head\n");
    systhread_mutex_lock(x->c_mutex);
    ac = x->c_vector2->size();

    DPOST("ac=%ld\n", ac);
    if (ac)
        av = new t_atom[ac];

    if (ac && av) {
        DPOST("assigning begin and end\n");
        begin = x->c_vector2->begin();
        end = x->c_vector2->end();

        DPOST("assigning iter\n");
        iter = begin;

        DPOST("entering for\n", ac);
        for (;;) {
            DPOST("i=%i\n", i);
            (*iter).getValue(value);
            atom_setfloat(av+i, value);

            DPOST("incrementing\n");
            i++;
            iter++;

            DPOST("comparing\n");
            if (iter == end)
                break;
        }
        systhread_mutex_unlock(x->c_mutex);    // must unlock before calling _clear() or we will deadlock

//        DPOST("about to clear\n", ac);
//        midiFilter_clear(x);

        DPOST("about to outlet\n", ac);
        outlet_anything(x->c_outlet, gensym("list"), ac, av); // don't want to call outlets in mutexes either

        DPOST("about to delete\n", ac);
        delete[] av;
    }
    else
        systhread_mutex_unlock(x->c_mutex);
}


void midiFilter_count(t_midiFilter *x)
{
    outlet_int(x->c_outlet, x->c_vector->size());
    outlet_int(x->c_outlet, x->c_vector2->size());
}


void midiFilter_int(t_midiFilter *x, long value)
{
    midiFilter_float(x, value);
    
}


void midiFilter_float(t_midiFilter *x, double value)
{
    systhread_mutex_lock(x->c_mutex);
    x->c_vector->push_back(value);
    systhread_mutex_unlock(x->c_mutex);
    
}


void midiFilter_list(t_midiFilter *x, t_symbol *msg, long argc, t_atom *argv)
{
    systhread_mutex_lock(x->c_mutex);
    for (int i=0; i<argc; i++) {
        double value = atom_getfloat(argv+i);
        x->c_vector->push_back(value);
    }
    systhread_mutex_unlock(x->c_mutex);
}

void midiFilter_list2(t_midiFilter *x, t_symbol *msg, long argc, t_atom *argv)
{
    if (argc > 0) {
        systhread_mutex_lock(x->c_mutex);
        
        double value = atom_getfloat(argv);
        x->c_vector2->push_back(value);
        
        systhread_mutex_unlock(x->c_mutex);
        
        post("midiFilter list2 called");
    }
}


void midiFilter_clear(t_midiFilter *x)
{
    systhread_mutex_lock(x->c_mutex);
    x->c_vector->clear();
    x->c_vector2->clear();
    systhread_mutex_unlock(x->c_mutex);
}

