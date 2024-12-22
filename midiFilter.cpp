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


// a c++ class representing a number, and types for a vector of those numbers
class number {
private:
    long value;
public:
    number(long &newValue)
    {
        value = newValue;
    }

    void setValue(const long &newValue)
    {
        value = newValue;
    }

    void getValue(long &retrievedValue)
    {
        retrievedValue = value;
    }
};
typedef std::vector<number>        numberVector;
typedef numberVector::iterator    numberIterator;


// max object instance data
typedef struct _midiFilter {
    t_object            ob;
    numberVector        *c_mainNotes;    // note: you must store this as a pointer and not directly as a member of the object's struct
    numberVector        *c_localNotes;    // note: you must store this as a pointer and not directly as a member of the object's struct
    void                *c_outlet;
    void                *c_outlet2;
    t_systhread_mutex    c_mutex;
} t_midiFilter;


// prototypes
void    *midiFilter_new(t_symbol *s, long argc, t_atom *argv);
void    midiFilter_free(t_midiFilter *x);
void    midiFilter_assist(t_midiFilter *x, void *b, long m, long a, char *s);
void    midiFilter_bang(t_midiFilter *x);
void    midiFilter_printLocalNotes(t_midiFilter *x); //same as bang but for list2
void    midiFilter_count(t_midiFilter *x);
void    midiFilter_int(t_midiFilter *x, long value);
void    midiFilter_list(t_midiFilter *x, t_symbol *msg, long argc, t_atom *argv);
void    midiFilter_list2(t_midiFilter *x, t_symbol *msg, long argc, t_atom *argv);
void    midiFilter_clear(t_midiFilter *x);
bool    midiFilter_contains(t_midiFilter *x, numberVector &collection, long targetValue);
bool    midiFilter_localMath(t_midiFilter *x, long value);
void    midiFilter_removeValue(t_midiFilter *x, numberVector &collection, long valueToRemove);


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
    class_addmethod(c, (method)midiFilter_printLocalNotes,    "printLocalNotes",            0);
    class_addmethod(c, (method)midiFilter_int,        "int",            A_LONG,    0);
    class_addmethod(c, (method)midiFilter_list,    "list",            A_GIMME,0);
    class_addmethod(c, (method)midiFilter_list2,    "list2",            A_GIMME,0);
    class_addmethod(c, (method)midiFilter_clear,    "clear",        0);
    class_addmethod(c, (method)midiFilter_count,    "count",        0);
    class_addmethod(c, (method)midiFilter_assist,    "assist",        A_CANT, 0);
    class_addmethod(c, (method)stdinletinfo,    "inletinfo",    A_CANT, 0);
    class_addmethod(c, (method)midiFilter_contains, "int",      A_LONG, 0);
    class_addmethod(c, (method)midiFilter_localMath, "int", A_LONG, 0);
    class_addmethod(c, (method)midiFilter_removeValue, "int", A_LONG, 0);

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
        x->c_outlet2 = outlet_new(x, NULL);
        x->c_outlet = outlet_new(x, NULL);
        x->c_mainNotes = new numberVector;
        x->c_mainNotes->reserve(10);
        midiFilter_list(x, gensym("list"), argc, argv);
        x->c_localNotes = new numberVector;
        x->c_localNotes->reserve(10);
        midiFilter_list2(x, gensym("list2"), argc, argv);
    }
    return(x);
}


void midiFilter_free(t_midiFilter *x)
{
    systhread_mutex_free(x->c_mutex);
    delete x->c_mainNotes;
    delete x->c_localNotes;
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
    long value;

    systhread_mutex_lock(x->c_mutex);
    ac = x->c_mainNotes->size();

    if (ac)
        av = new t_atom[ac];

    if (ac && av) {
        
        begin = x->c_mainNotes->begin();
        end = x->c_mainNotes->end();

        iter = begin;

        for (;;) {
            
            (*iter).getValue(value);
            atom_setlong(av+i, value);

            i++;
            iter++;

            if (iter == end)
                break;
        }
        systhread_mutex_unlock(x->c_mutex);    // must unlock before calling _clear() or we will deadlock


        outlet_anything(x->c_outlet2, gensym("list"), ac, av); // don't want to call outlets in mutexes either

        delete[] av;
    }
    else
        systhread_mutex_unlock(x->c_mutex);
}

void midiFilter_printLocalNotes(t_midiFilter *x)
{
    numberIterator iter, begin, end;
    int i = 0;
    long ac = 0;
    t_atom *av = NULL;
    long value;

    systhread_mutex_lock(x->c_mutex);
    ac = x->c_localNotes->size();

    if (ac)
        av = new t_atom[ac];

    if (ac && av) {

        begin = x->c_localNotes->begin();
        end = x->c_localNotes->end();

        iter = begin;

        for (;;) {

            (*iter).getValue(value);
            atom_setlong(av+i, value);

            i++;
            iter++;

            if (iter == end)
                break;
        }
        systhread_mutex_unlock(x->c_mutex);    // must unlock before calling _clear() or we will deadlock

//        DPOST("about to clear\n", ac);
//        midiFilter_clear(x);

        outlet_anything(x->c_outlet2, gensym("list"), ac, av); // don't want to call outlets in mutexes either

        delete[] av;
    }
    else
        systhread_mutex_unlock(x->c_mutex);
}


void midiFilter_count(t_midiFilter *x)
{
    outlet_int(x->c_outlet, x->c_mainNotes->size());
    outlet_int(x->c_outlet, x->c_localNotes->size());
}


void midiFilter_int(t_midiFilter *x, long value)
{
    systhread_mutex_lock(x->c_mutex);
    x->c_mainNotes->push_back(value);
    systhread_mutex_unlock(x->c_mutex);
    
}

void midiFilter_list(t_midiFilter *x, t_symbol *msg, long argc, t_atom *argv)
{
    //if there's an incoming list
    
    if (argc > 0) {
        
        //systhread_mutex_lock(x->c_mutex);
        
        long pitch = atom_getlong(argv);
        long velocity = atom_getlong(argv+1);
        
        //if incoming pitch is note-on and mainNotes is empty then play and add to lists
        
        if (x->c_mainNotes->size() < 1 && velocity > 0) {
            
            x->c_localNotes->push_back(pitch);
            x->c_mainNotes->push_back(pitch);
            
            
            //systhread_mutex_unlock(x->c_mutex);
            
            outlet_list(x->c_outlet, NULL, 2, argv);
            
            return;
            
        //otherwise if mainNotes does exists and is note-on...
            
        } else if (x->c_mainNotes->size() > 0 && velocity > 0){
            
            // refire if note is already in local and exit
            
            if (midiFilter_contains(x, *x->c_localNotes, pitch)){
                
                outlet_list(x->c_outlet, NULL, 2, argv);
                
                return;
                
            }
            
            //if pitch makes it through localMath output and save to lists
            //other localMath returns false and note is dropped
            
            else if (midiFilter_localMath(x, pitch)) {
                
                x->c_localNotes->push_back(pitch);
                x->c_mainNotes->push_back(pitch);
                outlet_list(x->c_outlet, NULL, 2, argv);
                
                return;
                
            }
            
        //if mainnotes exists and if note-off
            
        } else if (x->c_mainNotes->size() > 0 && velocity == 0) {
            
            //if localNotes contains pitch remove from lists and send note-off
            
            if (midiFilter_contains(x, *x->c_localNotes, pitch)){
                
                midiFilter_removeValue(x, *x->c_localNotes, pitch);
                midiFilter_removeValue(x, *x->c_mainNotes, pitch);
                outlet_list(x->c_outlet, NULL, 2, argv);
                
                return;
                
            }
            
        }
        
        
    }
//    systhread_mutex_lock(x->c_mutex);
//    for (int i=0; i<argc; i++) {
//        long value = atom_getlong(argv+i);
//        x->c_mainNotes->push_back(value);
//    }
//    systhread_mutex_unlock(x->c_mutex);
}

void midiFilter_list2(t_midiFilter *x, t_symbol *msg, long argc, t_atom *argv)
{
    if (argc > 0) {
        systhread_mutex_lock(x->c_mutex);
        
        long value = atom_getlong(argv);
        x->c_localNotes->push_back(value);
        
        systhread_mutex_unlock(x->c_mutex);
        
        post("midiFilter list2 called");
    }
}


void midiFilter_clear(t_midiFilter *x)
{
    systhread_mutex_lock(x->c_mutex);
    x->c_mainNotes->clear();
    x->c_localNotes->clear();
    systhread_mutex_unlock(x->c_mutex);
}

bool midiFilter_contains(t_midiFilter *x, numberVector &collection, long targetValue)
{
    numberIterator iter, begin, end;
    long value;
    bool found = false;

    // Lock the mutex to ensure thread safety
    systhread_mutex_lock(x->c_mutex);

    // Check if the collection is not empty
    if (!collection.empty()) {
        begin = collection.begin();
        end = collection.end();
        iter = begin;

        // Iterate through the collection to find the target value
        for (;;)
        {
            (*iter).getValue(value);
            if (value == targetValue) {
                found = true;
                break;
            }

            iter++;
            if (iter == end)
                break;
        }
    }

    // Unlock the mutex
    systhread_mutex_unlock(x->c_mutex);

    return found;
}

bool midiFilter_localMath(t_midiFilter *x, long value)
{
    numberIterator iter, begin, end;
    
    // true will signal pitch to conitue to output. false with drop the note
    
    bool response = true;
    long listValue;

    // Lock the mutex to ensure thread safety
    systhread_mutex_lock(x->c_mutex);

    // Check if the collection is not empty
    if (!x->c_localNotes->empty()) {
        begin = x->c_localNotes->begin();
        end = x->c_localNotes->end();
        iter = begin;

        // Iterate through the collection to find the target value
        for (;;)
        {
                    // Use the iterator to get the current element's value
                    (*iter).getValue(listValue);

                    // Apply your custom math logic
                    if (value - listValue == 1 || value - listValue == 2 ||
                        value - listValue == -1 || value - listValue == -2) {
                        response = false;
                        break;
                    }

                    iter++;
                    if (iter == end)
                        break;
                }
    }

    // Unlock the mutex
    systhread_mutex_unlock(x->c_mutex);

    return response;
}

void midiFilter_removeValue(t_midiFilter *x, numberVector &collection, long valueToRemove)
{
    // Lock the mutex to ensure thread safety
    systhread_mutex_lock(x->c_mutex);

    numberIterator iter = collection.begin();

    // Iterate through the collection to find the element
    while (iter != collection.end())
    {
        long currentValue;
        (*iter).getValue(currentValue);

        // If the value matches, erase the element
        if (currentValue == valueToRemove) {
            iter = collection.erase(iter); // Erase returns the next iterator
            break; // Stop after removing the first matching element
        } else {
            ++iter; // Move to the next element
        }
    }

    // Unlock the mutex
    systhread_mutex_unlock(x->c_mutex);
}

