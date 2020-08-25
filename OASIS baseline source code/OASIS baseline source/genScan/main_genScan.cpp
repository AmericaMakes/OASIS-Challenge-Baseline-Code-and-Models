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
main_genScan.cpp contains the main() function of the Scan Generator
program. Different functions are called from inside of main()
to perform various tasks of reading the LAYER files, generating
scanpaths for the same  and writing the output in XML and
SVG formats. This also calls functions to read in the user inputs
from the configuration files.
//============================================================*/

#define printTraj 0
// If printTraj set to 1, the regions of each trajectory will printed to console as they are added from the layer file
// Normally set printTraj to 0
// This complements printTrajectories found in writeScanXML.h, which calls out trajectories as they are processed


#include <fstream>
#include <iostream>
#include <filesystem>
#include <msxml6.h>
#include <conio.h>
#include "writeScanXML.h"
#include "ScanPath.h"

#include "constants.h"
#include "readExcelConfig.h"
#include "BasicExcel.hpp"
#include "errorChecks.h"
#include "io_functions.h"


using namespace std;

constexpr auto DISPLAYLAYER = 0;
constexpr auto DISPLAYSCANPATH = 0;
constexpr auto SVGSCANTARGET = 1;
constexpr auto SVGLAYERTARGET = 0;
constexpr auto XMLTARGET = 0;

//MSXML objects to read LAYER and write SCAN files
IXMLDOMDocument *pXMLDomLayer = NULL;
IXMLDOMDocument *pXMLDomScan = NULL; 

// function to compare two trajectories and determine which has the lower number, and should be printed first
bool compareTrajNums(const trajectory &a, const trajectory &b)
{
	return a.trajectoryNum < b.trajectoryNum;
}

// function to clear the large variables re-used in each layer iteration
void clearVars(layer* L, trajectory* T, path* tempPath)
{
	(*L).vList.clear();
	(*L).s.rList.clear();
	//
	(*L).vList.shrink_to_fit();
	(*L).s.rList.shrink_to_fit();

	(*T).trajRegionIsHatched.clear();
	(*T).trajRegionLinks.clear();
	(*T).trajRegions.clear();
	(*T).trajRegionTags.clear();
	(*T).trajRegionTypes.clear();
	(*T).vecPath.clear();
	//
	(*T).trajRegionIsHatched.shrink_to_fit();
	(*T).trajRegionLinks.shrink_to_fit();
	(*T).trajRegions.shrink_to_fit();
	(*T).trajRegionTags.shrink_to_fit();
	(*T).trajRegionTypes.shrink_to_fit();
	(*T).vecPath.shrink_to_fit();

	(*tempPath).vecSg.clear();
	//
	(*tempPath).vecSg.shrink_to_fit();
}


int main(int argc, char **argv)
// Required argument for genScan.exe:
// <filename>.xls = full path to configuration file (surrounded by "").  If not specified, error will be reported
{
	// To enhance memory management, this program is called from an external script. Every time this program is called, 
	// it reads the last layer number generated from a *cfg file, generates a specific number of layers, and write the 
	// final layer number in the *cfg file for next time (or write if it is finished).

	// Determine where we are (current path)
	TCHAR filePath[MAX_PATH + 1] = "";
	GetCurrentDirectory(MAX_PATH, filePath);
	string currentPath = &filePath[0];

	double a_min = 0.0;
	double a_max = 0.0;
	double hatchAngle;
	ofstream stfile;
	AMconfig configData;
	errorCheckStructure errorData;  // holds results of any errors encountered
	string errorMsg;
	double fullHatchOffset;
	// Define variables re-used in each layer iteration
	layer L;
	trajectory T;
	path tempPath;

	// Begin by parsing the command-line arguments, if any.
	// argc indicates the number of command-line arguments entered by the user
	string configFilename = "";
	if (argc > 1)
	{
		configFilename = argv[1];
	}
	else
	{
		// no config file specified... most likely the user tried to run genLayer or genScan directly
		string errMsg = "Please use createScanpaths.exe to handle layer and scan generation. genScan.exe and genLayer.exe are helper functions only\n";
		system("pause");
		return -1;
	}

	configData = AMconfigRead(configFilename);  // if file can't be read or is invalid, AMconfigRead will halt execution
	configData.executableFolder = currentPath;  // assume we begin in the executable folder, slic3r folder is located

	// Determine which layers to process in this function call
	// First, get total layers to process from the Excel configuration file
	string xmlFolder = configData.layerOutputFolder + "\\XMLdir\\";
	fileCount layerFileInfo = countLayerFiles(xmlFolder);	// provides number of XML files and min/max layer numbers
	
	// If there are no layer files in the folder, report an error and quit
	if (layerFileInfo.numFiles < 1) {
		errorMsg = configData.layerOutputFolder + " does not contain any XML layer files.  Please run layer generation prior to scan generation\n";
		updateErrorResults(errorData, true, "genScan", errorMsg, "", configData.configFilename, configData.configPath);
	}

	if (configData.endingScanLayer < 1)	// if user sets ending layer to -1, we use actual max layer# as ending layer#
		configData.endingScanLayer = layerFileInfo.maxLayer;
	//
	// Next, read the temporary config file to see which layers have already been done
	int finished = 0; // flag to write if slicing of the part is complete.  Set to zero by default
	sts cst = readStatus("gs_sts.cfg"); // read the ending layer number of the last iteration (or check if it is the first layer)
	int started = cst.started; // flag to check if slicing of the part has already started
	//
	// determine the starting layer
	int sLayer = cst.lastLayer +1; // lastLayer will have been intialized to 0 if no prior cfg file
	if (sLayer < configData.startingScanLayer)	// check if user wants to start other than at lowest layer
	{
		sLayer = configData.startingScanLayer;
	}
	if (sLayer < layerFileInfo.minLayer)	// check if user-selected start layer# is smaller than actual first layer file
	{
		sLayer = layerFileInfo.minLayer;
	}
	if (sLayer > layerFileInfo.maxLayer)	// check if user-selected start layer# is greater than actual last layer file
	{
		// If the desired starting layer# is above the actual highest layer#, report an error and quit
		errorMsg = "The starting layer# indicated in the config file (" + to_string(sLayer) + ") is beyond the highest layer file in this folder (" + to_string(layerFileInfo.maxLayer) + ")\n";
		updateErrorResults(errorData, true, "genScan", errorMsg, "", configData.configFilename, configData.configPath);
	}
	if (configData.endingScanLayer < layerFileInfo.minLayer)	// check if user wants to end scan generation below the actual first layer file
	{
		// If the desired ending layer# is below the actual lowest layer#, report an error and quit
		errorMsg = "The ending layer# indicated in the config file (" + to_string(configData.endingScanLayer) + ") is below the lowest layer file in this folder (" + to_string(layerFileInfo.minLayer) + ")\n";
		updateErrorResults(errorData, true, "genScan", errorMsg, "", configData.configFilename, configData.configPath);
	}
	
	// determine the final layer (fLayer) to be processed by this instance of genScan
	int fLayer = sLayer + numLayersPerCall;	// final layer number for a particular call to this function
	if (fLayer >= configData.endingScanLayer)
	{	// user wants to end below the highest layer
		fLayer = configData.endingScanLayer;
		finished = 1;
	}
	if (fLayer >= layerFileInfo.maxLayer)
	{	// check if user-selected start layer# is smaller than actual first layer file
		fLayer = layerFileInfo.maxLayer;
		finished = 1;
	}
	int numLayer = layerFileInfo.numFiles;

	int close = 0;
	string cmd;
	double currentContourOffset = 0.0; // used to compute offset from part + inter-contour spacing for individual contours

	//get viewer parameters
	string vmd = "copy \"" + configData.layerOutputFolder + "\\vConfig.txt\"> NUL";
	system(vmd.c_str());
	ifstream vfile("vConfig.txt");
	char c;
	double mag, xo, yo;
	string line;
	getline(vfile, line);
	istringstream ss(line);
	ss >> mag >> c >> xo >> c >> yo;

	// Generate a list of region profile tags, to enable indexing from a region's tag to a specific profile in regionProfileList
	vector<string> tagList;
	for (int r = 0; r < configData.regionProfileList.size(); r++)
	{	tagList.push_back(configData.regionProfileList[r].Tag);	}

	// set up variables for cursor control
	COORD cursorPosition;
	HANDLE hStdout;
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	// Identify current cursor position
	cursorPosition = GetConsoleCursorPosition(hStdout);

	//********************************************
	// PROCESS SOME LAYERS
	for (int i = sLayer; i <= fLayer; i++)
	{
		cout << "Processing layer " << i << " of " << layerFileInfo.maxLayer << endl;
		// reset cursor position
		SetConsoleCursorPosition(hStdout, cursorPosition);

		clearVars(&L, &T, &tempPath);
		//generate the output filename by pre-pending appropriate numbers of zeroes
		string zs;
		for (int k = 0; k <(int)(to_string(numLayer)).size() - (int)(to_string(i)).size(); k++)
		{
			zs = zs + "0";
		};
		string lfn = "layer_" + zs + to_string(i) + ".xml";
		string fullLayerPath = configData.layerOutputFolder + "\\XMLdir\\" + lfn;
		string svfn = "scan_" + zs + to_string(i) + ".svg";
		string xfn = "scan_" + zs + to_string(i) + ".xml";

		HRESULT hr = CoInitialize(NULL);  // attempt to set up the output Domain Object Model (DOM).  If this fails, we can't create XML
		int bCont = 1;
		if (SUCCEEDED(hr))
		{
			//read the appropriate XML layer file
			wstring wfn(fullLayerPath.begin(), fullLayerPath.end());
			LPCWSTR  wszValue = wfn.c_str();
			int fn_err = loadDOM(wszValue);
			if (fn_err != 0)
			{
				// could not load a particular layer file
				errorMsg = "Could not load " + fullLayerPath + "\n";
				updateErrorResults(errorData, true, "loadDOM", errorMsg, "", configData.configFilename, configData.configPath);
				bCont = 0;
			}			
			if (bCont)
			{
				// convert the data in xml file to appropriate data structure
				L = traverseDOM();
				int verifyResult = verifyLayerStructure(configData, fullLayerPath, L, tagList);  // check that the layer is valid.  If not, function will end genScan execution
				// determine the bounding box of the layer
				vector<vertex> BB;
				BB = getBB(L);

				// Identify the set of trajectory numbers encountered in the Layer file, and
				// the regions which fall under each trajectory.  This step only populates the region numbers, 
				// and does not actually create the scan paths within each trajectory
				vector<trajectory> trajectoryList;
				trajectoryList = identifyTrajectories(configData, L, i);
				// Sort the trajectory list by trajectoryNum
				sort(trajectoryList.begin(), trajectoryList.end(), compareTrajNums);
				
				// Create a list of the existing trajectory numbers
				vector<int> trajIndex;
				for (vector<trajectory>::iterator itl = trajectoryList.begin(); itl != trajectoryList.end(); ++itl)
				{
					trajIndex.push_back((*itl).trajectoryNum);	// This creates a list of trajectories, but not in numerical order
					#if printTraj
						cout << "Identified from Layer file: trajectory " << (*itl).trajectoryNum << endl;
					#endif
				}
				#if printTraj
					cout << "Size of trajectoryList: " << trajectoryList.size() << endl;
				#endif

				// Iterate across the list of trajectories in trajList and generate scan paths for their regions
				int numTrajectories = trajIndex.size();
				for (int tNum = 0; tNum != numTrajectories; ++tNum)
				{	
					// Iterate across the regions in trajectoryList[tNum].trajRegions.
					// First check if the region's isHatched == TRUE.  If so, ignore this region.
					// Otherwise, compare the region's tag against all other regions in this trajectory (of same hatch/contour type) to 
					// generate a list of regions to be grouped under a single path.  Pass this list of region numbers to
					// the hatching or contouring function and also mark these regions isHatched = TRUE.
					int numRegions = trajectoryList[tNum].trajRegions.size();  // Number of regions within this trajectory
					#if printTraj
						cout << "Processing trajectory " << trajectoryList[tNum].trajectoryNum << " in position " << tNum << endl;
						cout << "	This trajectory contains " << numRegions << " regions" << endl;
					#endif
					int regionIndex, rProfileNum;
					string regionType, regionTag;
					vector<string>::iterator temp3;
					regionProfile* rProfile;
					vector<int> regionsWithinPath;  // list of regions to be hatched or contoured together (same trajectory, tag and type)

					for (int rNum = 0; rNum != numRegions; ++rNum)
					{
						#if printTraj
							cout << "	  Evaluating region number " << rNum << endl;
						#endif
						if (trajectoryList[tNum].trajRegionIsHatched[rNum] == false)
						{
							#if printTraj
								cout << "		This region has not yet been scanpathed... proceeding" << endl;
							#endif
							regionsWithinPath.clear();
							//regionsWithinPath.push_back(rNum);
							regionsWithinPath.push_back(trajectoryList[tNum].trajRegions[rNum]);  //push back the region# in trajectoryList[tNum_position].trajRegions[rNum]
							trajectoryList[tNum].trajRegionIsHatched[rNum] = true;
							regionType = trajectoryList[tNum].trajRegionTypes[rNum];
							regionTag = trajectoryList[tNum].trajRegionTags[rNum];
							// determine the regionProfile which corresponds to regionTag
							temp3 = find(tagList.begin(), tagList.end(), regionTag);
							rProfileNum = distance(tagList.begin(), temp3);
							rProfile = &(configData.regionProfileList[rProfileNum]); // Create shortcut to the indicated region profile
							#if printTraj							
								cout << "		Creating scanpath for trajectory " << trajectoryList[tNum].trajectoryNum << " > region tag " << regionTag << " > type " << regionType << ", regionNum " << rNum << endl;
							#endif

							// If this region is a hatch rather than contour, compute the hatch angle and min/max hatch boundaries based on this angle
							if (regionType != "contour") {
								// compute hatch angle for this region as (starting angle + (#layers-1)*inter-later rotation angle )... all mod 360 degrees
								// the double use of fmod(x, 360) converts negative hatch angles to the positive equivalent
								hatchAngle = fmod(fmod((*rProfile).layer1hatchAngle + (i - 1)*((*rProfile).hatchLayerRotation), 360.0)+360.0, 360.0); // degrees
								// Determine the min/max coordinates of the current layer along the primary hatching axis for this region
								// Must repeat this for each region because hatch angles may differ, leading to alternate min/max hatch points
								if (L.vList.size() > 0) {
									findHatchBoundary(L.vList, hatchAngle, &a_min, &a_max);
								}
							}

							// Iterate across all remaining regions to aggregate other regions with the same tag
							for (int rNum2 = rNum+1; rNum2 < numRegions; ++rNum2)
							{
								#if printTraj
									cout << "			Comparing type and tag for region number " << rNum2 << endl;
								#endif
								if ((regionType == trajectoryList[tNum].trajRegionTypes[rNum2]) &
									(regionTag == trajectoryList[tNum].trajRegionTags[rNum2]))
								{
									regionsWithinPath.push_back(trajectoryList[tNum].trajRegions[rNum2]);
									trajectoryList[tNum].trajRegionIsHatched[rNum2] = true;
									#if printTraj
										cout << "				Adding region tag " << regionTag << " > type " << regionType << ", regionNum " << rNum2 << endl;
									#endif
								} //end if regionType
							} // end for rNum2

							// Now that we know which regions to process, send those regions to the appropriate generator
							//
							// IF THIS IS A CONTOUR:
							if ((regionType == "contour") & ((*rProfile).contourStyleID != "") & ((*rProfile).numCntr > 0))
							{	// Do contouring.
								// Loop over the indicated number of contours, create a contour and increment the contour offset
								#if printTraj
									cout << "		  Creating contour scanpaths" << endl;
								#endif
								for (int n = 0; n < (*rProfile).numCntr; n++)
								{
									currentContourOffset = (n*(*rProfile).resCntr + (*rProfile).offCntr); // offset = n*inter-contour spacing plus offset from part
									tempPath = contour(L, regionsWithinPath, (*rProfile), currentContourOffset, BB, configData.outputIntegerIDs);
									if ((tempPath.vecSg).size() > 0) {
										(trajectoryList[tNum].vecPath).push_back(tempPath);
									}
								}
							} // end contouring

							//
							// IF THIS IS A HATCH:
							if ((regionType == "hatch") & ((*rProfile).hatchStyleID != "") & ((*rProfile).resHatch > 0))
							{	
								#if printTraj
									cout << "			Creating hatch scanpaths for hatch angle " << hatchAngle << endl;
									cout << "			  a_min = " << a_min << ", a_max = " << a_max << endl;
								#endif
								// Do hatching for all regions with this tag
								// First compute the actual hatch offset relative to contours, if contouring is enabled for the region
								if (((*rProfile).contourStyleID != "") & ((*rProfile).numCntr > 0))
								{	// Contours are enabled in this region.  Add full contour offset to hatch offset
									fullHatchOffset = (*rProfile).offHatch + (*rProfile).offCntr + (max(0, (*rProfile).numCntr - 1)*(*rProfile).resCntr);
								}
								else {
									// Contours are not enabled in this region.  Only hatch offset matters
									fullHatchOffset = (*rProfile).offHatch;
								}
								// Create the hatches via either basic or optimized hatch algorithm.
								// Basic algo overlays parallel lines over all parts at once, and draws hatch lines through all parts
								// Optimized algorithm attempts to minimize jumps by hatching in smaller pieces, focusing on regional clusters
								if ((*rProfile).scHatch == 1) {
									tempPath = hatchOPT(L, regionsWithinPath, (*rProfile), fullHatchOffset, hatchAngle, a_min, a_max, configData.outputIntegerIDs, BB);
								}
								else
								{
									tempPath = hatch   (L, regionsWithinPath, (*rProfile), fullHatchOffset, hatchAngle, a_min, a_max, configData.outputIntegerIDs, BB);
								}
								if ((tempPath.vecSg).size() > 0) {
									(trajectoryList[tNum].vecPath).push_back(tempPath);
								}
							}  // end hatching

							#if printTraj
								cout << "		End if (trajectoryList[tNum].trajRegionIsHatched[rNum] == false)" << endl;
							#endif
						} // if trajRegionIsHatched

						#if printTraj
							cout << "		End for (int rNum = 0; rNum != numRegions; ++rNum)" << endl;
						#endif
					} // for rNum

					#if printTraj
						cout << "		End for (int temp1 = 0; temp1 != numTrajectories; ++temp1)" << endl;
					#endif
				} // for numTrajectories

				#if printTraj
				cout << "Trajectory loop completed; preparing to write XML and SVG files" << endl;
				#endif
				// write the XML schema to a DOM and then to a file
				string fullXMLpath = configData.scanOutputFolder + "\\XMLdir\\" + xfn;
				createSCANxmlFile(fullXMLpath, i, configData, trajectoryList);

				// if user wants to generate SVG files and we are either on the first layer or a multiple of the SVG interval, do so
				if ( (configData.createScanSVG == 1) && ((i % configData.scanSVGinterval == 0) | (i==0)) ) {
					//write SCAN output to SVG
					string fullSVGpath = configData.scanOutputFolder + "\\SVGdir\\" + svfn;
					scan2SVG(fullSVGpath, trajectoryList, 2000, mag, xo, yo);
				}
				#if printTraj
					cout << "Preparing to CoUninitialize" << endl;
				#endif
				CoUninitialize();  // release the Domain Object Model
				#if printTraj
					cout << "CoUninitialize complete" << endl << endl;
				#endif
			}  // if bCont
		}  // if succeeded
	}  // for i

	//write details to the *.cfg file for next call 
	stfile.open("gs_sts.cfg");
	stfile << 1 << endl;		// started
	stfile << fLayer << endl;	// last layer completed
	stfile << finished << endl;	// finished flag
	stfile << configData.scanOutputFolder << endl;	// config file folder
	stfile.close();

	return 0;

CleanUp:
	cout << "CleanUp section reached" << endl;
	return 0;
};
