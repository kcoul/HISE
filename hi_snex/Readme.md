![](snex.png)

Version: 1.0 alpha

# Scriptnode Expression Language

The Scriptnode Expression Language (**SNEX**) is a simplified subset of the C language family and is used throughout scriptnode for customization behaviour.

- **fast**. The assembly code generated by the compiler should (more or less) match the performance of C++ compiled code. 
- **easy to use**. The syntax can be adapted within minutes for people who know C++ and / or Javascript.
- **safe** - it's impossible to crash the application. Every call that could lead to a access violation is wrapped into a safe check

It's intended use is fast execution of expressions for signal processing algorithms. Unlike the HiseScript language, which is interpreted, the scriptnode expressions are JIT compiled and run in almost native speed (several orders of magnitude above HiseScript performance). When the scriptnode graph is exported as Cpp code, the expressions will be then compiled by the standard compiler (which is why it needs to be a strict subset of C / C++).

The JIT compiler uses the awesome `asmjit` library to emit the assembly instructions. Currently supported are macOS / Windows / Linux, however only 64bit is supported (32bit is pretty much deprecated by now). iOS support is not possible due to the security restriction that prevents allocation of executable memory.

There are two places where it can be used: in the `core.jit` node, which allows creating fully customizable nodes and at the connection between parameters (or modulation sources) via the `Expression` property which can be used to convert the value send to the connection target.

## Getting started

The easiest way to get to know the language is to use the SNEX Playground which offers you a JIT compiler, a code editor with a predefined snippet and shows the assembly output that is fed directly to the CPU. It also uses the same callbacks as the `core.jit` node and offers various test signals that you can use to check your algorithm.

### Callbacks

The SNEX code will be compiled to functions that are executed on certain events (similar to HiseScript). There are four callback types:

#### The `prepare()` callback

```cpp
void prepare(double samplerate, int blocksize, int numChannels)
```

The `prepare` callback will be executed when the node is initialised or when some of the processing specifications (samplerate, blocksize or channel amount) change. You should use this function to setup your variables according to the specifications - calculate frequency coefficients etc.

#### The `reset()` callback.

```cpp
void reset()
```

The `reset()` callback will be called whenever the processing pipeline needs to be resetted. This may occur under various circumstances and should be as fast as possible (unlike the prepare callback which happens very rarely):

- when a voice is started
- when the node is unbypassed
- after the `prepare` function was called

You should use this function to clear any state variable (eg. last values or counters for oscillators).

#### The `setX` callbacks

```cpp
void setX(double newValue)
```

Whenever you want to change a parameter from the outside, you can write a function that starts with `set`and it will automatically create a scriptnode parameter with the same name (so `setGain` will create a parameter called **Gain**)

#### The `processX` callbacks

These callbacks will be executed to calculate the audio signal. There are three different callbacks available, each one will be more suitable than the others for certain types of applications.

```cpp
// channel processing: one block of audio data at the time
void processChannel(block input, int numChannel)

// Monophonic (or stateless multichannel) processing: One sample in, one sample out
float processSample(float input)

// frame-based processing: all samples of every channel at the time
void processFrame(block frame)
```

The decision which callback you want to use depends on the algorithm: for stereo-effects, the interleaved processing of the `processFrame` callback is most suitable, but is slower than the `processSample` or the `processChannel` callbacks, which should be preferred for monophonic signal calculation.

> Be aware that if the `core.jit` node is wrapped in a `framex_block` node, the `processFrame` node will yield the best performance because it directly maps to the functionality of the surrounding container.

The `core.jit` node will pick the best function for the given context: block based processing or frame-based processing by wrapping it into a `framex_block` container node.

**Block based processing**: This is the default and means that the stream of audio samples is divided into chunks with a fixed length. In this case the functions are chosen in this order:

1. `processChannel()`
2. `processFrame()`
3. `processSample()`

The `processFrame` method will be always be prioritised over the `processSample` method because once you define it, it's highly likely that you want to perform some kind of interleaved algorithm.

The fastest operation mode is of course using the `processChannel` method with a `for` iterator, however using the `processFrame` method should be equally fast as using a `framex_block` method.

**Frame based processing**: Some DSP algorithms need to have all samples of each channel available at the same time. The simplest example would be a stereo-to mono converter, which just add the channels:

```cpp
l = l + r;
r = l;
```

This is known as interleaved (or frame-based) processing and can be achieved by wrapping any node into a `framex_block` container. In this case, the callbacks will be chosen in this order:

1. `processFrame()`
2. `processSample()`
3. `processChannel()`

Be aware that while the `processChannel` function is still used as last resort, it yields the worst performance, since it will create a buffer with the length 1 for each sample in each channel and then call the function.

The `core.jit` node will show which `processX` function is currently in use (along with the CPU usage), so you can compare the performances.

## Language Reference

This is the complete language reference. Features that deviate from C / Cpp are **emphasized**

The **SNEX** syntax uses brackets for blocks and semicolons for statements. Comment lines start with `//`, multiline comments with `/** Comment */)`

```cpp

/** This is a 
    multiline comment
*/
{
    x; // a statement
}
```

You can define variables in any scope (function scope or global scope), and variables in inner scopes override variables of the outer scope.

## Language structure

A valid SNEX code consists of definitions of variables, functions and more complex data structures (spans and structs):

```cpp
// some variables
type a = something;
type b = somethingElse;
...

// function definitions
type functionName()
{
    //... function body
}
```


## Variables

Variable names must be a valid identifier, and **definitions must initialise the value**:

```cpp
type variableName = initialValue;
```

You can also define constant values by prepending `const` to the definition:

```cpp
const type value = initialValue;
```

Doing so will speed up the execution because it doesn't need to lookup the memory location.

SNEX supports the `auto` keyword, which will use the type from the assigned literal:

```cpp
auto x = 12; // x is int
auto y = 12.0f; // y is float
auto z = Math.sin(2.0); // z is double;
```

## Functions


Function definitions have this syntax:

```cpp
ReturnType functionName(ArgumentType1 arg1, ArgumentType2 arg2)
{
   // body
}
```

Functions can be overloaded: despite having the same name, their argument amount and types can vary. However they can't differ only in the return type.

## Native Types

Unlike HiseScript, **SNEX** is strictly typed. **However there is a very limited set of available types**:

- `int` - Integer numbers
- `float` - single precision floating point numbers, marked with a trailing `f`: `2.012f`
- `double`- double precision floating point numbers
- `bool` - boolean values (just as intermediate type for expression results)
- `block` - a wrapper around a preallocated buffer of float numbers
- `event` - the HiseEvent

**There is no String type**. Conversion between the types is done via a C-style cast:

```cpp
// converts a float to an int
int x = (int)2.0f;
```

Type mismatches will be implicitely corrected by the compiler if possible (but it will produce a warning message so it's not recommended behaviour to just don't care about types).

## Complex Types

In addition to the native types, there are also complex types that can be used to create more complex data structure:

### Span

A span is a one-dimensional array with a number of elements which are known at compile time. The definition follows this syntax, specifying the data type, the size and an optional initialisation list:

```
span<type, size> name [= { initialisers }];
```

The type of the span can be any native type as well as complex types. This allows nesting of spans to create multidimensional arrays:

```
span<span<int, 3>, 3> matrix3x3;
```

Accessing the content of a span can be done by using the subscript-operator `[]`, which is a zero based integer index:

```
span<float, 3> data = { 1.0f, 2.0f, 3.0f };

// Usage:

data[1] = 3.0f; // sets the second element to 3.0f;
data[3] = 0.0f; // ERROR: out of bounds (trying to write element number 4)...
```

Be aware that the []-operator on spans is completely safe: If you use a constant value for the subscription index, it will check at compile time that it's within the bounds of the span (so the example above will not compile). However be aware that you can't use a non-constant value to access span elements because it might cause an out of bounds error:

```
span<float, 3> data = { 1.0f, 2.0f, 3.0f };

// Usage:

int x = 2;
data[x] = 2.0f; // OK
x = 3;
data[x] = 2.0f; // ERROR: out of bounds
```

> There is of course the option of accessing span elements dynamically, but you have to use a `wrap` object for the index.

### Wrap

The `wrap` type is an integer that is guaranteed to stay within the bounds specified at the definition:

```
wrap<8> index = { 1 };  // initialises to 1 
                        // (if you omit the list, it will be zero by default)

int x = index * 2 + index; // x = 3 // you can use it just like any other integer variable

index = 9;              // assigning a value will wrap around the specified limit
                        // (in this case it will have the value 1 because 9 % 8 == 1)

index = 7;
index++;                // incrementing the index will also wrap around
                        // also this is faster than assigning a value because it doesn't 
                        // need to calculate the modulo.

span<int, 8> data = {1,2,3,4,5,6,7,8}; // let's define a span now with 8 elements

data[++index] = 19;       // now you can use a dynamic index for the span []-operator...
data[++index] = 20;       // now you can use a dynamic index for the span []-operator...
```

Of course the wrap size must be equal or less than the size of the span, or it will again throw a compilation error:

```
wrap<124> index;

span<int, 4> data = { 1, 2, 3, 4};

index = 5; // will not wrap around;

data[index] = 5; // ERROR: out of bounds...
```

> If the wrap size is a power of two, the compiler will optimize the modulo operation to a simple bit shift operation, which might be noticeably faster, so if you have the choice, pick a power of two.

### Structs

A `struct` combines multiple other data types (including other complex data types) and can have functions that operate on instances of these structs. This allows a very basic object-oriented programming approach.

```
struct X
{
    int m1 = 5;
    int m2 = 4;
    float data = 1.0f;
    span<int, 3> list = {1, 2, 3}

    int getAll()
    {
        return m1 + m2 + (int)data + list[0] + list[1] + list[2];
    }
};

X obj1;

// Usage:


obj1.getAll(); // returns 18

obj1.m2 = 6;
obj1.getAll(); // returns 16
```

Definitions of a struct must always be concluded by a `;` (unlike function definitions). You can access the data members and call functions on a struct instance using the dot operator.

> there is no public / private concept with structs at the moment, but it might be added in the future.

You can also create spans of structs and call them in a loop:

```
struct FX
{
    void process(block b)
    {
        // do something;
    }
};

static const int NumChannels = 2;       // must be a compile time constant value

using StereoFX = span<FX, NumChannels>; // define an alias for a multichannel pair of FX
span<StereoFX, 4> chain;                // Creates a 2D array of FX objects
wrap<NumChannels> cIndex;               // used for the channelIndex later...

void process(channel b, int channelIndex)
{
    cIndex = channelIndex;

    for(auto& fx: chain[cIndex])
        fx.process(b);
}
```
## Type aliases & compile time constant values

If you create complex structures of nested data types, the syntax will quickly become a bit hard to read.

An array of multichannel frames and it's index might be defined like this:

```
span<span<float, 4>, 128> data1;
span<span<float, 4>, 128> data2;
wrap<4> index1;
wrap<4> index2;
```

You can enhance the readability by using type aliases and compile time constants for the numbers (also you increase the reusablility of the code):

```
static const NumChannels = 4; // define a constant for the channel amount
static const BlockSize = 128; // define a constant for the block size

using Frame = span<float, NumChannels>; // define Frame as span of 4 floats
using FrameBlock = span<Frame, BlockSize>;
using FrameIndex = wrap<NumChannels>;

// Usage:
FrameBlock data1;
FrameBlock data2;

FrameIndex index1;
FrameIndex index2;
```

## Variable visibility

**SNEX** variables are visible inside their scope (= `{...}` block) or parent scopes. The inner scope has the highest priority and override variable names is possible:

```cpp
void test()
{
    float x = 25.0f;
    
    {
        float x = 90.0f;
        Console.print(x); // 90.0f;
    }
    
    Console.print(x); // 25.0f;
}
```

However, since this is a common pitfall for bugs, it will produce a compiler warning.

## Operators

### Binary Operations

The usual binary operators are available: 

```cpp
a + b; // Add
a - b; // Subtract
a * b; // Multiply
a / b; // Divide
a % b; // Modulo (only with integer type)

a++; // post-increment
++a; // pre-increment (faster!)

!a; // Logical inversion
a == b; // equality
a != b; // inequality
a > b; // greater than
a >= b; // greater or equal
// ...

```

The rules of operator precedence are similar to every other programming language on the planet. You can use parenthesis to change the order of execution:

```cpp
(a + b) * c; // = a * c + b * c
```

Logical operators are short-circuited, which means that the second branch of a `&&` operator will not be evaluated if the first branch is false:

```cpp
int f1()
{
    Console.print(52.0f);
    return 0;
}

int f2()
{
    Console.print(12.0f);
    return 0;
}

void test()
{
    int c = f1() && f2() ? 2 : 1;
    Console.print(c);
}

// Will print 52 and 1 (f2 will not be executed)
```

### Assignment

Assigning a value to a variable is done via the `=` operator. Other assignment types are supported:

```cpp
int x = 12;
x += 3; // x == 15
x /= 3 // x == 5
```

You can access elements of a `block` via the `[]` operator:

```cpp
block b;
b[12] = 12.0f;
```

**There is an out-of-bounds check that prevents read access violations** if you use an invalid array index. This comes with some performance overhead, which can be deactivated using a compiler flag.

### Ternary Operator

Simple branching inside an expression can be done via the ternary operator:

```cpp
a ? b : c
```

The false branch will not be evaluated.

### if / else-if branching

Conditional execution of entire code blocks is possible using the `if` / `else` keywords:

```cpp
if(condition)
{
    // first case
}
else if (otherCondition)
{
    // second case
    return;
}
else
{
    // fallback code
}

// Some other code (will not be executed if otherCondition was true)
```

### Function calls

You can call other functions using this syntax: `functionCall(parameter1, parameter2);`Be aware that forward declaring is not supported so you can't call functions before defining them:

```cpp
void f1()
{
    f2(); // won't work
}

void f1()
{
    doSomething();
}

void f3()
{
    f1(); // now you can call it
}
```

### Return statement

Functions that have a return type need a return statement at the end of their function body:

```cpp
void f1()
{
    // Do something
    return; // this is optional
}

int f2()
{
    return 42; // must return a int
}
```

### Loops

There are no "classic" loops in SNEX (like `while` and `for`, but you can iterate over `span` and `block` data using the the range-based `for` loop syntax of C++:

```cpp
double uptime = 0.0;

for(auto& sample: block)
{
    sample = (float)Math.sin(uptime);
    uptime += 0.002;
}


span<int, 12> data;

for(int& d: data)
    d += 5;
```

The `&` token indicates that the iterator variable is a reference to the actual data, so assigning a value will write it into the memory location. If you don't need to change the value, you might want to omit the `&` token. In this case, the iterator variable will be copied from the original source and won't be written back, which saves a few CPU cycles.

### API classes

There are a few inbuilt API classes that offer additional helper functions.

- the `Math` class which contains a set of mathematical functions
- the `Console` class for printing a value to the console
- the `Message` class which contains methods to operate on a HiseEvent.

The syntax for calling the API functions is the same as in HiseScript: `Api.function()`.

```cpp
float x = Math.sin(2.0f);
```

> The `Math` class contains overloaded functions for `double` and `float`, so be aware of implicit casts here.


## Embedding the language

Embedding the language in a C++ project is pretty simple:

```cpp

// Create a global scope that contains global variables.
snex::jit::GlobalScope pool;

// Create a compiler that turns a String into a function pointer.
snex::jit::Compiler compiler(pool);

// The SNEX code to be parsed - Check the language reference below
juce::String code = "float member = 8.0f; float square(float input){ member = input; return input * input; }";

// Compiles and returns a object that contains the function code and slots for class variables.
if (auto obj = compiler.compileJitObject(code))
{
    // Returns a wrapper around the function with the given name
    auto f = obj["square"];
    
    // Returns a reference to the variable slot `member`
    auto ptr = obj.getVariablePtr("member");

    DBG(ptr->toFloat()); // 8.0f

    // call the function - the return type has to be passed in via template.
    // It checks that the function signature matches 
    // and the JIT function was compiled correctly.
    auto returnValue = f.call<float>(12.0f);
    
    DBG(returnValue); // 144.0f
    DBG(ptr->toFloat()); // 12.0f
}
else
{
    DBG(compiler.getErrorMessage());
}
```

## Examples

These examples show some basic DSP algorithms and how they are implemented in SNEX. In order to use it, just load the given HISE Snippets into the latest version of HISE and play around.

### Basic Sine Synthesiser

HISE automatically supports polyphony when 

```cpp
// we initialise it to a weird value, will get corrected later
double sr = 0.0;

// the counter for the signal generation
double uptime = 0.0;

// the increment value (will control the frequency)
double delta = 0.0;



void prepare(double sampleRate, int blockSize, int numChannels)
{
    // set the samplerate for the frequency calculation
    sr = sampleRate;
}

void reset()
{
    // When we start a new voice, we just need to reset the counter
    uptime = 0.0;
}

void handleEvent(event e)
{
    // get the frequency (in Hz) from the event
    const double cyclesPerSecond = e.getFrequency();

    // calculate the increment per sample
    const double cyclesPerSample = cyclesPerSecond / sr;
    
    // multiyply it with 2*PI to get the increment value
    delta = 2.0 * 3.14159265359 * cyclesPerSample;
}

void processFrame(block frame)
{
    // Calculate the signal
    frame[0] = (float)Math.sin(uptime);

    // Increment the value
    uptime += delta;

    // Copy the signal to the right channel
    frame[1] = frame[0];
}
```

```cpp
/** Initialise the processing here. */
void prepare(double sampleRate, int blockSize, int numChannels)
{
    x.prepare(sampleRate, 500.0);
}

/** Reset the processing pipeline */
void reset()
{
    x.reset(0.0);
    x.set(1.0);
}

sdouble x = 1.0;

float processSample(float input)
{
    return (float)x.next();
}



```