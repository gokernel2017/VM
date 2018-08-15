//-------------------------------------------------------------------
//
// Recursive Function Example:
//
//-------------------------------------------------------------------
//
int i = 10;

function Hello ()
{
    i++;

    "Hello i: " i;

    if (i < 20) {

        Hello ();

    }

    "\nFunction 'Hello' Recursive Exiting.\n"
}

/*
    #ifdef USE_JIT
    "\nSummer mode: USE_JIT\n\n";
    #endif
    #ifdef USE_VM
    "\nSummer mode: USE_VM\n\n";
    #endif
*/

    Hello ();
