

int i, a = 10, b = 20, c = 3, d = 5, x;

/*
function test (WIDGET o) {
function test (OBJECT o) {
  a = o->x;
}
*/

function add_a (arg1, arg2) {
    "Function add_a:\n"
    "Variable a VALUE: " a;

    a = arg1 + arg2;

    "arg1: "    arg1;
    "arg2: "    arg2;
    "value a: " a;
}

function add_b (arg1, arg2) {
    b = arg1 + arg2;
    "\nFunction(add_b) ... variable b: " b;
}


  "\nSimple Function Example:\n"

  //-------------------------
  // display a := 10;
  // display b := 20;
  //-------------------------
  "\n"
  "Value a: " a;
  "Value b: " b;
  "\n"


  //-------------------------
  // CALL THE FUNCTIONS
  //-------------------------
  //
  add_a (150, 50);  // a : 200;
  add_b (55, 35);   // b : 90;

  //-------------------------
  // display a := 200;
  // display b := 90;
  //-------------------------
  //
  "\n"
  "Value a: " a;
  "Value b: " b;
  "\n"
