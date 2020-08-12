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
main_createScanpaths.cpp contains the main() function of the basic user interface
for the scanpath generation program, createScanpaths.exe, which
in turn calls executables for layer generation (genLayer.exe)
and scanpath generation (genScan.exe).  This is the only executable
which should be accessed directly by end users; the other two executables
should not be called directly since they do not provide an interface
to select configuration files (and also assume that error checking
on the configuration file has already been performed)
//============================================================*/

#include "supportFunctions.h"  // special functions only used by createScanpaths

#include "constants.h"
#include "readExcelConfig.h"
#include "BasicExcel.hpp"
#include "errorChecks.h"
#include "io_functions.h"

using namespace std;

int main()
{
	// createScanpaths.exe should be located in the same directory (the "executable folder") as genLayer.exe, genScan.exe and the slic3r folder.
	// You can then create a shortcut to createScanpaths.exe and move that anywhere
	
	// 1a. Clean up any existing status and SVG files from prior slicing operations
	bool returnValue = cleanupOnStart();
	// 1b. Define variables
	string sysCommand;
	string statusFilename;
	int generationResult, finished;
	// 1c. Determine the location of the executable folder
	TCHAR filePath[MAX_PATH + 1] = "";
	GetCurrentDirectory(MAX_PATH, filePath);
	string currentPath = &filePath[0];

	// 2. Ask user to select a configuration file in this or another folder
	//	this also sets the current directory to wherever the config file is located, so we saved the executable folder in currentPath, above
	fileData configFileData = selectConfigFile();
	if (configFileData.xlsFileSelected != true) { 
		system("pause");  // user either cancelled the selection or chose something other than a .xls file
		return 1;
	}

	// 3. Read and evaluate the configuration file
	cout << "Loading the configuration file...";
	AMconfig configData = AMconfigRead(configFileData.filenamePlusPath);  // read the configuration file
	configData.executableFolder = currentPath;  // we have separate variables for location of the executables and of the config file (and STL files)
	//
	// erase any error file which may be present in the config-file folder
	string deleteErrFileCommand;
	deleteErrFileCommand = "del \"" + configData.configPath + "\\" + errorReportFilename + "\" >nul 2>&1";
	system(deleteErrFileCommand.c_str());
	//
	// check the config file for errors
	cout << " checking for errors...";
	errorCheckStructure errorData;
	bool haltNow = false;
	string functionWithIssue = "";
	string errorMsg = "";
	evaluateConfigFile(configData, errorData);  // if the config file has errors, this function will quit and write to console and error file
	
	// 4. Evaluate the project folder listed in the configuration file to see whether layer and/or scan xml files exist
	//	folderStatus indicates whether layer (L) files or layer + scan (LS) files exist.  "" = neither set of files & associated folders exists
	string folderStatus = evaluateProjectFolder(configData);

	// 5. Ask user what operations to execute, based on whatever prior layer and/or scan folders exist in the project folder
	string userChoice = getUserOption(configData, folderStatus);
	if (userChoice == "c") {
		// user opts to cancel
		cout << "Scanpath generation cancelled.  Any existing files will be left untouched\n";
		system("pause\n");
		return 1;
	}
	
	// 6. User elected to proceed with layer and/or scan generation.
	// Set up folder structure for layer and/or scan output
	int folderSetupOk = setupOutputFolders(configData, userChoice);
	if (folderSetupOk != 0) {
		cout << "Something went wrong during output folder deletion or creation... perhaps a file is in use in the output folder\nScanpath generation cancelled\n";
		system("pause\n");
		return 1;
	}
	else {
		// folder structure was set up properly
		// delete any .zip or .scn file that might be left from prior executions
		string deleteZipFileCommand;
		deleteZipFileCommand = "del \"" + configData.projectFolder + "\\scanpath_files.*\" >nul 2>&1";
		system(deleteZipFileCommand.c_str());
		// set current directory back to the executable folder so that we can find genLayer, genScan and slic3r
		LPCSTR executableFolder = (configData.executableFolder + "\\").c_str();
		if (!SetCurrentDirectoryA(executableFolder)) {
			cout << "Could not set the current directory back to the executable folder!" << endl;
			system("pause");
			return -1;
		}
	}

	// 7. Execute layer generation, if selected
	if (userChoice[0] == 'l' | userChoice[0] == 'b') {
		// call layer generation
		cout << "\nBeginning layer generation\n";

		sysCommand = "\"" + configData.executableFolder + "\\" + "genLayer\" \"" + configData.configFilename + "\"";
		statusFilename = "gl_sts.cfg";

		generationResult = callGenerationCode(sysCommand, statusFilename);  // calls genLayer until all layers are complete, then returns
		// move any SVG files created by slic3r into the layer output folder
		sysCommand = "move \"" + configData.configPath + "\\*.svg\" \"" + configData.layerOutputFolder + "\" >nul 2>&1"; // move the Slic3r-generated SVG files to layer directory
		system(sysCommand.c_str());
		//	If errors, report and halt
		if (generationResult != 0) {
			cout << "We encountered an error during layer generation\nSome layer files may have been created, but are not known to be valid\n";
			system("pause\n");
			return 1;
		}
		else {
			cout << "Layer generation was successful!\n";
		}
	}

	// 8. Execute scan generation, if selected
	if (userChoice[0] == 's' | userChoice[0] == 'b') {
		// call scan generation
		cout << "\nBeginning scan generation\n";

		sysCommand = "\"" + configData.executableFolder + "\\" + "genScan\" \"" + configData.configFilename + "\"";
		statusFilename = "gs_sts.cfg";

		generationResult = callGenerationCode(sysCommand, statusFilename);  // calls genScan until all layers are complete, then returns
		//	If errors, report and halt
		if (generationResult != 0) {
			cout << "We encountered an error during scan generation\nSome scan files may have been created, but are not known to be valid\n";
			returnValue = cleanupOnFinish();
			system("pause\n");
			return 1;
		}
		else {
			// Successfule scan path generation!
			cout << "Scan generation was successful!\n";
			// if user requested a zip file containing the scan XML files, generate this in the project folder
			if (configData.createScanZIPfile == true) {
				// create zip file
				generationResult = createScanZipfile(configData);
				// we don't check the result of zip file generation since it is handled/reported by the function, and doesn't count as a true error
			}
		}
	}

	// 9. Cleanup status files and other items
	returnValue = cleanupOnFinish();

	int sysReturnValue = system("\npause\n");

    return 0;
}

