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
errorChecks.cpp contains a series of error checks to be performed on
the Excel configuration file (evaluateConfigFile) as well as a
simple error aggregation and reporting routine (updateErrorResults)

It is utilized by all three projects within the ALSAM3024 solution
//============================================================*/

#include "errorChecks.h"
#include "constants.h"

// Function to check configuration-file for errors.
// Should be called after reading the file into an AMconfig structure.
// Function result indicates whether an error was found (true=one or more errors found, false=ok to proceed)
void evaluateConfigFile(AMconfig &configData, errorCheckStructure &errorData)
{
	bool haltNow = true;  // assume all errors are fatal unless otherwise noted
	string errorMsg = "";
	vector<string> missingItems;

	//*** 1. Check if any tabs were reported missing by AMconfigRead, other than the single-stripe tab.  If so, call a fatal error and halt here
	if (errorData.missingConfigTabs.size() > 0) {
		errorMsg = "One or more tabs are missing from the Excel configuration file, including " + errorData.missingConfigTabs[0];
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", errorMsg, "", configData.configFilename, configData.configPath);
	}

	// To assist the checks for each tab of the config file, 
	// set up vectors containing names of all velocity profiles, segment styles and regions
	vector<string> VPtags, SStags, RegionTags;
	for (int i = 0; i < configData.VPlist.size(); i++) {
		VPtags.push_back(configData.VPlist[i].ID);
	}
	for (int i = 0; i < configData.segmentStyleList.size(); i++) {
		SStags.push_back(configData.segmentStyleList[i].ID);
	}
	for (int i = 0; i < configData.regionProfileList.size(); i++) {
		RegionTags.push_back(configData.regionProfileList[i].Tag);
	}

	// Evaluate config-file tabs in reverse order, because lists such as segment styles were concatenated as the tabs were read,
	// which means that the elements from later tabs can't be differentiated from earlier tabs unless we check in reverse

	//*** 8. Single-stripe checks:  (if any are present)
	//	jump velocity profile exists
	//  all ss's exist
	//	all stripe trajectory#'s are <=0
	//	all stripes are non-zero in at least one direction (x or y)
	//	stripe heights are >=0
	if (configData.stripeList.size() > 0) {
		// check that stripe jump velocity profile exists
		vector<string> vplist;  // check function requires a vector of strings, not an individual string
		vplist.push_back(configData.stripeJumpVPID);
		missingItems = checkExistanceInList(VPtags, vplist);
		if (missingItems.size() > 0) {
			updateErrorResults(errorData, haltNow, "evaluateConfigFile", "The jump profile listed for single-stripes on tab 8 is not listed on tab 3: " + missingItems[0], "", configData.configFilename, configData.configPath);
		}
		// check that stripe segment styles exist
		vector<string> stripeSegStyles;  // Segment styles used by single-stripes
		for (int i = 0; i < configData.stripeList.size(); i++) {
			stripeSegStyles.push_back(configData.stripeList[i].segmentStyleID);
		}
		missingItems = checkExistanceInList(SStags, stripeSegStyles);
		if (missingItems.size() > 0) {
			updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one segment style referenced for single stripes on tab 8 is not listed on tab 4, including " + missingItems[0], "", configData.configFilename, configData.configPath);
		}
		// iterate across stripes (again) to check trajectory#'s, lengths and heights
		double xDim, yDim;
		for (int i = 0; i < configData.stripeList.size(); i++) {
			xDim = abs(configData.stripeList[i].startX - configData.stripeList[i].endX);
			yDim = abs(configData.stripeList[i].startY - configData.stripeList[i].endY);
			if (xDim + yDim <= 0.001) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one single stripe on tab 8 has no x or y length.  Stripe# " + i, "", configData.configFilename, configData.configPath);
			}
			if (configData.stripeList[i].stripeLayerNum < 1) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one single stripe on tab 8 has negative z-coordinate.  Stripe# " + i, "", configData.configFilename, configData.configPath);
			}
			if (configData.stripeList[i].trajectoryNum > 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one single stripe on tab 8 has trajectory# > 0.  Stripe# " + i, "", configData.configFilename, configData.configPath);
			}
		}
	}

	//*** 7. BuildOrder checks:
	//	(potential warning only) all trajectory numbers are listed on Parts tab
	//	trajectory processing values are either sequential or concurrent
	vector<string> trajProcValues;  // list of trajectory processing values
	for (int i = 0; i < configData.trajProcList.size(); i++) {
		trajProcValues.push_back(configData.trajProcList[i].trajProcessing);
	}
	missingItems = checkExistanceInList({ "sequential", "concurrent" }, trajProcValues);
	if (missingItems.size() > 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "The trajectory-processing tab contains something unrecognized (should be sequential or concurrent).  Value is " + missingItems[0], "", configData.configFilename, configData.configPath);
	}

	//*** 6. Part checks:
	//	(potential warning only) At least one part is present
	//	all region profiles exist
	//	part files all end in .stl
	//	x/y offsets are within reason
	//	trajectory#'s are >0
	vector<string> regionsUsedByParts;  // list of regions tags referenced by all parts
	for (int i = 0; i < configData.vF.size(); i++) {
		regionsUsedByParts.push_back(configData.vF[i].Tag);
	}
	missingItems = checkExistanceInList(RegionTags, regionsUsedByParts);
	if (missingItems.size() > 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one region referenced for a part on tab 6 is not listed on tab 5, including " + missingItems[0], "", configData.configFilename, configData.configPath);
	}
	// Now iterate across parts (again) to do the remaining checks
	for (int i = 0; i < configData.vF.size(); i++) {
		//	part files all end in .stl
		//	x/y offsets are within reason
		//	trajectory#'s are >0
		int sz = configData.vF[i].fn.size();
		if (configData.vF[i].fn.substr(sz - 4, 4) != ".stl") {
			updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one part file listed on tab 6 does not end in .stl, including " + configData.vF[i].fn, "", configData.configFilename, configData.configPath);
		}
		if (abs(configData.vF[i].x_offset) > 400) {
			updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one part file listed on tab 6 has an extreme x-offset, including " + configData.vF[i].fn, "", configData.configFilename, configData.configPath);
		}
		if (abs(configData.vF[i].y_offset) > 400) {
			updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one part file listed on tab 6 has an extreme y-offset, including " + configData.vF[i].fn, "", configData.configFilename, configData.configPath);
		}
		if (abs(configData.vF[i].z_offset) > 1500) {
			updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one part file listed on tab 6 has an extreme z-offset, including " + configData.vF[i].fn, "", configData.configFilename, configData.configPath);
		}
		if (configData.vF[i].contourTraj < 1) {
			updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one part file listed on tab 6 has contour trajectory# < 1, including " + configData.vF[i].fn, "", configData.configFilename, configData.configPath);
		}
		if (configData.vF[i].hatchTraj < 1) {
			updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one part file listed on tab 6 has hatch trajectory# < 1, including " + configData.vF[i].fn, "", configData.configFilename, configData.configPath);
		}
	}

	//*** 5. Region profile checks:
	//	don't need to check that at least one region exists, since single stripes could be used in place of parts and regions
	//	regions reference valid jump vp's and contour/hatch ss's
	//	warning if region contains neither hatch nor contour ss (or error?)
	//	if a contour style is listed:  number of contours >0, contour spacing >0; contour offset >=0; skywriting mode >=0
	//	hatch spacing > 0; hatch offset >=0; skywriting mode >=0
	vector<string> regionVPid;  // generate a list of velocity profiles referenced by regions for jumps
	for (int i = 0; i < configData.regionProfileList.size(); i++) { regionVPid.push_back(configData.regionProfileList[i].vIDJump); }
	missingItems = checkExistanceInList(VPtags, regionVPid);
	if (missingItems.size() > 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one velocity profile referenced on tab 5 (jump style by region) is not listed in the VP list on tab 3, including " + missingItems[0], "", configData.configFilename, configData.configPath);
	}
	vector<string> contourSegStyles, hatchSegStyles;  // Segment styles used by hatch and contour styles
	for (int i = 0; i < configData.regionProfileList.size(); i++) {
		if (configData.regionProfileList[i].contourStyleID != "") {
			contourSegStyles.push_back(configData.regionProfileList[i].contourStyleID);
		}
		if (configData.regionProfileList[i].hatchStyleID != "") {
			hatchSegStyles.push_back(configData.regionProfileList[i].hatchStyleID);
		}
	}
	missingItems = checkExistanceInList(SStags, contourSegStyles);
	if (missingItems.size() > 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one segment style referenced for contours on tab 5 is not listed on tab 4, including " + missingItems[0], "", configData.configFilename, configData.configPath);
	}
	missingItems = checkExistanceInList(SStags, hatchSegStyles);
	if (missingItems.size() > 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one segment style referenced for hatches on tab 5 is not listed on tab 4, including " + missingItems[0], "", configData.configFilename, configData.configPath);
	}
	// contour and hatch checks - for styles which include either or both features
	for (int i = 0; i < configData.regionProfileList.size(); i++) {
		if (configData.regionProfileList[i].contourStyleID != "") {
			// contour checks: number of contours >0, contour spacing >0; contour offset >=0; skywriting mode >=0
			if (configData.regionProfileList[i].numCntr <= 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Contour count is zero or less for region " + configData.regionProfileList[i].Tag + ". Must delete contour segment style to avoid contours", "", configData.configFilename, configData.configPath);
			}
			if (configData.regionProfileList[i].offCntr < 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Contour offset is < zero for region " + configData.regionProfileList[i].Tag, "", configData.configFilename, configData.configPath);
			}
			if (configData.regionProfileList[i].resCntr < 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Contour spacing is < zero for region " + configData.regionProfileList[i].Tag, "", configData.configFilename, configData.configPath);
			}
			if (configData.regionProfileList[i].cntrSkywriting < 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Contour skywriting mode is <0, not a recognized mode, for region " + configData.regionProfileList[i].Tag, "", configData.configFilename, configData.configPath);
			}
		}
		if (configData.regionProfileList[i].hatchStyleID != "") {
			// hatch checks: hatch spacing > 0; hatch offset >=0; skywriting mode >=0
			if (configData.regionProfileList[i].resHatch <= 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Hatch spacing <= 0 for region " + configData.regionProfileList[i].Tag, "", configData.configFilename, configData.configPath);
			}
			if (configData.regionProfileList[i].offHatch < 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Hatch offset < 0 for region " + configData.regionProfileList[i].Tag, "", configData.configFilename, configData.configPath);
			}
			if (configData.regionProfileList[i].hatchSkywriting < 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Hatch skywriting mode is <0, not a recognized mode, for region " + configData.regionProfileList[i].Tag, "", configData.configFilename, configData.configPath);
			}
		}
	}
	
	//*** 4. Segment style checks:
	//	at least one ss exists
	//	all ss reference valid vp's
	//	if lasers are listed, power must be >=0, spot size > 30
	//	if trailing laser is populated, it must have different ID than the lead laser, sync > 0 plus same checks as lead laser
	if (configData.segmentStyleList.size() == 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "No segment styles are listed on config file tab 4", "", configData.configFilename, configData.configPath);
	}
	vector<string> ssVPid;  // generate a list of velocity profiles referenced by segment styles
	for (int i = 0; i < configData.segmentStyleList.size(); i++) { ssVPid.push_back(configData.segmentStyleList[i].vpID); }
	missingItems = checkExistanceInList(VPtags, ssVPid);
	if (missingItems.size() > 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "At least one velocity profile referenced on tab 4 is not listed in the VP list on tab 3, including " + missingItems[0], "", configData.configFilename, configData.configPath);
	}
	// Go through the traveler sections of each segment style and evaluate parameters.  
	// Complicated, because each style may include zero, one or two traveler sections
	for (int i = 0; i < configData.segmentStyleList.size(); i++) {
		if (configData.segmentStyleList[i].leadLaser.travelerID != "") { // lead laser is populated; evalute parameters
			if (configData.segmentStyleList[i].leadLaser.power < 0.0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Lead laser power is < 0 for segment style " + configData.segmentStyleList[i].ID, "", configData.configFilename, configData.configPath);
			}
			if (configData.segmentStyleList[i].leadLaser.spotSize < 30) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Lead laser spot size power is < 30um for segment style " + configData.segmentStyleList[i].ID, "", configData.configFilename, configData.configPath);
			}
		}
		if (configData.segmentStyleList[i].trailLaser.travelerID != "") { // trailing laser is populated; evalute parameters
			if (configData.segmentStyleList[i].trailLaser.power < 0.0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Trailing laser power is < 0 for segment style " + configData.segmentStyleList[i].ID, "", configData.configFilename, configData.configPath);
			}
			if (configData.segmentStyleList[i].trailLaser.spotSize < 30) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Trailing laser spot size power is < 30um for segment style " + configData.segmentStyleList[i].ID, "", configData.configFilename, configData.configPath);
			}
			if (configData.segmentStyleList[i].trailLaser.syncOffset < 0) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Trailing laser sync delay is < 0us for segment style " + configData.segmentStyleList[i].ID, "", configData.configFilename, configData.configPath);
			}
			if (configData.segmentStyleList[i].leadLaser.travelerID == "") {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Trailing laser is populated - but lead laser is not - for segment style " + configData.segmentStyleList[i].ID, "", configData.configFilename, configData.configPath);
			}
			if (configData.segmentStyleList[i].leadLaser.travelerID == configData.segmentStyleList[i].trailLaser.travelerID) {
				updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Lead and trailing lasers have the same ID for segment style " + configData.segmentStyleList[i].ID, "", configData.configFilename, configData.configPath);
			}
		}
	}
	
	//*** 3. Velocity profile checks:
	//	at least one vp exists
	//	all vp's have velocity >0
	// CHECKS TO ADD:  1. Mode value is Delay or Auto.  2) all delays are real-valued and in a reasonable range
	if (configData.VPlist.size() == 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "No velocity profiles are listed on config file tab 3", "", configData.configFilename, configData.configPath);
	}
	vector<double> VPvelocities;
	for (int i = 0; i < configData.VPlist.size(); i++) {
		VPvelocities.push_back(configData.VPlist[i].velocity);
	}
	missingItems = checkForFloatErrors(VPtags, VPvelocities, 1.0);
	if (missingItems.size() > 0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "One or more velocity profiles on config file tab 3 has velocity < 1, including " + missingItems[0], "", configData.configFilename, configData.configPath);
	}
	
	//*** 2. General tab checks:
	//	all numerical values are in a valid range
	if (configData.projectFolder == "") {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Project folder name is blank on config file tab 2", "", configData.configFilename, configData.configPath);
	}
	if (configData.layerThickness_mm < 0.01) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Layer thickness is less than 0.01 mm on config file tab 2", "", configData.configFilename, configData.configPath);
	}
	if (configData.dosingFactor < 1.0) {
		updateErrorResults(errorData, haltNow, "evaluateConfigFile", "Dosing factor is < 1.0 on config file tab 2", "", configData.configFilename, configData.configPath);
	}
	
	// REVIEW WORD DOC FOR ANY ADDITIONAL CHECKS
}

// Function to update the error results structure.
// Calling this function automatically sets errorFound to true.
// errorMsg will be appended to the fullResults, but shortResult will only be updated if it is currently blank
void updateErrorResults(errorCheckStructure &errorData, bool haltNow, string functionWithIssue, string errorMsg, string missingTab, string configFilename, string configPath)
{
	// errorData is a structure containing checks and results
	// if haltNow is true, this function attempts to write to an output file and quit execution
	// Otherwise, errorMsg will be appended to the running list of errors identified
	// configFilename is needed only to include the config filename in the error-results file
	// configPath is the folder containing the config file, in which any error report should be written

	errorData.errorFound = true;
	if (errorData.firstError == "") { errorData.firstError = errorMsg; }
	errorData.fullErrorList.push_back(functionWithIssue + ": " + errorMsg);

	// Determine whether to write everything to the error filename and halt immediately
	if (haltNow == true) {
		// write to console
		cout << "\n***** FATAL ERROR ENCOUNTERED *****\n" << endl;
		cout << "  " << errorMsg << endl;
		cout << "  Function reporting error: " << functionWithIssue << endl << endl;
		cout << "Execution will be cancelled" << endl;

		// attempt to write findings to error file in the config folder
		string fullErrFilepath = configPath + "\\" + errorReportFilename;
		ofstream errFile(fullErrFilepath.c_str());
		if (!errFile) {
			// Could not open the error output file
			cout << endl << "Could not open the error-report file listed below to write the error; it may be in use\n";
			cout << fullErrFilepath << endl;
		}
		else {
			cout << "See " << fullErrFilepath.c_str() << " for more information" << endl;
			errFile << "ALSAM scanpath-generation error report\n";
			// List config filename and current timestamp in the file
			time_t now = time(0);
			char str[26];
			ctime_s(str, sizeof str, &now);   // convert time to string form
			errFile << str << endl;
			errFile << "Configuration file: " << configFilename << "\n\nError(s) identified:\n";
			for (int x = 0; x < errorData.fullErrorList.size(); x++) {
				errFile << errorData.fullErrorList[x].c_str() << endl;
			}
			errFile.close();
		}
		// halt or quit execution
		system("pause");
		exit(-1);
	}
}

// Error-check helper function which returns a null vector if all values of valuesToFind are in referenceValues, otherwise returns vector of missing values
vector<string> checkExistanceInList(vector<string> referenceValues, vector<string> valuesToFind)
{
	vector<string> valuesNotFound;
	std::vector<string>::iterator it;

	for (int i = 0; i < valuesToFind.size(); i++) {
		// determine if valuesToFind[i] is contained in referenceValues
		// if not, append this tag onto valuesNotFound
		if (valuesToFind[i] > "") {  // don't search for null values
			it = std::find(referenceValues.begin(), referenceValues.end(), valuesToFind[i]);
			if (it == referenceValues.end()) {
				// value not found!
				valuesNotFound.push_back(valuesToFind[i]);
			}
		}
	}
	return valuesNotFound;
}

// Error-check helper function which returns a null vector if all values are >= minima, otherwise returns vector of tags whose values are < minima
vector<string> checkForFloatErrors(vector<string> tags, vector<double> values, double minima)
{
	vector<string> tagsBelowMinima;
	for (int i = 0; i < values.size(); i++) {
		if (values[i] < minima) {
			tagsBelowMinima.push_back(tags[i]);
		}
	}
	return tagsBelowMinima;
}

// Error-check helper function which returns a null vector if all values are >= minima, otherwise returns vector of tags whose values are < minima
vector<string> checkForIntErrors(vector<string> tags, vector<int> values, int minima)
{
	vector<string> tagsBelowMinima;
	for (int i = 0; i < values.size(); i++) {
		if (values[i] < minima) {
			tagsBelowMinima.push_back(tags[i]);
		}
	}
	return tagsBelowMinima;
}

