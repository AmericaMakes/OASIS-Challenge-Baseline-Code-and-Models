/*============================================================//
Copyright (c) 2020 America Makes
All rights reserved
Created under ALSAM project 3024

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================*/

// supportFunctions.h contains file and folder management items for generateScanpaths

#pragma once

#include <filesystem>
#include <cctype>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <msxml6.h>
#include <comdef.h>
#include <tchar.h>
#include <conio.h>
#include <sstream>
#include <math.h>
#include <chrono>
#include <time.h>
#include <algorithm>

#include "readExcelConfig.h"
#include "BasicExcel.hpp"
#include "errorChecks.h"
#include "io_functions.h"
#include "constants.h"

#include "zip.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif


// holds results of user's navigation and selection of configuration file
struct fileData
{
	bool xlsFileSelected = false;  // false if user cancels or selects non-xls file.  True means user selected the right type of file, but the file may still have errors
	string filenamePlusPath, filename, path, extension;
};


// delete any runtime configuration and SVG files from prior runs
bool cleanupOnStart();

// allow user to navigate to and select a configuration file
fileData selectConfigFile();

// assess folder structure at/below the project folder indicated in config file
string evaluateProjectFolder(AMconfig &configData);

// provide the user with options based on existing layer and/or scan folders
string getUserOption(AMconfig &configData, string folderStatus);

// set up folder structure for output
int setupOutputFolders(AMconfig &configData, string userChoice);

// run layer or scan generation
int callGenerationCode(string sysCommand, string statusFilename);

// perform final cleanup after layer or scan generation, irrespective of success.  Does not affect error result file, if any
bool cleanupOnFinish();

// create a zip file containing the scan XML files
bool createScanZipfile(AMconfig &configData);
