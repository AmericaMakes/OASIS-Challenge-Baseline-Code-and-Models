/*============================================================//
Copyright (c) 2020 America Makes
All rights reserved
Created under ALSAM project 3024

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================*/

/*============================================================//
errorChecks.h defines the data structures and the functions needed
to evaluate Excel configuration files in errorChecks.cpp
//============================================================*/

#pragma once

#include <iostream>
#include <ctime>
#include <vector>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <cctype>
#include "readExcelConfig.h"

// Global structure to contain result of cumulative error checks.
// One such instance should be created by the main() routine (gl_script, genScan, etc.)
// in which errorFound is false and error filename is defined as config filename + current timestamp.
// The error structure should be passed by reference to each successive function,
// which can flip errorFound to true or append any issues idenrified to fullResults.
// However, only the first function that encounters an error (and sets errorFound to true) should populate shortResult
struct errorCheckStructure
{
	bool errorFound = false;				// true if error identified which prevents execution
	vector<string> missingConfigTabs = {};  // contains names of any config-file tabs which couldn't be found
	string firstError = "";					// short string containing the first error encountered, to be displayed on screen
	vector<string> fullErrorList = {};		// cumulative text of all error results identified
};

// Function to check configuration-file for errors.
// Should be called after reading the file into an AMconfig structure.
// Function result indicates whether an error was found (true=one or more errors found, false=ok to proceed)
void evaluateConfigFile(AMconfig &configData, errorCheckStructure &errorData);

// Function to update the error results structure.
// Calling this function automatically sets errorFound to true.
// errorMsg will be appended to the fullResults, but shortResult will only be updated if it is currently blank
void updateErrorResults(errorCheckStructure &errorData, bool haltNow, string functionWithIssue, string errorMsg, string missingTab, string configFilename, string configPath);

// Error-check helper function which returns a null vector if all values of valuesToFind are in referenceValues, otherwise returns vector of missing values
vector<string> checkExistanceInList(vector<string> referenceValues, vector<string> valuesToFind);

// Error-check helper function which returns a null vector if all values are >= minima, otherwise returns vector of tags whose values are < minima
vector<string> checkForFloatErrors(vector<string> tags, vector<double> values, double minima);

// Error-check helper function which returns a null vector if all values are >= minima, otherwise returns vector of tags whose values are < minima
vector<string> checkForIntErrors(vector<string> tags, vector<int> values, int minima);
