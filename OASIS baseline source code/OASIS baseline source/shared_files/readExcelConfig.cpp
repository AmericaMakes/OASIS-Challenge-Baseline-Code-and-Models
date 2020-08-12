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
readExcelConfig.cpp contains the functions needed to read the Excel-based 
America Makes configuration input.  It is used by all three
ALSAM3024 projects.
//============================================================*/

#include <sstream>
#include <algorithm>
#include <vector>
#include <cctype>
#include <string>

#include "constants.h"
#include "readExcelConfig.h"
#include "BasicExcel.hpp"
#include "errorChecks.h"

using namespace YExcel;


string as_lower(string input)
{
	// Reads a string input and returns in all lower case
	std::transform(input.begin(), input.end(), input.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return input;
}

string parseToString(BasicExcelCell* unknownInput)
{
	// Parses a cell value returned from BasicExcel into a string.
	// If unknownInput is a string, no further processing is needed.
	// If it is an integer, we need to truncate it to avoid storing "3" as "3.000000".
	// Similarly, for double values we want to avoid excess truncation.
	string output;
	double d1, d2;
	int tempInt;
	switch (unknownInput->Type())
	{
	case BasicExcelCell::UNDEFINED:
		// This case will be selected for blank cells
		output = "";
		break;
	case BasicExcelCell::STRING:
		output = as_lower(unknownInput->GetString());
		break;
	case BasicExcelCell::INT:
		// IN PRACTICE, BASICEXCEL NEVER USES THIS CASE. INT IS ALWAYS CLASSIFIED AS DOUBLE
		output = as_lower(to_string(unknownInput->GetInteger()));
		break;
	case BasicExcelCell::DOUBLE:
		// This case captures both int and double.  Differentiate them to avoid int-->double, e.g. 15.000000
		d1 = unknownInput->GetDouble();
		d2 = trunc(d1);
		if (d1 == d2)
		{	   // Value is an integer
			tempInt = int(d2);
			output = std::to_string(tempInt);
		}
		else { // Value is a double
			output = std::to_string(d1);
		}
		break;
	}
	return output;
}

bool parseToBool(BasicExcelCell* unknownInput)
{
	// Parses a cell value returned from BasicExcel into a boolean true/false value
	// "yes" is the only value accepted as true.  All other values are interpreted as false.  Case insensitive
	
	bool result = false;

	switch (unknownInput->Type())
		{
		case BasicExcelCell::UNDEFINED:
			// This case will be selected for blank cells
			break;
		case BasicExcelCell::STRING:
			if (as_lower(unknownInput->GetString()) == "yes") { result = true; }
			break;
		case BasicExcelCell::INT:
			// IN PRACTICE, BASICEXCEL NEVER USES THIS CASE. INT IS ALWAYS CLASSIFIED AS DOUBLE
			break;
		case BasicExcelCell::DOUBLE:
			// This case captures both int and double.  Differentiate them to avoid int-->double, e.g. 15.000000
			break;
	}
	
	return result;
}

void readGeneralParameters(BasicExcelWorksheet* sheet2, AMconfig* configData, string tabName)
{	// Read the general parameters tab of an America Makes Excel configuration file.
	// This is called by AMconfigRead

	// First determine the path of the configuration file
	size_t slashPos = (*configData).configFilename.find_last_of("\\");
	string path = (*configData).configFilename.substr(0, slashPos);
	//
	// Read folder name and layer thickness
	std::string folderName = sheet2->Cell(3, 2)->GetString();	// Name of the desired folder to contain both layer and scan outputs
	// Determine the path of the configuration file, and pre-pend this to folderName
	(*configData).projectFolder = path + "\\" + folderName;
	// Define layer and scan sub-folders based on folderName
	(*configData).layerOutputFolder = (*configData).projectFolder + "\\" + "LayerFiles";
	(*configData).scanOutputFolder  = (*configData).projectFolder + "\\" + "ScanFiles";

	// Read layer thickness and dosing factor
	(*configData).layerThickness_mm = sheet2->Cell(5, 2)->GetDouble();
	(*configData).dosingFactor      = sheet2->Cell(6, 2)->GetDouble();

	// Read and parse whether to create and use auto-generated integer ID's instead of the string ID's in the configuration file
	(*configData).outputIntegerIDs = parseToBool(sheet2->Cell(7, 2));

	// Read and parse whether to create a zip file containing scan files
	(*configData).createScanZIPfile = parseToBool(sheet2->Cell(8, 2));

	// Read SVG output controls for layer files
	(*configData).createLayerSVG = parseToBool(sheet2->Cell(13, 2));
	(*configData).layerSVGinterval = sheet2->Cell(14, 2)->GetInteger();
	if ((*configData).layerSVGinterval < 1) { (*configData).layerSVGinterval = 1; }  // so that -1 is treated as "all layers"

	// Read SVG output controls for scan files
	(*configData).createScanSVG = parseToBool(sheet2->Cell(13, 3));
	(*configData).scanSVGinterval = sheet2->Cell(14, 3)->GetInteger();
	if ((*configData).scanSVGinterval < 1) { (*configData).scanSVGinterval = 1; }  // so that -1 is treated as "all layers"

	// Read starting/ending scan output layers
	(*configData).startingScanLayer = sheet2->Cell(17, 2)->GetInteger();
	(*configData).endingScanLayer = sheet2->Cell(18, 2)->GetInteger();

	return;
}

void readVelocityProfiles(BasicExcelWorksheet* sheet3, AMconfig* configData, string tabName)
{	// Read the velocity profiles tab of an America Makes Excel configuration file.
	// This is called by AMconfigRead

	// Loop until we run into a blank profile ID
	velocityProfile vpRow;
	int rowNum = 6;	// First row of data.  Note that C++ counts from 0 rather than 1
	string temp = parseToString(sheet3->Cell(rowNum, 0));
	while (temp != "")
	{
		vpRow.ID = temp;
		vpRow.integerID = (*configData).VPlist.size() + 1;  // auto-populate a unique integer ID for LabView
		vpRow.velocity = sheet3->Cell(rowNum, 1)->GetDouble();
		vpRow.mode = sheet3->Cell(rowNum, 2)->GetString();
		vpRow.laserOnDelay = sheet3->Cell(rowNum, 3)->GetDouble();
		vpRow.laserOffDelay = sheet3->Cell(rowNum, 4)->GetDouble();
		vpRow.jumpDelay = sheet3->Cell(rowNum, 5)->GetDouble();
		vpRow.markDelay = sheet3->Cell(rowNum, 6)->GetDouble();
		vpRow.polygonDelay = sheet3->Cell(rowNum, 7)->GetDouble();
		// Add this velocity profile row to the vector structure
		(*configData).VPlist.push_back(vpRow);
		rowNum += 1;
		// Read next scan strategy row, to decide whether to proceed
		temp = parseToString(sheet3->Cell(rowNum, 0));
	}
}

void readSegmentStyles(BasicExcelWorksheet* sheet4, AMconfig* configData, string tabName)
{	// Read the segment-styles tab of an America Makes Excel configuration file.
	// This is called by AMconfigRead

	// Generate a list of velocity profile ID strings, to enable indexing to a specific profile
	vector<string> vpList;
	for (int r = 0; r < (*configData).VPlist.size(); r++)
	{	vpList.push_back((*configData).VPlist[r].ID);	}

	// Read SegmentStyles on tab 4.  This is an open-ended list which forms segmentStyleList
	// Loop until we run into a blank tag
	segmentStyle scanRow;	// a single scan strategy
	int rowNum = 8;	// starting row of data; hard-coded expectation
	string temp = parseToString(sheet4->Cell(rowNum, 0));
	while (temp != "")
	{
		// This row contains actual data and will be read in as a scan strategy
		scanRow.ID = temp;
		scanRow.integerID = (*configData).segmentStyleList.size() + 1;  // auto-populate a unique integerID for LabView
		scanRow.vpID = parseToString(sheet4->Cell(rowNum, 1));
		// Identify the vpIntID which corresponds to vpID
		std::vector<string>::iterator it = find(vpList.begin(), vpList.end(), scanRow.vpID);
		scanRow.vpIntID = std::distance(vpList.begin(), it) + 1;

		// Determine if the lead-laser section is populated
		string temp2 = parseToString(sheet4->Cell(rowNum, 2));
		if (temp2 == "")
		{
			scanRow.laserMode = ""; // This is a Jump-only strategy without a specific laser indicated
			scanRow.leadLaser.travelerID = "";
			scanRow.leadLaser.syncOffset = 0.0;
			scanRow.leadLaser.power = 0;
			scanRow.leadLaser.spotSize = 0;
			scanRow.leadLaser.wobble = false;
			scanRow.trailLaser.travelerID = "";
			scanRow.trailLaser.syncOffset = 0.0;
			scanRow.trailLaser.power = 0;
			scanRow.trailLaser.spotSize = 0;
			scanRow.trailLaser.wobble = false;
		}
		else
		{
			// Read parameters for the Lead laser
			scanRow.leadLaser.travelerID = temp2;
			scanRow.leadLaser.syncOffset = 0.0;	// lead laser by definition has zero sync delay to itself
			scanRow.leadLaser.power = sheet4->Cell(rowNum, 3)->GetDouble();
			scanRow.leadLaser.spotSize = sheet4->Cell(rowNum, 4)->GetDouble();
			// Determine whether wobble is enabled for the lead laser
			string tempBool = parseToString(sheet4->Cell(rowNum, 5));
			if (as_lower(tempBool) == "on")
			{
				scanRow.leadLaser.wobble = true;	// also read the remaining wobble parameters
				scanRow.leadLaser.wobFrequency = sheet4->Cell(rowNum, 6)->GetDouble();
				scanRow.leadLaser.wobShape = sheet4->Cell(rowNum, 7)->GetInteger();
				scanRow.leadLaser.wobTransAmp = sheet4->Cell(rowNum, 8)->GetDouble();
				scanRow.leadLaser.wobLongAmp = sheet4->Cell(rowNum, 9)->GetDouble();
			}
			else { scanRow.leadLaser.wobble = false; }
			//
			// Determine if there is a trailing laser in this strategy.
			// If so, laserMode will be set to FollowMe rather than Independent
			temp2 = parseToString(sheet4->Cell(rowNum, 10));
			if (temp2 == "")
			{
				scanRow.laserMode = "Independent";  // This will cause trailing-laser parameters to be omitted from XML
				scanRow.trailLaser.travelerID = "";
				scanRow.trailLaser.syncOffset = 0.0;
				scanRow.trailLaser.power = 0;
				scanRow.trailLaser.spotSize = 0;
				scanRow.trailLaser.wobble = false;
			}
			else
			{
				// Read parameters for the Trailing laser
				scanRow.laserMode = "FollowMe";
				scanRow.trailLaser.travelerID = temp2;
				scanRow.trailLaser.syncOffset = sheet4->Cell(rowNum, 11)->GetDouble();
				scanRow.trailLaser.power = sheet4->Cell(rowNum, 12)->GetDouble();
				scanRow.trailLaser.spotSize = sheet4->Cell(rowNum, 13)->GetDouble();
				// Determine whether wobble is enabled for the trailing laser
				tempBool = parseToString(sheet4->Cell(rowNum, 14));
				if (as_lower(tempBool) == "on")
				{
					scanRow.trailLaser.wobble = true;	// also read the remaining wobble parameters
					scanRow.trailLaser.wobFrequency = sheet4->Cell(rowNum, 15)->GetDouble();
					scanRow.trailLaser.wobShape = sheet4->Cell(rowNum, 16)->GetInteger();
					scanRow.trailLaser.wobTransAmp = sheet4->Cell(rowNum, 17)->GetDouble();
					scanRow.trailLaser.wobLongAmp = sheet4->Cell(rowNum, 18)->GetDouble();
				}
				else { scanRow.trailLaser.wobble = false; }
			} // end else if trailing laser ID exists
		} // end else if lead laser ID exists

		  // Add this row to the segment style structure
		(*configData).segmentStyleList.push_back(scanRow);
		rowNum += 1;
		temp = parseToString(sheet4->Cell(rowNum, 0));	// Read next segment style row to decide whether to proceed
	}
	return;  // currently no return value, but could indicate error code or row# with data problems
}

void readRegionProfiles(BasicExcelWorksheet* sheet5, AMconfig* configData, string tabName)
{	// Read the region profile tab of an America Makes Excel configuration file.
	// This is called by AMconfigRead

	// First create a list of segment styles so that we can index from string-input ID to auto-generated integer ID's
	vector<string> segStyleList;
	for (int r = 0; r < (*configData).segmentStyleList.size(); r++)
	{	segStyleList.push_back((*configData).segmentStyleList[r].ID);	}

	// Read the region profiles on tab 5 to create an open-ended list
	// Loop until we run into a blank tag
	regionProfile regionRow;
	int rowNum = 6;	// first row of data - hard coded expectation
	string temp = parseToString(sheet5->Cell(rowNum, 0));
	while (temp != "")	// iterate until a blank row is reached
	{
		regionRow.Tag = temp;
		regionRow.vIDJump = parseToString(sheet5->Cell(rowNum, 1));
		// Contour parameters
		regionRow.contourStyleID = parseToString(sheet5->Cell(rowNum, 2)); // If blank, contours will be omitted for this region
		// Match contourStyleID to a specific segmentStyleID to determine the contourStyleIntID
		if (regionRow.contourStyleID == "")
		{
			regionRow.contourStyleIntID = -1;	// auto-generated integerID's start at 1, so -1 means no contours
		}
		else {
			// Identify the integerID which corresponds to contourStyleIntID
			std::vector<string>::iterator it = find(segStyleList.begin(), segStyleList.end(), regionRow.contourStyleID);
			regionRow.contourStyleIntID = std::distance(segStyleList.begin(), it) + 1;
		}
		regionRow.numCntr = sheet5->Cell(rowNum, 3)->GetInteger();
		regionRow.offCntr = sheet5->Cell(rowNum, 4)->GetDouble();
		regionRow.resCntr = sheet5->Cell(rowNum, 5)->GetDouble();
		regionRow.cntrSkywriting = sheet5->Cell(rowNum, 6)->GetInteger();
		// Hatch parameters
		regionRow.hatchStyleID = parseToString(sheet5->Cell(rowNum, 7)); // If blank, contours will be omitted for this region
		// Match hatchStyleID to a specific segmentStyleID to determine the contourStyleIntID
		if (regionRow.hatchStyleID == "")
		{
			regionRow.hatchStyleIntID = -1;	// auto-generated integerID's start at 1, so -1 means no hatches
		}
		else {
			// Identify the integerID which corresponds to hatchStyleID
			std::vector<string>::iterator it = find(segStyleList.begin(), segStyleList.end(), regionRow.hatchStyleID);
			regionRow.hatchStyleIntID = std::distance(segStyleList.begin(), it) + 1;
		}

		regionRow.offHatch = sheet5->Cell(rowNum, 8)->GetDouble();
		regionRow.resHatch = sheet5->Cell(rowNum, 9)->GetDouble();
		regionRow.hatchSkywriting = sheet5->Cell(rowNum, 10)->GetInteger();
		regionRow.scHatch = sheet5->Cell(rowNum, 11)->GetInteger();
		regionRow.layer1hatchAngle = sheet5->Cell(rowNum, 12)->GetDouble();
		regionRow.hatchLayerRotation = sheet5->Cell(rowNum, 13)->GetDouble();
		//
		// Add this row to the region-profile structure
		(*configData).regionProfileList.push_back(regionRow);
		rowNum += 1;
		temp = parseToString(sheet5->Cell(rowNum, 0));	// Read next row to decide whether to proceed
	}

	// After reading in all the region profiles, we need to create a "jump" segment style for
	// the velocity profileID found in each profile.  The SCAN XML requires a segment style for jumps,
	// rather than just a velocity profile ID
	string tempID;
	segmentStyle ssNew;
	// Generate a list of velocity profile ID strings to enable indexing to the position of a specific profile
	vector<string> vpList;
	for (int r = 0; r < (*configData).VPlist.size(); r++)
	{
		vpList.push_back((*configData).VPlist[r].ID);
	}
	// Iterate across all regions
	for (vector<regionProfile>::iterator r = (*configData).regionProfileList.begin(); r != (*configData).regionProfileList.end(); ++r)
	{
		// Define a segment style containing the jump velocity profile listed in region r
		// This profile will use an auto-generated integer ID equal to the size of the profile list.
		ssNew.integerID = (*configData).segmentStyleList.size() + 1;  // auto-populate a unique integerID for LabView
		ssNew.ID = "Auto-generated" + to_string(ssNew.integerID);
		ssNew.isUsed = true;
		ssNew.vpID = (*r).vIDJump;
		// Identify the vpIntID which corresponds to vpID
		std::vector<string>::iterator it = find(vpList.begin(), vpList.end(), ssNew.vpID);
		ssNew.vpIntID = std::distance(vpList.begin(), it) + 1;
		ssNew.laserMode = "";  // jump profiles don't need a laserMode value
		// push ssNew onto (*inputConfig).segmentStyleList
		(*configData).segmentStyleList.push_back(ssNew);
		// Cross-list the integerID of the new jump style onto the original region profile
		(*r).jumpStyleID = ssNew.ID;
		(*r).jumpStyleIntID = ssNew.integerID;
	}
	
	return;  // currently no return value, but error code could be returned in future
}

void readPartFiles(BasicExcelWorksheet* sheet6, AMconfig* configData, string tabName)
{	// Read the global parameters tab of an America Makes Excel configuration file.
	// This is called by AMconfigRead

	// Read the part files on tab 6.  This is an open-ended list which forms a vector of part files, vF.
	// Loop until we run into a blank filename
	ipFile ipf;
	int rowNum = 6;
	string temp = parseToString(sheet6->Cell(rowNum, 0));
	while (temp != "")
	{	// Filename is populated.  Add config-file path to filename, save it, and read the rest of the row
		ipf.fn = (*configData).configPath + "\\" + temp;
		// Read part offset values
		ipf.x_offset = sheet6->Cell(rowNum, 1)->GetDouble();
		ipf.y_offset = sheet6->Cell(rowNum, 2)->GetDouble();
		ipf.z_offset = sheet6->Cell(rowNum, 3)->GetDouble();
		ipf.Tag = parseToString(sheet6->Cell(rowNum, 4)); // Region profile ID
		//
		// Read optional contour and hatch trajectory assignment.  If either are left blank, they are set to a high value 
		// meaning that those elements will be built last
		ipf.contourTraj = sheet6->Cell(rowNum, 5)->GetInteger();
		ipf.hatchTraj = sheet6->Cell(rowNum, 6)->GetInteger();
		// Null values for trajectory number are read as zero; force them to a value
		if (ipf.contourTraj <= 0) { ipf.contourTraj = 9998; }	// default contour trajectory is built before default hatch trajectory	
		if (ipf.hatchTraj <= 0) { ipf.hatchTraj = 9999; }
		//
		// Add this part to the list
		(*configData).vF.push_back(ipf);	// Add this filename and data to the config structure
		//
		rowNum += 1;
		temp = parseToString(sheet6->Cell(rowNum, 0));	// Read next filename to decide whether to proceed
	}
	return;
}

void readTrajProcessing(BasicExcelWorksheet* sheet7, AMconfig* configData, string tabName)
{	// Read the trajectory processing tab of an America Makes Excel configuration file.
	// This is called by AMconfigRead

	// Loop until we run into a blank trajectory number
	trajectoryProc trajRow;
	int rowNum = 6;
	int tempInt = sheet7->Cell(rowNum, 0)->GetInteger();
	while (tempInt > 0)
	{
		trajRow.trajectoryNum = tempInt;
		trajRow.trajProcessing = parseToString(sheet7->Cell(rowNum, 1));  // will be forced to lower case
																		  // Perform simple error-checking on the trajectory processing; set to sequential if there's an issue
		if ((trajRow.trajProcessing != "sequential") & (trajRow.trajProcessing != "concurrent"))
		{
			trajRow.trajProcessing = "sequential";
		}
		// Add this processing rule to the trajectory-processing structure
		(*configData).trajProcList.push_back(trajRow);
		rowNum += 1;
		tempInt = sheet7->Cell(rowNum, 0)->GetInteger();	// Read next trajectory-processing row, to decide whether to proceed
	}
	return;
}

void readStripes(BasicExcelWorksheet* sheet8, AMconfig* configData, string tabName)
{
	// Read the optional single stripes on tab 8.  If the list is blank, (*configData).stripeList will be empty 
	// and allStripesMarked set to true (so that stripes will be ignored when writing scan XML).
	// Otherwise, stripeList will contain one or more line-segment stripes and allStripesMarked will be set false
	try {
		// See if there is a segment style populated on the first input row
		int rowNum = 6;
		string temp = parseToString(sheet8->Cell(rowNum, 2));  // column C
		if (temp != "") {
			// At least one stripe is present.  Read the global stripe jump profile and skywriting mode values before reading the stripe list

			// read and process jump profile and skywriting mode
			(*configData).stripeJumpVPID = parseToString(sheet8->Cell(4, 2));
			(*configData).stripeSkywrtgMode = (int)sheet8->Cell(4, 3)->GetDouble();
			//
			// Create a segment style referencing stripeJumpVID
			segmentStyle ssNew;
			string tempID;
			ssNew.integerID = (*configData).segmentStyleList.size() + 1;  // auto-populate a unique integerID for LabView
			ssNew.ID = "Auto-generated" + to_string(ssNew.integerID);
			ssNew.isUsed = true;
			// Identify the integer ID of stripeJumpVID
			vector<string> vpList;
			for (int r = 0; r < (*configData).VPlist.size(); r++)
			{
				vpList.push_back((*configData).VPlist[r].ID);
			}
			ssNew.vpID = (*configData).stripeJumpVPID;
			std::vector<string>::iterator it = find(vpList.begin(), vpList.end(), ssNew.vpID);  // Identify vpIntID which corresponds to vpID
			ssNew.vpIntID = std::distance(vpList.begin(), it) + 1;
			ssNew.laserMode = "";  // jump profiles don't need a laserMode value
			// push ssNew onto (*inputConfig).segmentStyleList
			(*configData).segmentStyleList.push_back(ssNew);
			// Cross-list the integerID of the new jump segment style onto the stripe configuration
			(*configData).stripeJumpSegStyleID = ssNew.ID;
			(*configData).stripeJumpSegStyleIntID = ssNew.integerID;

			// set up a temporary stripe structure and list of segment styles for referencing
			singleStripe stripe;
			double stripeLayerHeight = 0.0;  // holds stripe height value read from file, which may be empty
			// Create a list of segment styles so that we can index from string-input ID to auto-generated integer ID's
			vector<string> segStyleList;
			for (int r = 0; r < (*configData).segmentStyleList.size(); r++)
			{
				segStyleList.push_back((*configData).segmentStyleList[r].ID);
			}

			//*** Read the stripe list
			// we already read the segment style for the first stripe (to determine if any stripes are present), so
			// process that row and continue until we hit an unpopulated stripe row
			while (temp != "")
			{	// SegmentStyleID is populated.  Save this and read the rest of the row
				(*configData).allStripesMarked = false;  // we have at least one stripe, so we'll indicate that they need to be marked
				stripe.segmentStyleID = temp;
				// Identify the integer segment style ID which corresponds to temp
				std::vector<string>::iterator it2 = find(segStyleList.begin(), segStyleList.end(), temp);
				stripe.segmentStyleIntID = std::distance(segStyleList.begin(), it2) + 1;

				// Read the stripe trajectory#
				stripe.trajectoryNum = sheet8->Cell(rowNum, 0)->GetInteger();
				// Null values for trajectory number are read as zero; force them to a value
				if (stripe.trajectoryNum > 0) { stripe.trajectoryNum = 0; }	// stripe trajectory#'s should be <0, because stripes are built before parts. 0=default if blank

				// Read the stripe ID
				stripe.stripeID = parseToString(sheet8->Cell(rowNum, 1));

				// Read start/end coordinates
				stripe.startX = sheet8->Cell(rowNum, 3)->GetDouble();
				stripe.startY = sheet8->Cell(rowNum, 4)->GetDouble();
				stripe.endX = sheet8->Cell(rowNum, 5)->GetDouble();
				stripe.endY = sheet8->Cell(rowNum, 6)->GetDouble();
				
				// Read stripe layer height value and process into a layer interval value
				stripeLayerHeight = sheet8->Cell(rowNum, 7)->GetDouble();
				if (stripeLayerHeight <= (*configData).layerThickness_mm) {
					stripeLayerHeight = (*configData).layerThickness_mm;  // if value is blank it will be read as 0
				}
				stripe.stripeLayerNum = (int)floor(stripeLayerHeight / (*configData).layerThickness_mm);  // compute layer at which this stripe will be marked
				if (stripe.stripeLayerNum < 1) { stripe.stripeLayerNum = 1; }
				//
				// Add this stripe to the list
				(*configData).stripeList.push_back(stripe);	// Add this filename and data to the config structure
				//
				rowNum += 1;
				temp = parseToString(sheet8->Cell(rowNum, 2));	// Read next segmentStyleID to decide whether to proceed
			}
			// All stripes read
		}
	}
	catch (...) {
		// Stripes tab is not included in this config file
		(*configData).allStripesMarked = true;  // stripes will not be evaluated on any layer
	}

	return;
}

AMconfig AMconfigRead(const std::string& configFilename)
{
	// Read the America Makes configuration file specified by configFilename and store in an AMconfig structure.
	// This function proceeds tab by tab and populates vectors without error-checking (yet) to ensure all ID references are valid.
	// The config file version on tab 1 must match the version# defined in constexpr auto AMversionNum

	BasicExcel excelFile;
	AMconfig configData;	// create the output structure
	string tabName;

	errorCheckStructure errorData;  // set up error reporting structure
	bool haltNow = true;  // any error encountered will result in immediate quit and report
	string errorMsg;
	string functionWithIssue;
	// save config filename and path in configData
	configData.configFilename = configFilename.c_str();  // full path to the file along with the filename itself
	size_t slashPos = configFilename.find_last_of("\\");
	configData.configPath = configFilename.substr(0, slashPos);
	// try and load the file via BasicExcel
	try {
		bool openFileSuccess;
		openFileSuccess = excelFile.Load(configFilename.c_str());	// load the indicated Excel file.  Note - try/catch won't properly handle a runtime error in this module
		if (!openFileSuccess) {
			errorMsg = "Unable to open the configuration file - it may be open in Excel (please close and re-try)";
			updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, "", configData.configFilename, configData.configPath);
		}
	}
	catch (...) { // fatal error - can't open the config file
		errorMsg = "Unable to open the Excel configuration file";
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, "", configData.configFilename, configData.configPath);
	}
	//***********************************
	// Open tab 1 to get the config file version
	BasicExcelWorksheet* oneSheet;
	tabName = configTabNames[0];
	try {
		oneSheet = excelFile.GetWorksheet(tabName.c_str());
		if (oneSheet == 0) { throw(1); }
	}
	catch (...) { // fatal error - can't find tab #1
		errorMsg = "Config file does not contain tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, tabName, configData.configFilename, configData.configPath);
	}

	// Verify that the config file's version number is compatible with this code
	string versionString = "";
	try {
		versionString = parseToString(oneSheet->Cell(1, 1));	// version# should be an integer, but read as if a string in case we later change to double, e.g. 2.1
	}
	catch (...) { // fatal error - can't read version#
		haltNow = true;
		errorMsg = "Unable to read config-file version number on tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, "", configData.configFilename, configData.configPath);
	}
	
	if (versionString != to_string(AMconfigFileVersion))
	{
		errorMsg = "Incompatible config file:  this code requires version " + to_string(AMconfigFileVersion) + " but config file is version " + versionString;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, "", configData.configFilename, configData.configPath);
	}
	else
	{
		configData.fileVersion = AMconfigFileVersion;
		configData.validConfigFile = true;	// config file version matches this code; proceed to read the rest of the file
	}
	
	//***********************************************************************
	// READ REMAINING TABS
	
	// Read general parameters on tab 2
	try {
		tabName = configTabNames[1];
		oneSheet = excelFile.GetWorksheet(tabName.c_str());
		if (oneSheet == 0) { throw(1); }
		readGeneralParameters(oneSheet, &configData, tabName);
	}
	catch (...) {
		errorMsg = "Could not read tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, tabName, configData.configFilename, configData.configPath);
	}
	
	// Read velocity profiles on tab 3.  Open-ended list which forms a vector of velocityProfile
	try {
		tabName = configTabNames[2];
		oneSheet = excelFile.GetWorksheet(tabName.c_str());
		if (oneSheet == 0) { throw(1);  }
		readVelocityProfiles(oneSheet, &configData, tabName);
	}
	catch (...) {
		errorMsg = "Could not read tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, tabName, configData.configFilename, configData.configPath);
	}
	
	// Read SegmentStyles on tab 4.  This is an open-ended list which forms segmentStyleList
	try {
		tabName = configTabNames[3];
		oneSheet = excelFile.GetWorksheet(tabName.c_str());
		if (oneSheet == 0) { throw(1); }
		readSegmentStyles(oneSheet, &configData, tabName);
	}
	catch (...) {
		errorMsg = "Could not read tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, tabName, configData.configFilename, configData.configPath);
	}
	
	// Read the region profiles on tab 5 to form an open-ended list
	try {
		tabName = configTabNames[4];
		oneSheet = excelFile.GetWorksheet(tabName.c_str());
		if (oneSheet == 0) { throw(1); }
		readRegionProfiles(oneSheet, &configData, tabName);
	}
	catch (...) {
		errorMsg = "Could not read tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, tabName, configData.configFilename, configData.configPath);
	}
	
	// Read the part files on tab 6.  This is an open-ended list which forms a vector of part files, vF.
	try {
		tabName = configTabNames[5];
		oneSheet = excelFile.GetWorksheet(tabName.c_str());
		if (oneSheet == 0) { throw(1); }
		readPartFiles(oneSheet, &configData, tabName);
	}
	catch (...) {
		errorMsg = "Could not read tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, tabName, configData.configFilename, configData.configPath);
	}
	
	// Read trajectory processing instructions on tab 7.  This is an open-ended list which forms scanStrategyList
	try {
		tabName = configTabNames[6];
		oneSheet = excelFile.GetWorksheet(tabName.c_str());
		if (oneSheet == 0) { throw(1); }
		readTrajProcessing(oneSheet, &configData, tabName);
	}
	catch (...) {
		errorMsg = "Could not read tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, tabName, configData.configFilename, configData.configPath);
	}
	
	// Read optional single-stripe marks on tab 8.  Open-ended list which forms stripeList
	try {
		tabName = configTabNames[7];
		oneSheet = excelFile.GetWorksheet(tabName.c_str());
		if (oneSheet == 0) { throw(1); }
		readStripes(oneSheet, &configData, tabName);
	}
	catch (...) {
		errorMsg = "Could not read tab " + tabName;
		updateErrorResults(errorData, haltNow, "AMconfigRead", errorMsg, tabName, configData.configFilename, configData.configPath);
	}

	//***********************************************************************
	// CONFIG FILE READ COMPLETE
	// We will not call the error check function here, because that would repeat all checks each time the file is read (every 25 genLayer/genScan iterations)
	// Instead we assume that generateScanpaths calls evaluateConfigFile the first time the config file is read
	//evaluateConfigFile(configData, errorData);

	return configData;
}
