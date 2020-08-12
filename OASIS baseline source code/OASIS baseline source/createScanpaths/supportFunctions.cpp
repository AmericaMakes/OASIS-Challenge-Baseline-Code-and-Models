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
supportFunctions.cpp contains the functions needed by createScanpaths.cpp
for file, folder and executable operations.
//============================================================*/

#include "supportFunctions.h"

using namespace std;
namespace fs = std::experimental::filesystem;


// remove any "orphan" status and output files from previous runs in the execution directory
bool cleanupOnStart()
{
	system("cls");
	cout << "Starting generateScanpaths!" << endl;
	system("del gl_sts.cfg >nul 2>&1");  // delete genLayer and genScan status files.  >nul 2>&1 avoids output if file does not exist
	system("del gs_sts.cfg >nul 2>&1");
	system("del vconfig.txt >nul 2>&1"); // delete the svg-scaling output created by genLayer
	system("del *.svg >nul 2>&1");		 // delete any slic3r outputs which weren't moved to an output folder
	string deleteErrFileCommand;
	deleteErrFileCommand = "del \"" + errorReportFilename + "\" >nul 2>&1";  // delete error reports from previous runs
	system(deleteErrFileCommand.c_str());
	return true;
}

// allow user to navigate to and select a configuration file
fileData selectConfigFile()
{
	cout << "\nPlease select an AmericaMakes configuration file in the same folder as your STL files\n";

	fileData userFile;
	LPSTR filename[MAX_PATH];
	ZeroMemory(&filename, sizeof(filename));
	filename[0] = '\0';

	OPENFILENAME ofn;  // structure used to call GetOpenFileName
	CHAR File[256];
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);

	//ofn.hwndOwner = NULL;
	ofn.hInstance = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = File;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(File);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = NULL;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = NULL;
	ofn.lCustData = 0L;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
	ofn.lpstrFilter = "Excel Files\0*.xls\0\0";
	ofn.lpstrTitle = "Please select an America Makes configuration file (.xls extension) in this or another folder";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
	// other possible flags include OFN_ENABLESIZING

	if (GetOpenFileNameA(&ofn))
	{
		// User selected a file.  Convert items in wchar_t format to string via this process:
		char DefChar1 = ' ';
		//WideCharToMultiByte(CP_ACP, 0, ofn.lpstrFile, -1, ch1, 260, &DefChar1, NULL);
		string fullFilePath = ofn.lpstrFile;
		size_t periodPos = fullFilePath.find_last_of(".");
		size_t slashPos = fullFilePath.find_last_of("\\");

		userFile.extension = fullFilePath.substr(periodPos + 1);
		userFile.filename = fullFilePath.substr(slashPos + 1);
		userFile.filenamePlusPath = fullFilePath;
		userFile.path = fullFilePath.substr(0, slashPos);

		if (userFile.extension != "xls") {
			// not the right kind of file for configuration
			userFile.xlsFileSelected = false;
			cout << "\n***You selected something which is not a .xls file***\nThis is not an AmericaMakes configuration file ... cancelling execution\n";
		}
		else {
			// .xls file selected
			userFile.xlsFileSelected = true;
			cout << "Configuration file selected: " << fullFilePath << endl;
		}	
	}
	else
	{
		// Something went wrong, or the user cancelled the selection
		cout << "\nFile selection cancelled\n";
		userFile.xlsFileSelected = false;

		// All this stuff below is to tell us exactly what went wrong
		// They are retained in case we eventually want to differentiate between them rather than simply quitting
		
		switch (CommDlgExtendedError())
		{
			case CDERR_DIALOGFAILURE: std::cout << "CDERR_DIALOGFAILURE\n";     break;
			case CDERR_FINDRESFAILURE: std::cout << "CDERR_FINDRESFAILURE\n";   break;
			case CDERR_INITIALIZATION: std::cout << "CDERR_INITIALIZATION\n";   break;
			case CDERR_LOADRESFAILURE: std::cout << "CDERR_LOADRESFAILURE\n";   break;
			case CDERR_LOADSTRFAILURE: std::cout << "CDERR_LOADSTRFAILURE\n";   break;
			case CDERR_LOCKRESFAILURE: std::cout << "CDERR_LOCKRESFAILURE\n";   break;
			case CDERR_MEMALLOCFAILURE: std::cout << "CDERR_MEMALLOCFAILURE\n"; break;
			case CDERR_MEMLOCKFAILURE: std::cout << "CDERR_MEMLOCKFAILURE\n";   break;
			case CDERR_NOHINSTANCE: std::cout << "CDERR_NOHINSTANCE\n";         break;
			case CDERR_NOHOOK: std::cout << "CDERR_NOHOOK\n";                   break;
			case CDERR_NOTEMPLATE: std::cout << "CDERR_NOTEMPLATE\n";           break;
			case CDERR_STRUCTSIZE: std::cout << "CDERR_STRUCTSIZE\n";           break;
			case FNERR_BUFFERTOOSMALL: std::cout << "FNERR_BUFFERTOOSMALL\n";   break;
			case FNERR_INVALIDFILENAME: std::cout << "FNERR_INVALIDFILENAME\n"; break;
			case FNERR_SUBCLASSFAILURE: std::cout << "FNERR_SUBCLASSFAILURE\n"; break;
		}
	}

	return userFile;  // success
}

// assess folder structure at/below the project folder indicated in config file
string evaluateProjectFolder(AMconfig &configData)
{
	// return value indicates one of several possibilities
	// "" (blank) = project folder does not exist and/or exists but has no layer subfolder
	// "L" = project folder + layer subfolder exist (and there is at least one layer_xxx.xml file present), but there is no scan subfolder
	// "LS" = project folder, layer subfolder and scan subfolder exist, and both layer_xxx.xml and scan_xxx.xml files are present

	// 1. check if layer XML folder exists
	string layerXMLFolder = configData.layerOutputFolder + "\\XMLdir";
	if (!dirExists(layerXMLFolder)) {
		// layer xml folder doesn't exist, so there are no layer files and we'll assume no scan files either
		return "";
	}

	// layer xml folder exists
	//
	// 2. count the number of layer xml files		
	fileCount xmlFileData = countLayerFiles(layerXMLFolder);
	if (xmlFileData.numFiles == 0) {
		// there are no layer files, which is the same as having no layer output folder
		return "";
	}

	// layer xml folder exists and contains xml files
	//
	// 3. see if the scan xml folder exists
	string scanXMLFolder = configData.scanOutputFolder + "\\XMLdir";
	if (!dirExists(scanXMLFolder)) {
		// indicate that there are layer files, but no scan folder or files
		return "L";
	}

	// scan xml folder exists
	//
	// 4. count the number of scan xml files
	xmlFileData = countScanFiles(scanXMLFolder);
	if (xmlFileData.numFiles == 0) {
		// no scan xml files.  indicate that we have layer but not scan files
		return "L";
	}
	else {
		// we have both scan and layer xml files
		return "LS";
	}
	
	return "";  // default, which should not be reached
} // end function

// provide the user with options based on existing layer and/or scan folders
string getUserOption(AMconfig &configData, string folderStatus)
{
	// Potential choices are based on folderStatus:
	//	"" = layer folder doesn't exist
	//		user can cancel or select either layer generation or layer+scan generation
	//	"L" = layer folder exists but not scan folder, 
	//		user can cancel or select scan generation only, layer generation only, or layer+scan generation
	//		the last two options present a further choice: Overwrite existing layer files or Merge new/old results
	//	"LS" = both layer and scan folders exist,
	//		user can cancel or select scan generation only, layer generation only, or layer+scan generation
	//		these present a further choice:  Overwrite existing layer and/or scan files or Merge new/old results
	//
	// Return values:
	//	"c" = user opts to cancel and leave any existing files untouched
	//	"l" = run layer generation only.  Delete (overwrite) the layer folder, if it exists
	//	"lm" = run layer generation only.  Merge new output with results the existing layer folder (files of same layer number will still overwrite)
	//	"s" = run scan generation only.  Delete (overwrite) the scan folder, if it exists
	//	"sm" = run scan generation only.  Merge new output with results the existing scan folder (files of same name will still overwrite)
	//	"b" = run layer generation.  If successful, run scan generation.  Delete (overwrite) layer and scan folders, if they exist, prior to layer generation
	//	"bm" = run layer generation.  If successful, run scan generation.  Merge all results with existing layer and scan files (overwriting those with same layer number)
	// Note that the "m" character is appended in a separate query to the user, based on overwrite versus merge.  Assume overwrite unless "m" is included

	string returnVal = "cancel";
	string validOptions = "";
	bool needConfirmation = false;

	// Let the user know what output folders exist, if any
	if (folderStatus == "") {
		// no existing output
		cout << "\n\nThe project folder has no existing output\n\n";
		cout << "Please type L, B or C to select from the following options\n";
		cout << "  L  Generate layer files (only), then quit\n";
		cout << "  B  Generate both layer and scan files\n";
		cout << "  C  Cancel scanpath generation\n\n";
		validOptions = "lb";
	}
	else {
		if (folderStatus == "L") {
			// existing layer output only
			cout << "\n\nThe project folder contains existing layer files, but no scan files\n\n";
			cout << "Please type L, B, S or C to select from the following options\n";
			cout << "  L  Regenerate only the layer files     (then, choose to merge with or delete prior layer files)\n";
			cout << "  B  Generate both layer and scan files  (then, choose to merge with or delete prior layer files\n";
			cout << "  S  Generate scan files from existing layers\n";
			cout << "  C  Cancel scanpath generation\n\n";
			validOptions = "lbs";
		}
		else {
			// existing layer and scan output
			cout << "\n\nThe project folder contains existing layer and scan files\n\n";
			cout << "Please type L, B, S or C to select from the following options\n";
			cout << "  L  Regenerate layer files; delete scan files   (then, choose to merge with or delete prior layer files)\n";
			cout << "  B  Regenerate both layer and scan files        (then, choose to merge with or delete prior files)\n";
			cout << "  S  Regenerate scan files from existing layers  (then, choose to merge with or delete prior scan files)\n";
			cout << "  C  Cancel scanpath generation\n\n";
			validOptions = "lbs";
		}
	}

	// Get the selection
	cout << "Enter your choice (not case sensitive) and press Enter: ";
	string textEntry;
	cin >> textEntry;  // get a string or null
	string firstChar = "";
	if ((textEntry.size() == 0) | (textEntry == "")) {
		// user simply hit return or otherwise entered a null string
		return "c"; }
	else {
		// user entry is at least one character
		firstChar = tolower(textEntry[0]);
	}

	// compare user selection against validOptions.  If match found, continue.  Otherwise, treat as cancellation
	std::size_t found = validOptions.find(firstChar);
	if (found != std::string::npos) {
		// user selection matches one of the available options
		returnVal = firstChar;
	}
	else {
		// user selection is not a valid option, or is Cancel
		return "c";
	}

	// user entered a valid choice other than Cancel.  Determine if we need to ask for confirmation to eliminate existing files
	cout << endl;
	if ((folderStatus == "") | ((folderStatus == "L")&(firstChar == "s"))) {
		// don't need confirmation because either there is no existing output -or- there are layers and user is creating scanpaths only
		return returnVal;
	}
	else {
		// need user to decide whether to merge or delete the existing results
		cout << "***Enter D to delete existing results (the usual choice) or M to merge old/new results (and overwrite any matching layer/scan#'s)\n***Anything other than d or m (plus Enter) cancels: ";
		cin >> textEntry;  // get a string or null
		if ((textEntry.size() == 0) | (textEntry == "")) {
			// user simply hit return or otherwise entered a null string
			return "c";
		}
		// user entry is at least one character
		firstChar = tolower(textEntry[0]);
		if (firstChar[0] == 'm') {
			// user opts to merge old/new results.  Indicate this by appending "m" to returnVal
			returnVal += "m";
		}
		else if (firstChar[0] != 'd') {
			// user entered something other than Merge or Delete.  Treat this as cancellation
			returnVal = "c";
		}
	}

	// at this point the user either didn't need to decide about existing results, or, they chose delete or merge
	// so we return with their selection
	cout << endl;
	return returnVal;
}

// set up folder structure for output
int setupOutputFolders(AMconfig &configData, string userChoice)
{
	// we expect one of the following values in userChoice:
	//	"l"  = Delete the layer and scan folders, if they exist.  Set up a new layer folder
	//	"lm" = do nothing - the layer folder already exists and new output will be merged with existing
	//	"s"  = Delete the scan folder, if it exists.  Set up a new scan folder
	//	"sm" = do nothing - the scan folder already exists and new output will be merged with existing
	//	"b"  = Delete the layer and scan folders, if they exist.  Set up a new layer and scan folders
	//	"bm" = do nothing - layer and scan folders already exist and new output will be merged with existing
	//
	// To simplify this, if the character "m" is appended to userChoice, we can quit this routine immediately
	//
	// return value = 0 if all is well, 1 if error

	if (userChoice.size() == 2) {
		if (userChoice[1] == 'm') {
			// merge new and old output.  folders already exist, so no further action required
			return 0;
		}
	}
	
	// Otherwise, we have to create or delete/recreate one or more folders
	//
	// First, identify which folders and sub-folders exist
	bool projectFolderExists = false;
	bool layerFolderExists = false;
	bool scanFolderExists = false;

	try {
		string mdir, rdir; // used to generate system command strings (mdir to make directories, rdir to remove them)
		if (dirExists(configData.projectFolder)) {
			projectFolderExists = true;
			if (dirExists(configData.layerOutputFolder)) {
				layerFolderExists = true;
			}
			if (dirExists(configData.scanOutputFolder)) {
				scanFolderExists = true;
			}
		}

		// If the project folder itself does not exist, create it
		if (!projectFolderExists) {
			cout << "Creating project folder " << configData.projectFolder << endl;
			mdir = "mkdir \"" + configData.projectFolder + "\" >nul 2>&1";
			system(mdir.c_str());
		}

		// Delete the scan folder, if it exists, in all cases
		// this is because we do not want to retain scan outputs which are potentially incompatible with new layer outputs
		if (scanFolderExists) {
			rdir = "rmdir \"" + configData.scanOutputFolder + "\" /s /q >nul 2>&1";
			system(rdir.c_str());
		}

		// Delete/recreate the layer folder, if userChoice is "l" or "b"
		if (userChoice == "l" | userChoice == "b") {
			// delete the layer folder, if it exists
			if (layerFolderExists) {
				rdir = "rmdir \"" + configData.layerOutputFolder + "\" /s /q >nul 2>&1";
				system(rdir.c_str());
			}
			cout << "Creating layer folder " << configData.layerOutputFolder << endl;
			mdir = "mkdir \"" + configData.layerOutputFolder + "\" >nul 2>&1";
			system(mdir.c_str());
			mdir = "mkdir \"" + configData.layerOutputFolder + "\\XMLdir\" >nul 2>&1";
			system(mdir.c_str());
			if (configData.createLayerSVG == true) {
				mdir = "mkdir \"" + configData.layerOutputFolder + "\\SVGdir\" >nul 2>&1";
				system(mdir.c_str());
			}
		}

		// Recreate the scan folder, if userChoice is "s" or "b"
		if (userChoice == "s" | userChoice == "b") {
			cout << "Creating scan folder " << configData.scanOutputFolder << endl;
			mdir = "mkdir \"" + configData.scanOutputFolder + "\" >nul 2>&1";
			system(mdir.c_str());
			mdir = "mkdir \"" + configData.scanOutputFolder + "\\XMLdir\" >nul 2>&1";
			system(mdir.c_str());
			if (configData.createLayerSVG == true) {
				mdir = "mkdir \"" + configData.scanOutputFolder + "\\SVGdir\" >nul 2>&1";
				system(mdir.c_str());
			}
		}
		return 0;  // folder structure is properly set up
	}
	catch (...) {
		// Something went wrong with folder setup
		return 1;
	}
}

// run layer or scan generation
int callGenerationCode(string sysCommand, string statusFilename)
{
	// sysCommand should include either genLayer or genScan plus full path to the configuration file
	// statusFilename should be either the executable folder path plus \gl_sts.cfg or \gs_sts.cfg
	LPSTR cmdLine = _strdup(sysCommand.c_str());

	int finished = 0;
	sts cst;  // status structure; initialized to started=false, lastLayer=0, finished=0, dn="NULL"
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	DWORD exit_code;

	// iterate until finished or error encountered
	while (!finished)
	{
		if (!CreateProcessA(
			NULL,		// lpApplicationName
			cmdLine,    // Command line (needs to include app path as first argument)
			NULL,       // Process handle not inheritable
			NULL,       // Thread handle not inheritable
			FALSE,      // Set handle inheritance to FALSE
			0,		    // Opens file in a same window
			NULL,       // Use parent's environment block
			NULL,       // starting directory - doesn't work as a parameter, so we need to specify the starting directory before calling CreateProcess
			&si,        // Pointer to STARTUPINFO structure
			&pi         // Pointer to PROCESS_INFORMATION structure
		)
			) {
			cout << "*** Unable to start a new process via\n" << sysCommand << endl;
			system("pause\n");
			return -1;
		}
		// Wait until child process exits
		WaitForSingleObject(pi.hProcess, INFINITE);
		// check return code from the executable
		if (FALSE == GetExitCodeProcess(pi.hProcess, &exit_code))
		{
			std::cerr << "GetExitCodeProcess() failure: " <<
				GetLastError() << "\n";
		}
		else if (STILL_ACTIVE == exit_code)
		{
			std::cout << "Still running\n";
		}
		// Close process and thread handles
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		if (exit_code != 0) {
			// generation returned an error; don't process additional layers
			finished = 1;
		}
		else {
			// no error; see what the generation routine sent back as status
			cst = readStatus(statusFilename);
			finished = cst.finished;
		}
	}

	return exit_code;
}

// perform final cleanup after layer or scan generation
bool cleanupOnFinish()
{
	// we should be working in the executable folder
	system("del gl_sts.cfg >nul 2>&1");	 // delete the temporary status file created by genLayer
	system("del gs_sts.cfg >nul 2>&1");	 // delete the temporary status file created by genScan
	system("del vconfig.txt >nul 2>&1"); // delete the svg-dimension file (which should have been moved to the layer folder)
	system("del *.svg >nul 2>&1");		 // delete any stray svg files (which should have been moved to the layer folder)
	return true;
}

// create a zip file containing the scan XML files
bool createScanZipfile(AMconfig &configData) {
	// try/catch in case of error
	try {
		cout << "Creating a .scn (zip) file containing the scan output files\n";
		// create a zip file in the executable folder.  If everything goes smoothly, we'll later change its extension to .scn
		HZIP hz = CreateZip(__T("scanpath_files.zip"), 0);
		if (hz == 0) {
			// hz, the zip file handle, will be zero if the file can't be created and opened for use
			cout << "*** Was not able to create scanpath_files.zip in the executable folder\n    Cancelling zip\n";
			return 1;
		}

		string scanXMLfolder = configData.scanOutputFolder + "\\XMLdir\\";
		string fname, pname, sysCommand;
		ZRESULT fnResult;  // format for zip.cpp results
		int xmlCount = 0;  // number of XML files found and added to zip

		// Identify current cursor position
		COORD cursorPosition;
		HANDLE hStdout;
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		cursorPosition = GetConsoleCursorPosition(hStdout);

		// search for and iterate over xml files in the scan output folder
		for (auto& p : fs::directory_iterator(scanXMLfolder))
		{
			if (p.path().extension() == ".xml")
			{	// found an xml file
				fname = p.path().stem().string() + ".xml";  // filename without path
				pname = p.path().string();					// full path to the file
				cout << "   Adding " << fname;
				// reset cursor position for next iteration
				SetConsoleCursorPosition(hStdout, cursorPosition);

				// create wstring versions of fname and pname
				LPCSTR wsf, wsp;
				wsf = fname.c_str();
				wsp = pname.c_str();

				// get a handle to the file using full path
				HANDLE hfout = CreateFile(wsp, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hfout == INVALID_HANDLE_VALUE) {
					cout << "\nCould not access " << fname << " due to error " << GetLastError() << endl;
					// at this point we could choose to continue iterating over XML files or simply halt
				}
				else {
					// add file to zip via handle, giving it a name without path
					fnResult = ZipAddHandle(hz, wsf, hfout);
					if (fnResult == 0) {
						xmlCount++;
					} // else ... could warn the user that this file could not be added to the zip
					// close file handle
					CloseHandle(hfout);
				}
			}
		} // end for

		// wrap up
		CloseZip(hz);

		// if xmlCount > 0, we'll consider it valid.
		// change the archive's extension to .scn and move it to project folder, else delete it and alert the user
		if (xmlCount > 0) {
			// there is at least one file in the archive, so we consider it valid
			
			// change extension to the .scn expected by LabVIEW
			cout << "Changing extension to .scn... ";
			sysCommand = "rename scanpath_files.zip scanpath_files.scn";
			system(sysCommand.c_str());

			// move the scn file to the project folder
			sysCommand = "move scanpath_files.scn \"" + configData.projectFolder + "\" >nul 2>&1";
			system(sysCommand.c_str());
			cout << "\nDone! scanpath_files.scn contains " << xmlCount << " files and is located in\n" << configData.projectFolder << endl << endl;
			// report success
			return 0;
		}
		else
		{
			cout << "\nNo XML files were found in the scan folder, or at least none which could be accessed\nThe zip archive has been deleted\n";
			// delete the zip or scn file, if it exists
			system("del scanpath_files.* >nul 2>&1");
			return 1;
		}
	}
	catch (...) {
		// something went wrong!
		cout << "\nWe encountered an unknown error while zipping the xml files\nZip has been deleted\n";
		// delete the zip file, if it exists
		system("del scanpath_files.* >nul 2>&1");
		return 1;
	}

	// cleanup - should have exited the function via one of the return statements above
	return 1;
}
