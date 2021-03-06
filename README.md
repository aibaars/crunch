# crunch

Having looked at and messed a bit with testing systems such as Deja GNU and deciding I did not like them nor how they would force me to work, I went about building my own - one which coped with threads nicely and ensured a fresh
environment for each and every test in a test suit.

[![Build Status](https://travis-ci.org/DX-MON/crunch.svg?branch=master)](https://travis-ci.org/DX-MON/crunch)
[![codecov](https://codecov.io/gh/DX-MON/crunch/branch/master/graph/badge.svg)](https://codecov.io/gh/DX-MON/crunch)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/DX-MON/crunch.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/DX-MON/crunch/alerts/)
[![Coverity Status](https://scan.coverity.com/projects/20294/badge.svg)](https://scan.coverity.com/projects/dx-mon-crunch)

## Building crunch

Building crunch is as simple as a "make && sudo make install".
If you would like crunch to test itself, follow this up with a "make check".

For those who want Crunch for both 32- and 64-bit builds, the scripts build{32,64}.sh should suffice for most distros.
If you want to customise your build and installation further, there are some handy make variables provided. These are:
 *	PREFIX - same as the configure option "--prefix". Use this to point the library and tools at where you want them installed.
 *	LIBDIR - same as the configure option "--libdir". Use this to point the library portion at your prefered library directory.
 *	GCC - specify your compiler and any global compiler options with this. For example, for a 64-bit build: GCC="gcc -m64 -fPIC -DPIC".
The build system also accepts the PKG_CONFIG_PATH variable which is forwarded to pkg-config.

## Using crunch++

An example use of crunch++ follows:
```C++
#include <crunch++.h>

// The following names are arbitrary unless noted otherwise
class testSomething final : public testsuite
{
private:
	void testMe()
	{
		// Do something..
	}

public:
	// This is a mandatory function that allows you to
	// then register your individual tests to the framework
	void registerTests() final
	{
		CXX_TEST(testMe)
	}
};

// This export must exist and be named as you see here
// as this is what crunch++ looks for when it loads the
// testsuite in order to discover the tests
CRUNCHpp_TEST void registerCXXTests();
void registerCXXTests()
{
	registerTestClasses<testSomething>();
}
```

## Using crunch

Crunch has been designed to be as simple as possible to use and to not get in your way if you don't want it to.

A template program is given:
```C
#include <crunch.h>

/* Arbitrary function name.
 * The signature is all that is important - crunch expects all tests to be
 * a function returning nothing and taking no parameters
 */
void testMe()
{
}

BEGIN_REGISTER_TESTS()
	TEST(testMe)
END_REGISTER_TESTS()
```

A complete list of supported assertions is given in crunch.h

## Integrating crunch

crunchMake is designed to be as compatable as it needs to be with GCC and Clang.
This includes optionally specifying -o to set the output file name and location of the test.
If -o is not specified, then crunchMake will compute this for you by taking the input test name and changing the file extension

crunchMake's extra options are --log, --silent, -s, --quiet and -q.
The --silent and -s option makes crunchMake produce no output of any kind, only compiler standard output.
The --quiet and -q option makes crunchMake produce a kernel-Makefile style "CCLD" line and any compiler standard output.
Specifying neither give the default Makefile style output of the command that is to be run in full.

The --log option is provided so that all output can be redirected to a file rather than the terminal - including all compiler output on stdout.
The option does not allow logging of stderr though.

Specifying multiple source files on the crunchMake command line builds multiple tests libraries.

crunch takes the names of the test libraries (test suits) to run and automatically appends the platform's library sufix.
To run crunch's own tests, the Makefiles issue "crunch testCrunch".
Non-existant test libraries are warned about but the testing continued in the hopes that some of the named test suits exist.
Statistics are issued for the overall run rather than the individual suits.

## The License

As stated in the code, I have licensed the library using LGPL v3+.
Please report bugs to dx-mon@users.sourceforge.net

## Known Bugs

None
