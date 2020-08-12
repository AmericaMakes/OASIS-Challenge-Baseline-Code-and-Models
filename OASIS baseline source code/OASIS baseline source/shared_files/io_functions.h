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
io_functions.h defines the data structure and the functions needed 
to manipulate the user inputs through CSV files
//============================================================*/

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <msxml6.h>
#include <comdef.h>
#include <tchar.h>
#include <conio.h>
#include <sstream>
#include <fstream>
#include <math.h>
#include <filesystem>

#include "readExcelConfig.h"

// structure to read/write the genLayer or genScan *.cfg file to get information about the last layer proessed, if any
struct sts
{
	int started;	// whether processing has started for the part
	int lastLayer;	// last layer number done in the current call
	int finished;	// whether part is finished
	string dn = "NULL";	// directory of the config file
};

//identify current cursor position, to enable it to be reset between layers
COORD GetConsoleCursorPosition(HANDLE hConsoleOutput);

//check if directory exists
bool dirExists(const std::string& dirName_in);

//find maximum in a vector
double find_max(vector<double>in);

//fin minimum in a vector
double find_min(vector<double>in);

//read layer-processing history from gl_sts.cfg (from genLayer) or gs_sts.cfg (from genScan)
sts readStatus(string filename);

//returns true of s is a string of numerals only (a valid integer)
bool has_only_digits(const string s);

//Holds output from countFiles function
struct fileCount
{
	int numFiles = 0;
	int minLayer = 1;
	int maxLayer = 0;
};

//counts the number of layer xml files in a particular directory (prefaced with "layer_" and ending in .xml)
fileCount countLayerFiles(string path);

//counts the number of scan xml files in a particular directory (prefaced with "scan_" and ending in .xml)
fileCount countScanFiles(string path);
