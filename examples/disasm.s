
int i, a= 100, b = 300;

function loop () {

    i = 70;

    for (;;) {

        i; // print value i
        i++;

        if (i > 100) {
            break;
        }
    }

}

/*
function hello () {
    a = b;
    a++;
    a;
}
    disasm ("loop");
    //disasm ("hello");
*/
