/*
 * This file is part of crunch
 * Copyright © 2013-2019 Rachel Mant (dx-mon@users.sourceforge.net)
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

#include "threading/threadShim.h"
#include <stdio.h>
#include "Core.h"
#include "Logger.h"
#include "ArgsParser.h"
#include "StringFuncs.h"
#include "Memory.h"
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
	const DWORD error = GetLastError();
	char *message;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL,
		SUBLANG_DEFAULT), &message, 0, NULL);
	return message;
}

#endif

const arg_t crunchArgs[] =
{
	{"--log", 1, 1, 0},
	{NULL, 0, 0, 0}
};

#ifdef _MSC_VER
#define LIBEXT "tlib"
#else
#define LIBEXT "so"
#endif

parsedArgs_t parsedArgs = NULL;
parsedArgs_t namedTests = NULL;
uint32_t numTests = 0;
const char *workingDir = NULL;
uint8_t loggingTests = 0;

typedef void (__cdecl *registerFn)();

void newline()
{
#ifdef _MSC_VER
	SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	testPrintf("\n");
#else
	testPrintf(NEWLINE);
#endif
}

int testRunner(void *testPtr)
{
	test *theTest = testPtr;
	if (isTTY != 0)
#ifndef _MSC_VER
		testPrintf(INFO);
#else
		SetConsoleTextAttribute(console, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
	testPrintf("%s...", theTest->testName);
	if (isTTY != 0)
#ifndef _MSC_VER
		testPrintf(NEWLINE);
#else
		newline();
#endif
	else
		testPrintf(" ");
	theTest->testFunc();
	// Did the test switch logging on?
	if (!loggingTests && logger)
		// Yes, switch it back off again
		stopLogging(logger);
	logResult(RESULT_SUCCESS, "");
	return THREAD_SUCCESS;
}

void printStats()
{
	uint64_t total = passes + failures;
	testPrintf("Total tests: %u,  Failures: %u,  Pass rate: ", total, failures);
	if (total == 0)
		testPrintf("--\n");
	else
		testPrintf("%0.2f%%\n", ((double)passes) / ((double)total) * 100.0);
}

uint8_t getTests()
{
	uint32_t i, j, n;
	for (n = 0; parsedArgs[n] != NULL; n++);
	namedTests = testMalloc(sizeof(constParsedArg_t) * (n + 1));

	for (j = 0, i = 0; i < n; i++)
	{
		if (findArgInArgs(parsedArgs[i]->value) == NULL)
		{
			namedTests[j] = parsedArgs[i];
			j++;
		}
	}
	if (j == 0)
	{
		free(namedTests);
		return FALSE;
	}
	else
	{
		namedTests = testRealloc(namedTests, sizeof(parsedArg_t *) * (j + 1));
		numTests = j;
		return TRUE;
	}
}

uint8_t tryRegistration(void *testSuit)
{
	registerFn registerTests;
	registerTests = dlsym(testSuit, "registerTests");
	if (registerTests == NULL)
	{
		dlclose(testSuit);
		return FALSE;
	}
	registerTests();
	return TRUE;
}

int runTests()
{
	uint32_t i;
	test *currTest;
	testLog *logFile = NULL;

	constParsedArg_t logging = findArg(parsedArgs, "--log", NULL);
	if (logging)
	{
		logFile = startLogging(logging->params[0]);
		loggingTests = 1;
	}

	for (i = 0; i < numTests; i++)
	{
		char *testLib = formatString("%s/%s." LIBEXT, workingDir, namedTests[i]->value);
		void *testSuit = dlopen(testLib, RTLD_LAZY);
		free(testLib);
		if (testSuit == NULL || tryRegistration(testSuit) == FALSE)
		{
			if (testSuit == NULL)
			{
				if (isTTY != 0)
#ifndef _MSC_VER
					testPrintf(FAILURE);
#else
					SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
#endif
				testPrintf("Could not open test library: %s", dlerror());
				if (isTTY != 0)
					newline();
				else
					testPrintf("\n");
			}
			if (isTTY != 0)
#ifndef _MSC_VER
				testPrintf(FAILURE);
#else
				SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
#endif
			testPrintf("Test library %s was not a valid library, skipping", namedTests[i]->value);
			if (isTTY != 0)
				newline();
			else
				testPrintf("\n");
			continue;
		}
		if (isTTY != 0)
#ifndef _MSC_VER
			testPrintf(COLOUR("1;35"));
#else
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
		testPrintf("Running test suit %s...", namedTests[i]->value);
		if (isTTY != 0)
			newline();
		else
			testPrintf("\n");
		currTest = tests;
		while (currTest->testFunc)
		{
			int retVal = THREAD_ABORT;
			thrd_t testThread;
			thrd_create(&testThread, testRunner, currTest);
			thrd_join(testThread, &retVal);
			if (retVal != THREAD_SUCCESS)
				return retVal;
			++currTest;
		}
	}

	printStats();
	if (logging)
		stopLogging(logFile);
	return THREAD_SUCCESS;
}

#ifdef _WINDOWS
void invalidHandler(const wchar_t *expr, const wchar_t *func, const wchar_t *file,
	const uint32_t line, const uintptr_t res) { }
#endif

void callFreeParsedArgs() { parsedArgs = freeParsedArgs(parsedArgs); }

int main(int argc, char **argv)
{
#ifdef _WINDOWS
	_set_invalid_parameter_handler(invalidHandler);
	_CrtSetReportMode(_CRT_ASSERT, 0);
#endif
	registerArgs(crunchArgs);
	parsedArgs = parseArguments(argc, (const char **)argv);
	if (parsedArgs == NULL || getTests() == FALSE)
	{
		callFreeParsedArgs();
		testPrintf("Fatal error: There are no tests to run given on the command line!\n");
		return 2;
	}
	workingDir = getcwd(NULL, 0);
#ifndef _MSC_VER
	isTTY = isatty(STDOUT_FILENO);
#else
	console = GetStdHandle(STD_OUTPUT_HANDLE);
	if (console == NULL)
	{
		printf("Error: could not grab console!");
		return 1;
	}
	isTTY = isatty(fileno(stdout));
#endif
	const int result = runTests();
	free(namedTests);
	free((void *)workingDir);
	callFreeParsedArgs();
	if (result != THREAD_SUCCESS)
		return result == THREAD_ERROR ? 0 : 2;
	return failures == 0 ? 0 : 1;
}
