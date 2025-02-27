/**
    @file
    midi filter - patch midi filter
    Version 2!
    doug hirlinger - doughirlinger@gmail.com

    @ingroup    examples
*/

#include "ext.h"
#include "ext_obex.h"
#include "ext_strings.h"
#include "ext_common.h"
#include "ext_systhread.h"
#include <array>
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
typedef std::array<long, 2> numberArray;
typedef std::vector<numberArray> numberArrayVector;
typedef numberArrayVector::iterator numberArrayIterator;


// max object instance data
typedef struct _midiFilter2 {
    t_object            ob;
    numberVector        *m_mainNotes;    // note: you must store this as a pointer and not directly as a member of the object's struct
    numberVector        *m_localNotes;    // note: you must store this as a pointer and not directly as a member of the object's struct
    numberArrayVector   *m_reassignedNotes;
    void                *m_outlet;
    void                *m_outlet2;
    //t_systhread_mutex    m_mutex;
} t_midiFilter2;


// prototypes
void    *midiFilter2_new(t_symbol *s, long argc, t_atom *argv);
void    midiFilter2_free(t_midiFilter2 *x);
void    midiFilter2_assist(t_midiFilter2 *x, void *b, long m, long a, char *s);
void    midiFilter2_bang(t_midiFilter2 *x);
void    midiFilter2_printLocalNotes(t_midiFilter2 *x); //same as bang but for localNotes
void    midiFilter2_count(t_midiFilter2 *x);
void    midiFilter2_list(t_midiFilter2 *x, t_symbol *msg, long argc, t_atom *argv);
void    midiFilter2_externalMidi(t_midiFilter2 *x, t_symbol *msg, long argc, t_atom *argv);
void    midiFilter2_clear(t_midiFilter2 *x);
bool    midiFilter2_contains(t_midiFilter2 *x, numberVector &collection, long targetValue);
bool    midiFilter2_localMath(t_midiFilter2 *x, long value);
void    midiFilter2_removeValue(t_midiFilter2 *x, numberVector &collection, long valueToRemove);
long    midiFilter2_mainMath(t_midiFilter2 *x, long value);
long    midiFilter2_arrayContains(t_midiFilter2 *x, numberArrayVector &collection, long targetValue);
void    midiFilter2_removeValuesFromArray(t_midiFilter2 *x, numberArrayVector &collection, long valueToRemove);
void    midiFilter2_version();
void    midiFilter2_printReassigned(t_midiFilter2 *x);


// globals
static t_class    *s_midiFilter2_class = NULL;

/************************************************************************************/

void ext_main(void *r)
{
    t_class    *c = class_new("midiFilter2",
                           (method)midiFilter2_new,
                           (method)midiFilter2_free,
                           sizeof(t_midiFilter2),
                           (method)NULL,
                           A_GIMME,
                           0);

    class_addmethod(c, (method)midiFilter2_bang,    "bang",            0);
    class_addmethod(c, (method)midiFilter2_printLocalNotes,    "printLocalNotes",            0);
    class_addmethod(c, (method)midiFilter2_list,    "list",            A_GIMME,0);
    class_addmethod(c, (method)midiFilter2_externalMidi,    "externalMidi",            A_GIMME,0);
    class_addmethod(c, (method)midiFilter2_clear,    "clear",        0);
    class_addmethod(c, (method)midiFilter2_count,    "count",        0);
    class_addmethod(c, (method)midiFilter2_assist,    "assist",        A_CANT, 0);
    class_addmethod(c, (method)stdinletinfo,    "inletinfo",    A_CANT, 0);
    class_addmethod(c, (method)midiFilter2_contains, "int",      A_LONG, 0);
    class_addmethod(c, (method)midiFilter2_localMath, "int", A_LONG, 0);
    class_addmethod(c, (method)midiFilter2_removeValue, "int", A_LONG, 0);
    class_addmethod(c, (method)midiFilter2_mainMath, "int", A_LONG, 0);
    class_addmethod(c,(method)midiFilter2_arrayContains, "int", A_LONG, 0);
    class_addmethod(c, (method)midiFilter2_removeValuesFromArray, "int", A_LONG, 0);
    class_addmethod(c, (method)midiFilter2_version, "version", 0);
    class_addmethod(c, (method)midiFilter2_printReassigned, "printReassigned", 0);
    

    class_register(CLASS_BOX, c);
    s_midiFilter2_class = c;
    
    post("midiFilter2 object 2.3.7");
}


/************************************************************************************/
// Object Creation Method

void *midiFilter2_new(t_symbol *s, long argc, t_atom *argv)
{
    t_midiFilter2    *x;

    x = (t_midiFilter2 *)object_alloc(s_midiFilter2_class);
    if (x) {
       //systhread_mutex_new(&x->m_mutex, 0);
        x->m_outlet2 = outlet_new(x, NULL);
        x->m_outlet = outlet_new(x, NULL);
        x->m_mainNotes = new numberVector;
        x->m_mainNotes->reserve(220);
        midiFilter2_list(x, gensym("list"), argc, argv);
        x->m_localNotes = new numberVector;
        x->m_localNotes->reserve(22);
        x->m_reassignedNotes = new numberArrayVector;
        x->m_reassignedNotes->reserve(220);
    }
    return(x);
}


void midiFilter2_free(t_midiFilter2 *x)
{
    //systhread_mutex_free(x->m_mutex);
    delete x->m_mainNotes;
    delete x->m_localNotes;
}


/************************************************************************************/
// Methods bound to input/inlets

void midiFilter2_assist(t_midiFilter2 *x, void *b, long msg, long arg, char *dst)
{
    if (msg==1)
        strcpy(dst, "input");
    else if (msg==2)
        strcpy(dst, "output");
}



void midiFilter2_bang(t_midiFilter2 *x)
{
    numberIterator iter, begin, end;
    int i = 0;
    long ac = 0;
    t_atom *av = NULL;
    long value;

//    systhread_mutex_lock(x->m_mutex);
    ac = x->m_mainNotes->size();

    if (ac)
        av = new t_atom[ac];

    if (ac && av) {
        
        begin = x->m_mainNotes->begin();
        end = x->m_mainNotes->end();

        iter = begin;

        for (;;) {
            
            (*iter).getValue(value);
            atom_setlong(av+i, value);

            i++;
            iter++;

            if (iter == end)
                break;
        }
//        systhread_mutex_unlock(x->m_mutex);    // must unlock before calling _clear() or we will deadlock

        outlet_anything(x->m_outlet2, gensym("list"), ac, av); // don't want to call outlets in mutexes either

        delete[] av;
        
    }
    //else
//        systhread_mutex_unlock(x->m_mutex);
}

void midiFilter2_printLocalNotes(t_midiFilter2 *x)
{
    numberIterator iter, begin, end;
    int i = 0;
    long ac = 0;
    t_atom *av = NULL;
    long value;

//    systhread_mutex_lock(x->m_mutex);
    ac = x->m_localNotes->size();

    if (ac)
        av = new t_atom[ac];

    if (ac && av) {

        begin = x->m_localNotes->begin();
        end = x->m_localNotes->end();

        iter = begin;

        for (;;) {

            (*iter).getValue(value);
            atom_setlong(av+i, value);

            i++;
            iter++;

            if (iter == end)
                break;
        }
        //systhread_mutex_unlock(x->m_mutex);    // must unlock before calling _clear() or we will deadlock

//        DPOST("about to clear\n", ac);
//        midiFilter2_clear(x);

        outlet_anything(x->m_outlet2, gensym("list"), ac, av); // don't want to call outlets in mutexes either

        delete[] av;
    }
    //else
       // systhread_mutex_unlock(x->m_mutex);
}


void midiFilter2_count(t_midiFilter2 *x)
{
    outlet_int(x->m_outlet, x->m_mainNotes->size());
    outlet_int(x->m_outlet, x->m_localNotes->size());
}


void midiFilter2_list(t_midiFilter2 *x, t_symbol *msg, long argc, t_atom *argv)
{
    //if there's an incoming list
    
    //systhread_mutex_lock(x->m_mutex);
    
    if (argc > 0) {
        
        long pitch = atom_getlong(argv);
        long velocity = atom_getlong(argv+1);
        long beforeMain;
        long firstReturnedPitch;
        long reassignedPitch;
        
        //if incoming pitch is note-on and mainNotes is empty then play and add to lists
        
        if (x->m_mainNotes->size() < 1 && velocity > 0) {
            
            x->m_localNotes->push_back(pitch);
            x->m_mainNotes->push_back(pitch);
            
            
            //systhread_mutex_unlock(x->m_mutex);
            
            outlet_list(x->m_outlet, NULL, 2, argv);
            
            return;
            
        //otherwise if mainNotes does exists and is note-on...
            
        //systhread_mutex_lock(x->m_mutex);
            
        } else if (x->m_mainNotes->size() > 0 && velocity > 0){
            
            //systhread_mutex_unlock(x->m_mutex);
            
            // refire if note is already in local and exit
            
//            bool localMath = midiFilter2_localMath(x, pitch);
//            post("localMath %ld", localMath);
            
            if (midiFilter2_contains(x, *x->m_localNotes, pitch)){
                
                outlet_list(x->m_outlet, NULL, 2, argv);
                
                return;
                
            }
            
            //if pitch makes it through localMath pass to mainMath
            //otherwise localMath returns false and note is dropped
            
            else if (midiFilter2_localMath(x, pitch)) {
          
                beforeMain = pitch;
                pitch = midiFilter2_mainMath(x, pitch);
                
                //if mainMath returns same pitch play and add to lists
                 
                //systhread_mutex_unlock(x->m_mutex);
                
                if (beforeMain == pitch) {
                    
                    x->m_localNotes->push_back(pitch);
                    x->m_mainNotes->push_back(pitch);
                    
                    //systhread_mutex_unlock(x->m_mutex);
                    
                    outlet_list(x->m_outlet, NULL, 2, argv);
                    
                    return;
                    
                    //else run again with new pitch
                    
                } else {
                    
                    firstReturnedPitch = pitch;
                    pitch = midiFilter2_mainMath(x, pitch);
                   
                    //if pitch is unchanged second time play and add to lists
                    
                    if (firstReturnedPitch == pitch) {
                        
                        //systhread_mutex_lock(x->m_mutex);
                        
                        x->m_localNotes->push_back(pitch);
                        x->m_mainNotes->push_back(pitch);
                        x->m_reassignedNotes->push_back({beforeMain,pitch});
                        
                        //systhread_mutex_unlock(x->m_mutex);
                        
                        atom_setlong(argv, pitch);
                        
                        outlet_list(x->m_outlet, NULL, 2, argv);
                        
                        return;
                        
                        //else drop and exit
                        
                    } else {
                        
                        firstReturnedPitch = pitch;
                        pitch = midiFilter2_mainMath(x, pitch);
                        
                        if (firstReturnedPitch == pitch) {
                            
                            //systhread_mutex_lock(x->m_mutex);
                            
                            x->m_localNotes->push_back(pitch);
                            x->m_mainNotes->push_back(pitch);
                            x->m_reassignedNotes->push_back({beforeMain,pitch});
                            
                            //systhread_mutex_unlock(x->m_mutex);
                            
                            atom_setlong(argv, pitch);
                            
                            outlet_list(x->m_outlet, NULL, 2, argv);
                            
                            return;
                            
                        } else {
                            
                            return;
                            
                        }
                        
                    }
                        
                }
                
                return;
                
            }
            
        //if is note-off
            
        }
        
        if (velocity == 0) {
            
            //if reassignedNoters is empty, output note-off and remove from lists
            
            if(x->m_reassignedNotes->size() == 0){
                
                outlet_list(x->m_outlet, NULL, 2, argv);
                midiFilter2_removeValue(x, *x->m_localNotes, pitch);
                midiFilter2_removeValue(x, *x->m_mainNotes, pitch);
                
                return;
            }
            
            //if reassignedNotes contains pitch change output, output reassigned note-off and remove from lists
            
            if(midiFilter2_arrayContains(x, *x->m_reassignedNotes, pitch) != -1){
                
                reassignedPitch = midiFilter2_arrayContains(x, *x->m_reassignedNotes, pitch);
                atom_setlong(argv, reassignedPitch);
                
                outlet_list(x->m_outlet, NULL, 2, argv);
                
                //remove from reassignedNotes & other lists
                midiFilter2_removeValuesFromArray(x, *x->m_reassignedNotes, pitch);
                midiFilter2_removeValue(x, *x->m_mainNotes, reassignedPitch);
                midiFilter2_removeValue(x, *x->m_localNotes, reassignedPitch);
                
                return;
            }
            
            //otherwise if localNotes contains the pitch send note-off and remove
            
            if (midiFilter2_contains(x, *x->m_localNotes, pitch)){
                
                midiFilter2_removeValue(x, *x->m_localNotes, pitch);
                midiFilter2_removeValue(x, *x->m_mainNotes, pitch);
                
                
                outlet_list(x->m_outlet, NULL, 2, argv);
                
                return;
                
            }
            
        }
        
        
    }
//    systhread_mutex_lock(x->m_mutex);

//    systhread_mutex_unlock(x->m_mutex);
}

//for midi coming from other channels via send and receive objects

void midiFilter2_externalMidi(t_midiFilter2 *x, t_symbol *msg, long argc, t_atom *argv)
{
    if (argc > 0) {
        
        long pitch = atom_getlong(argv);
        long velocity = atom_getlong(argv+1);
        
        //if incoming pitch is note-on and mainNotes is empty then add to mainNotes
        
        if (velocity > 0){
            
            x->m_mainNotes->push_back(pitch);
            
        } else if (velocity == 0){
            
            midiFilter2_removeValue(x, *x->m_mainNotes, pitch);
            
        }
            
    }
}


void midiFilter2_clear(t_midiFilter2 *x)
{
    //systhread_mutex_lock(x->m_mutex);
    x->m_mainNotes->clear();
    x->m_localNotes->clear();
    x->m_reassignedNotes->clear();
    //systhread_mutex_unlock(x->m_mutex);
}

bool midiFilter2_contains(t_midiFilter2 *x, numberVector &collection, long targetValue)
{
    numberIterator iter, begin, end;
    long value;
    bool found = false;

    // Lock the mutex to ensure thread safety
    //systhread_mutex_lock(x->m_mutex);

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
    //systhread_mutex_unlock(x->m_mutex);

    return found;
}

//returns -1 if no arrays[0] contain note else returns origin value before reassignment
long midiFilter2_arrayContains(t_midiFilter2 *x, numberArrayVector &collection, long targetValue)
{
    long output = -1;
    
    if (!collection.empty()) {
        
        for (numberArrayIterator it = collection.begin(); it != collection.end(); ++it) {
            
            for(size_t j = 0; j < 2; ++j) {
                if ((*it)[0] == targetValue) {
                    output = (*it)[1];
                    return output;
                }
            }
            
        }
        
    }
    
    return output;
    
}

bool midiFilter2_localMath(t_midiFilter2 *x, long value)
{
    numberIterator iter, begin, end;
    
    // true will signal pitch to conitue to output. false with drop the note
    
    bool response = true;
    long listValue;

    // Lock the mutex to ensure thread safety
    //systhread_mutex_lock(x->m_mutex);

    // Check if the collection is not empty
    if (!x->m_localNotes->empty()) {
        begin = x->m_localNotes->begin();
        end = x->m_localNotes->end();
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
    //systhread_mutex_unlock(x->m_mutex);

    return response;
}

long midiFilter2_mainMath(t_midiFilter2 *x, long value)
{
    numberIterator iter, begin, end;
    long output = value; // Default output is the input value
    long listValue;

    if (!x->m_mainNotes->empty()) {
        begin = x->m_mainNotes->begin();
        end = x->m_mainNotes->end();
        iter = begin;

        for (; iter != end; ++iter) {
            (*iter).getValue(listValue);

            // Check for minor third or less
            if (value - listValue == 1) {
                output = listValue;
            } else if (value - listValue == 2) {
                output = listValue;
            } else if (value - listValue == -1) {
                output = listValue;
            } else if (value - listValue == -2) {
                output = listValue;
            }
        }
    }

    return output;
}



void midiFilter2_removeValue(t_midiFilter2 *x, numberVector &collection, long valueToRemove)
{
    // Lock the mutex to ensure thread safety
    //systhread_mutex_lock(x->m_mutex);

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
    //systhread_mutex_unlock(x->m_mutex);
}

void midiFilter2_removeValuesFromArray(t_midiFilter2 *x, numberArrayVector &collection, long valueToRemove)
{
    if (!collection.empty()) {
        for (numberArrayIterator it = collection.begin(); it != collection.end(); ) {
            if ((*it)[0] == valueToRemove) {
                it = collection.erase(it);  // Erase returns the next valid iterator
            } else {
                ++it;  // Only increment if no deletion occurs
            }
        }
    }
}


void midiFilter2_printReassigned(t_midiFilter2 *x)
{
    numberArrayIterator iter, begin, end;
    int i = 0;
    long ac = 0;
    t_atom *av = NULL;

    ac = x->m_reassignedNotes->size() * 2; // Each entry in m_reassignedNotes contains two values

    if (ac)
        av = new t_atom[ac];

    if (ac && av) {
        begin = x->m_reassignedNotes->begin();
        end = x->m_reassignedNotes->end();
        iter = begin;

        for (; iter != end; ++iter) {
            atom_setlong(av + i, (*iter)[0]); // First value
            atom_setlong(av + i + 1, (*iter)[1]); // Second value
            i += 2;
        }

        outlet_anything(x->m_outlet2, gensym("list"), ac, av); // Send list to outlet

        delete[] av;
    }
}
    


void midiFilter2_version()
{
    post("midiFilter2 object 2.3.7");
}

