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
io_functions.cpp contains several low-level file and folder functions 
used by createScanpaths.cpp and/or supportFunctions.cpp
//============================================================*/

#include "io_functions.h"
#include "readExcelConfig.h"

using namespace std;
namespace fs = std::experimental::filesystem;


COORD GetConsoleCursorPosition(HANDLE hConsoleOutput)
{
	CONSOLE_SCREEN_BUFFER_INFO cbsi;
	if (GetConsoleScreenBufferInfo(hConsoleOutput, &cbsi))
	{
		return cbsi.dwCursorPosition;
	}
	else
	{
		// The function failed. Call GetLastError() for details.
		COORD invalid = { 0, 0 };
		return invalid;
	}
}

bool dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

double find_max(vector<double>in)
{
	double max = in[0];
	for (int i = 1; i < in.size(); i++)
	{
		if (max < in[i])
			max = in[i];
	}
	return max;
}

double find_min(vector<double>in)
{
	double min = in[0];
	for (int i = 1; i < in.size(); i++)
	{
		if (min> in[i])
			min = in[i];
	}
	return min;
}

// read status of genLayer or genScan runtime status file
sts readStatus(string filename)
{	// filename should be either gl_sts.cfg or gs_sts.cfg for genLayer and genScan, respectively
	ifstream file(filename);
	sts s;
	if (!file)
	{
		s.started = 0;
		s.lastLayer = 0;
		s.finished = 0;
		s.dn = "NULL";
	}
	else
	{
		string line;
		getline(file, line);
		s.started = atoi(line.c_str());
		getline(file, line);
		s.lastLayer = atoi(line.c_str());
		getline(file, line);
		s.finished = atoi(line.c_str());
		getline(file, line);
		s.dn = line;
	}
	return s;
}

bool has_only_digits(const string s)
{
	return s.find_first_not_of("0123456789") == string::npos;
};

fileCount countLayerFiles(string path)
// Counts the number of files in a given directory which begin with "layer_" and have a particular extension, such as .xml
{
	fileCount output;
	std::string fname, fname2;

	for (auto& p : fs::directory_iterator(path))
	{
		if (p.path().extension() == ".xml")
		{
			fname = p.path().stem().string();
			if (fname.substr(0, 6) == "layer_")
			{
				fname2 = fname.substr(6, 12).c_str();
				if (has_only_digits(fname2))
				{
					output.numFiles += 1;
					if (atoi(fname2.c_str()) < output.minLayer)
						output.minLayer = atoi(fname2.c_str());
					if (atoi(fname2.c_str()) > output.maxLayer)
						output.maxLayer = atoi(fname2.c_str());
				}
			}
		}
	}
	return output;
};

fileCount countScanFiles(string path)
// Counts the number of files in a given directory which begin with "scan_" and have a particular extension, such as .xml
{
	fileCount output;
	std::string fname, fname2;

	for (auto& p : fs::directory_iterator(path))
	{
		if (p.path().extension() == ".xml")
		{
			fname = p.path().stem().string();
			if (fname.substr(0, 5) == "scan_")
			{
				fname2 = fname.substr(5, 12).c_str();
				if (has_only_digits(fname2))
				{
					output.numFiles += 1;
					if (atoi(fname2.c_str()) < output.minLayer)
						output.minLayer = atoi(fname2.c_str());
					if (atoi(fname2.c_str()) > output.maxLayer)
						output.maxLayer = atoi(fname2.c_str());
				}
			}
		}
	}
	return output;
};

