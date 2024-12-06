/**
	@file
	midi filter - patch midi filter
	doug hirlinger - doughirlinger@gmail.com

	@ingroup	examples
*/

#include "ext.h"					// standard Max include, always required
#include "ext_obex.h"				// required for new style Max object

////////////////////////// object struct
typedef struct _midiFilter
{
	t_object	ob;				// the object itself (must be first)
    t_atom *i_argv;
    long i_argc;
} t_midiFilter;

///////////////////////// function prototypes
//// standard set
void *midiFilter_new();
void midiFilter_free(t_midiFilter *x);
void midiFilter_assist(t_midiFilter *x, void *b, long m, long a, char *s);
//// additional methods
//void midiFilter_bang(t_midiFilter *x); // incoming bang message
void midiFilter_printargs(t_midiFilter *x, t_symbol *s, long argc, t_atom *argv);



//////////////////////// global class pointer variable
void *midiFilter_class;

void ext_main(void *r)
{
	t_class *c;

	c = class_new("midiFilter", (method)midiFilter_new, (method)midiFilter_free, (long)sizeof(t_midiFilter), 0L, A_GIMME, 0);

//	class_addmethod(c, (method)midiFilter_bang,		"bang",			0);
	class_addmethod(c, (method)midiFilter_assist,		"assist",		A_CANT, 0);
    class_addmethod(c, (method)midiFilter_printargs, "list", A_GIMME, 0);
	
	class_register(CLASS_BOX, c);
    midiFilter_class = c;

	post("I am the midiFilter object");
}

void midiFilter_assist(t_midiFilter *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { //inlet
		sprintf(s, "I am inlet %ld", a);
	}
	else {	// outlet
		sprintf(s, "I am outlet %ld", a);
	}
}

void midiFilter_printargs(t_midiFilter *x, t_symbol *s, long argc, t_atom *argv)
{
    long pitch;
    long vel;
    
    pitch = atom_getlong(argv);
    vel = atom_getlong(argv+1);
    
    post("message selector is %s",s->s_name);
    post("there are %ld arguments",argc);
    post("pitch %ld", pitch);
    post("vel %ld", vel);

}

void midiFilter_free(t_midiFilter *x)
{
    if (x->i_argv)
        sysmem_freeptr(x->i_argv);// FREE ALLOCED MEMORY
}

void *midiFilter_new()
{
    t_midiFilter *x = (t_midiFilter *)object_alloc(midiFilter_class);
    
    return x;
    
}


