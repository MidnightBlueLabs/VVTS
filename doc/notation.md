# State machine notation

A state machine essentially consists of _states_ and _transitions_ between them. When you are familiar with the notation, the next step is to study the provided [examples](https://github.com/MidnightBlueLabs/VVTS/blob/main/state_machines). In case the outcome of a statement is not clear from its name or the context, it can be looked up in the individual [components documentation](components.md).

## Comments
In order to increase readability, a comment may be present within a state machine definition file. They are intended to be read by a human and ignored by the execution engine. A comment is opened with `(*` and closed with `*)`, like so:
```
(* This is a comment *)
```

## States
A state is defined by the _state_ keyword, as follows:
```
state x:
```
where `x` denotes the name of the state, which can be used to refer to this state.

### Statements
A state may be coupled with any number of _statements_ that are executed in sequence once that particular state is entered, much like imperative programming:
```
state x:
  print("entering state x");
  exit();
```
In the example above, when entering state `x`, the builtin function `print` is invoked, with "entering state x", causing this to be printed on the screen. Finally, the builtin function `exit` is invoked, causing VVTS to terminate.

### Variables
Variables can be created, read, and written to by refering to them rather than an immediate value, like so:
```
state x:
  global_str = "entering state x";
  print(global_str);
  exit();
```
Here, variable `global_str` is created and assigned a string value `"entering state x"`. All variables are global variables and can hence be referenced from any state. Then, the builtin function `print` is invoked, this time with a variable as input, causing the value assigned to be printed to the screen. Finally, `exit` is invoked, once again causing VVTS to terminate.

### Variable types
Besides the string type, variables can also be of type _object_. Currently, three object types exist: [`accesspoint`](components/accesspoint.md), [`miscnet`](components/miscnet.md) and [`trafficmonitor`](components/trafficmonitor.md).
A new object of any of such types created by invoking their name as if it were a function. For example, a new `accesspoint` is created and stored in the `global_obj` variable like so:
```
state x:
  global_obj = accesspoint();
  exit();
```

Besides strings and objects, states themselves can also be referenced by their name, and they are treated as variables as well, like so:
```
state x:
  print(x);
  exit();
```

There are no designated integer or floating point variable types. Such values should be encoded as strings.

### Object methods and properties
Objects have certain _methods_ (functions) that can be invoked and may take (variable or fixed) inputs and may produce a return value. For example:
```
state x:
  mn = miscnet();
  resolved = mn.lookup_ipv4("google.com");
  exit();
```
Here, a new objects of type `miscnet` is created. Then, the `lookup_ipv4` method described in the [`miscnet` class documentation](components/miscnet.md) is invoked, taking an input of type string, and producing a string containing the IPv4 address of the domain name provided as input, in this case `google.com`. Finally, the result is stored in a variable labeled `resolved`, and VVTS is terminated.

Each object can have readable and/or writable _properties_. For example:
```
state x:
  ap = accesspoint();
  ap.interface = "wlan0";
  exit();
```
Which sets the `interface` property of an [`accesspoint`](components/accesspoint.md) object.

The available methods and properties for each object are documented in [the components documentation](components.md).

### Builtin functions
There are three builtin functions, `exit`, `print` and `concat`. Their functionality should be obvious from their name. All three functions take any number of input variables.

## Transitions
In order to perform a transition from one state to another, we rely on a method that take one or several states as an input. Subsequently, we must wait for the conditions specified in the documentation pertaining to the method in question to be satisfied, and then a state transition will automatically occur. For example:
```
state x:
  ap = accesspoint();
  ap.interface = "wlan0";
  ap.ipv4_addr = "192.168.123.1";
  ap.bring_up(ap_success, ap_err);

state ap_success:
  print("ap_success");
  exit();

state ap_err:
  print("ap_err");
  exit();
```
In the example above, the `bring_up` method found in the [`accesspoint`](components/accesspoint.md) component takes two arguments, the success and error state, respectively. When the accesspoint is successfully set up and operational, a state transition to the success state occurs, whereas a state transition to the error state occurs in case of an error.

## Initial state
Given that the state machine consists of states and transitions, exactly one of the states is the _initial state_, which is entered at the start of the execution of the state machine. The initial state is denoted through the `init` keyword, like so:
```
state init x:
  print("initial state");
  exit();
```

## Special variables
Every method that allows one to provide an error state as an argument, sets a human readable description of the error in the global `errstr` variable, so that it can subsequently be printed to the screen from the error state. For example:
```
state init x:
  ap = accesspoint();
  ap.interface = "wlan0";
  ap.ipv4_address = "192.168.123.1";
  ap.bring_up(ap_success, ap_err);

state ap_success:
  print("ap_success");
  exit();

state ap_err:
  print("ap_err: ", errstr);
  exit();
```
