/*
 * This file is part of crunch
 * Copyright © 2013 Rachel Mant (dx-mon@users.sourceforge.net)
 *
 * crunch is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * crunch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "Core.h"
#include "Logger.h"
#include "ArgsParser.h"
#include "StringFuncs.h"
#include "crunch++.h"
#ifndef _MSC_VER
#include <dlfcn.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <io.h>
#define RTLD_LAZY 0
#define dlopen(fileName, flag) (void *)LoadLibrary(fileName)
#define dlsym(handle, symbol) GetProcAddress(HMODULE(handle), symbol)
#define dlclose(handle) FreeLibrary(HMODULE(handle))

char *dlerror()
{
	const auto error = GetLastError();
	char *message;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, MAKELANGID(LANG_NEUTRAL,
		SUBLANG_DEFAULT), reinterpret_cast<char *>(&message), 0, nullptr);
	return message;
}

#endif
#include <exception>
#include <stdlib.h>

using namespace std;

const arg_t args[] =
{
	{"--log", 1, 1, 0},
	{{}, 0, 0, 0}
};

#ifdef _MSC_VER
#define LIBEXT "tlib"
#else
#define LIBEXT "so"
#endif

parsedArgs_t parsedArgs;
parsedArgs_t namedTests;
uint32_t numTests = 0;
const char *cwd = nullptr;

typedef void (__cdecl *registerFn)();

void newline()
{
	if (isTTY)
#ifdef _MSC_VER
	{
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		testPrintf("\n");
	}
#else
		testPrintf(NEWLINE);
#endif
	else
		testPrintf("\n");
}

void red()
{
	if (isTTY)
#ifndef _MSC_VER
		testPrintf(FAILURE);
#else
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
#endif
}

void magenta()
{
	if (isTTY)
#ifndef _MSC_VER
		testPrintf(COLOUR("1;35"));
#else
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
}

void printStats()
{
	uint64_t total = passes + failures;
	testPrintf("Total tests: %u,  Failures: %u,  Pass rate: ", total, failures);
	if (total == 0)
		testPrintf("--\n");
	else
		testPrintf("%0.2f%%\n", double(passes) / double(total) * 100.0);
}

bool getTests()
{
	uint32_t i, j, n;
	for (n = 0; parsedArgs[n] != nullptr; n++)
		continue;
	namedTests = makeUnique<constParsedArg_t []>(n + 1);
	if (!namedTests)
		return false;

	for (j = 0, i = 0; i < n; i++)
	{
		// this might be as simple as (!parsedArgs[i]->minLength) now
		if (!findArgInArgs(parsedArgs[i]->value.data()))
		{
			namedTests[j] = parsedArgs[i];
			j++;
		}
	}
	if (j == 0)
		return false;
	parsedArgs_t tests = makeUnique<constParsedArg_t []>(j + 1);
	if (!tests)
		return false;
	std::copy(namedTests.get(), namedTests.get() + j, tests.get());
	namedTests = std::move(tests);
	namedTests[j] = nullptr;
	numTests = j;
	return true;
}

bool tryRegistration(void *testSuit)
{
	registerFn registerTests;
	registerTests = (registerFn)dlsym(testSuit, "registerCXXTests");
	if (!registerTests)
	{
		dlclose(testSuit);
		return false;
	}
	registerTests();
	return true;
}

void runTests()
{
	uint32_t i;
	testLog *logFile = nullptr;

	constParsedArg_t logging = findArg(parsedArgs, "--log", nullptr);
	if (logging)
	{
		logFile = startLogging(logging->params[0].data());
		loggingTests = true;
	}

	for (i = 0; i < numTests; i++)
	{
		auto testLib = formatString("%s/%s." LIBEXT, cwd, namedTests[i]->value.data());
		void *testSuit = dlopen(testLib.get(), RTLD_LAZY);
		if (!testSuit || !tryRegistration(testSuit))
		{
			if (!testSuit)
			{
				red();
				testPrintf("Could not open test library: %s", dlerror());
				newline();
			}
			red();
			testPrintf("Test library %s was not a valid library, skipping", namedTests[i]->value.data());
			newline();
			continue;
		}
		magenta();
		testPrintf("Running test suit %s...", namedTests[i]->value.data());
		newline();

		for (auto &test : cxxTests)
		{
			magenta();
			testPrintf("Running tests in class %s...", test.testClassName);
			newline();

			try { test.testClass->registerTests(); }
			catch (threadExit_t &)
			{
				delete test.testClass;
				continue;
			}

			try
				{ test.testClass->test(); }
			catch (threadExit_t &)
			{
				delete test.testClass;
				cxxTests.clear();
				printStats();
				if (logging != nullptr)
					stopLogging(logFile);
				throw;
			}
			delete test.testClass;
		}
		cxxTests.clear();
	}

	printStats();
	if (logging != nullptr)
		stopLogging(logFile);
}

int main(int argc, char **argv)
{
	registerArgs(args);
	parsedArgs = parseArguments(argc, argv);
	if (!parsedArgs || !getTests())
	{
		testPrintf("Fatal error: There are no tests to run given on the command line!\n");
		return 2;
	}
	cwd = getcwd(nullptr, 0);
#ifndef _MSC_VER
	isTTY = isatty(STDOUT_FILENO);
#else
	console = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!console)
	{
		printf("Error: could not grab console!");
		return 1;
	}
	isTTY = bool(isatty(fileno(stdout)));
#endif
	try { runTests(); }
	catch (threadExit_t &val)
	{
		free((void *)cwd);
		return val;
	}
	free((void *)cwd);
	return failures ? 1 : 0;
}
