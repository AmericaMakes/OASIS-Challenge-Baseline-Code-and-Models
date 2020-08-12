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
readExcelConfig.h defines the data structure and the functions needed
to read the Excel-based America Makes configuration input
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
#include <algorithm>
#include <vector>
#include <cctype>
#include "BasicExcel.hpp"

using namespace YExcel;

const vector<string> configTabNames = { "1.Header", "2.General", "3.VelocityProfiles", "4.SegmentStyles", "5.Regions", "6.Parts", "7.PathProcessing", "8.Stripes" };

// Structure to contain data on each STL file indicated in the configuration file
struct ipFile
{
	string fn;	// filename
	double x_offset = 0, y_offset = 0, z_offset = 0; //user assigned offsets, in mm
	string Tag; // Selects a Region Profile (combination of contour/hatch segment styles, offsets and spacings) for the part
	//
	// Trajectory ordering.  If user omits an order for hatches or contours, this part will be built after others
	int contourTraj = 9998;	// Permits grouping and ordering of regions from different parts
	int hatchTraj = 9999;	// Permits grouping and ordering of regions from different parts
};

// Structure to contain a single row from the Velocity Profiles tab
struct velocityProfile
{
	string ID = "";			// ID of velocity profile
	int integerID = 0;		// Auto-generated ID to be passed to LabView (which may not handle the string ID's)
	bool isUsed = true;		// indicates whether this velocity profile is actually referenced by any SegmentStyles in the build
	double velocity = 0.0;	// laser velocity, mm/s
	string mode = "Delay";	// mode in which the velocity profile is specified.  Can be Delay, Acceleration, Auto
	double laserOnDelay = 0.0;
	double laserOffDelay = 0.0;
	double jumpDelay = 0.0;
	double markDelay = 0.0;
	double polygonDelay = 0.0;
};

// Structure to define parameters for an individual laser within a SegmentStyle.
// This is equivalent in format to the Traveler portion of the SegmentStyle XML schema,
// and is only referenced as a component of a SegmentStyle (not stand-alone).
struct traveler
{
	string travelerID = "";		// References a specific laser.  User-input string will be cast to lower case
	double syncOffset = 0.0;	// Sync offset from lead laser, for FollowMe mode.  Lead (or single) laser will have 0 delay
	double power = 0.0;			// Mark power, watts
	double spotSize = 50.0;		// Spot size, microns
	bool wobble = false;		// Indicates whether wobble is used in this style.  If false, ignore remaining parameters
	double wobFrequency = 0.0;	// Wobble frequency
	int wobShape = 0;			// Selects elliptical (-1, 1) or oval (0) shape
	double wobTransAmp = 0.0;	// Wobble transverse amplitude, mm (perpendicular to laser travel direction)
	double wobLongAmp = 0.0;	// Wobble longitudinal amplitude, mm (in direction of laser travel)
};

// Structure to contain a single row from the SegmentStyles tab.
// This is equivalent in format to the SegmentStyle portion of the SCAN schema, except that
// no more than two lasers (lead and trailing) can be included in Follow-Me mode.
// This is consistent with the current Excel input, which has fields for two lasers.
// If >2 Follow-Me lasers are needed, the two traveler fields could be replaced with vector<traveler>
struct segmentStyle
{
	string ID = "";			// Identifies the SegmentStyle, which may be used by many parts or regions (contours, hatches)
	int integerID = 0;		// Auto-generated ID to be passed to LabView (which may not handle the string ID's)
	bool isUsed = true;		// indicates whether this SegmentStyle is actually referenced by any region profiles in the build
	string vpID = "";		// Velocity profile to be used for segments that specify this style.
								// Note 1: when alternating hatches with jumps, XML will alternate between two different SegmentStyles.
								// Note 2: if this style is a jump (no power), the remaining fields can be left blank.
								//	This will cause the most-recently-used laser(s) to execute the jump.
	int vpIntID = 0;		// index to velocity profile (eliminates need to match up string vpID against all vpID's)
	string laserMode = "";	// Blank means no laser is explicitly selected.  Other values are {Independent, FollowMe}
	traveler leadLaser;		// (optional) Parameters for the lead laser in Independent or FollowMe mode.  May be omitted for jump-only styles
	traveler trailLaser;	// (optional) Parameters for the trailing laser, in FollowMe mode only.
};

// Structure to define jump, contour and hatch parameters and SegmentStyles for a particular region.
// The contour and hatch sections of a region will be implemented as separate paths in XML,
// and each part that uses the same regionProfile will be created as separate contour and hatch paths.
struct regionProfile
{
	string Tag = "";			// Identifies the region profile
	bool isUsed = true;			// indicates whether this region profile is actually referenced by any parts in the build
	//
	string vIDJump = "";		// ID of velocity profile for jumps between hatches and contour segments
	string jumpStyleID = "";	// ID of a jump segment style which will be automatically created in genScan based on vIDJump for this region
	int jumpStyleIntID = 0;		// Integer ID corresponding to jumpStyleID for this region
	//
	// Contour parameters
	string contourStyleID = "";	// Selects a SegmentStyle for contours.  Blank = omit contours
	int contourStyleIntID = 0;
	double offCntr = 0.0;		// Offset of the first contour from actual part outline, in um.  Positive = indented
	int numCntr = 0;			// Number of contours.  0 = omit contours
	double resCntr = 0.0;		// Center-to-center spacing (resolution) for multiple contours, in um.  Irrelevant for <2 contours
	int cntrSkywriting = 0;		// Skywriting mode for contours.  0 = off
	//
	// Hatch parameters
	string hatchStyleID = "";	// Selects a SegmentStyle for hatches.  Blank = omit hatches
	int hatchStyleIntID = 0;
	double offHatch = 0.0;		// offset of the hatches as measured from innermost contour, in um.  Positive = indented
	double resHatch = 0.0;		// hatch center-to-center spacing (resolution) in um
	int hatchSkywriting = 0;	// Skywriting mode for hatches.  0 = off
	int scHatch = 0;			// hatch scheme selection.  0=basic hatching, 1=prototype optimization (minimize jumps)
	double layer1hatchAngle = 0.0;	 // hatch angle to be used on layer 1 for this region
	double hatchLayerRotation = 0.0; // incremental change in hatch angle (counter-clockwise) per layer
};

// Structure to hold definitions for trajectory processing (sequential or concurrent)
struct trajectoryProc
{
	int trajectoryNum = 1;	// identity of a particular trajectory.  single-stripe trajectories are excluded
	bool isUsed = true;		// indicates whether this trajectoryNum is actually referenced by any parts in the build
	string trajProcessing = "sequential";  // valid is {concurrent, sequential}.  Forced to lower case
};

// Structure to hold definition of a single line-segment stripe (for bead on plate experiments).
// These elements will be marked only once, on the single layer indicated on stripeLayerNum
struct singleStripe
{
	int trajectoryNum = 0;		// groups and orders the stripes.  Stripe Trajectory#'s from config file should be <0.  0 is default if blank 
	string stripeID = "";		// optional; not used to differentiate stripes.  Could be included in output as segment ID
	string segmentStyleID = "";	// selects an existing SegmentStyle for this stripe
	int segmentStyleIntID = 0;  // integerized version of segmentStyleID, to be populated after reading in all styles
	double startX, startY, endX, endY;     // coordinates of start, end of stripe
	int stripeLayerNum = 1;		// layer on which this stripe will appear, computed as floor(input stripe height / global layerThickness_mm)
	bool marked = false;		// if true, stripe has already been marked and can be ignored in future layers
};

// *********************************************
// Overall structure to store configuration data read from Excel (.xls) file.
// The fields shown here are a subset of the entire file contents; just enough to create directories
struct AMconfig
{
	string executableFolder = "";	// folder where generateScanpaths was executed.  We assume genLayer & genScan are in same folder, and slic3r is in a slic3r subfolder
	int fileVersion = 0;	 		// code version of this AmericaMakes configuration file.  constants.h contains current value, which is assessed by this code
	bool validConfigFile = false;	// true if config file version# matches code version and error checks indicate validity
	string configFilename = "";		// name of input config file, including full path
	string configPath = "";			// directory where the configuration file is located
	//
	string projectFolder = "";		// folder name read from configuration file.  Will be created if it does not exist
	string layerOutputFolder = "";	// target for layer files = configPath + projectFolder + layer subfolder
	string scanOutputFolder = "";	// target for scan files  = configPath + projectFolder + scan subfolder
	//
	double pMag = 1, vMag = 1;		// SVG layer-view parameters
	double vOffx = 0, vOffy = 0;
	int dim = 0;
	//
	double layerThickness_mm = 0.0;	// desired slice thickness
	double dosingFactor = 1.5;		// multiplier on layer thickness to indicate amount of powder applied to each layer
	bool outputIntegerIDs = true;	// if true, the string ID's for Velocity Profiles and SegStyles will be replaced by auto-generated integer ID's for simplicity/consistency
	bool createScanZIPfile = false; // if true, a zip file containing the scan XML files will be created in the SCAN folder.  NOT YET IMPLEMENTED
	//
	bool createLayerSVG = false;	// if true, SVG files for layers will be created
	int layerSVGinterval = -1;		// if createLayerSVG is true, this indicates the frequency of layer files.  -1=all
	bool createScanSVG = false;		// see above for createLayerSVG
	int scanSVGinterval = -1;		// see above for layerSVGinterval
	int startingScanLayer = 0;		// layer to start creating scan files, ordered from zero
	int endingScanLayer = -1;		// layer to end scan file creation (inclusive).  -1=topmost layer
	//
	vector<ipFile>          vF;				   // vector of input STL files and associated data
	vector<trajectoryProc>  trajProcList;	   // vector of trajector processing instructions
	vector<regionProfile>   regionProfileList; // vector of region profiles
	vector<segmentStyle>    segmentStyleList;  // vector of segment styles
	vector<velocityProfile> VPlist;			   // vector of velocity profiles
	//
	// Single-stripe section
	bool allStripesMarked = true;	// indicates whether genScan has completed all elements of stripeList; avoids re-scanning the stripe list in all layers.
									// If no stripes are included in the build, allStripesMarked==true.  Otherwise, it will be set false until all stripes have been completed
	int stripeTraj = 0;				// trajectory number for single-stripes, if included in the build.  Cannot be revised by the user
	string stripeRegionTag = "single_stripes";  // region tag placeholder for stripes ... not utilized by LabVIEW to affect the build
	string stripeJumpVPID = "";		// ID of velocity profile for jumps between stripes
	string stripeJumpSegStyleID = "";//ID of the segment style for jumps, which will be automatically created based on stripeJumpVID
	int stripeJumpSegStyleIntID = 0;// Integer ID corresponding to stripeJumpSegStyleID
	int stripeSkywrtgMode = 0;		// Skywriting mode for stripes.  0 = off
	vector<singleStripe> stripeList;// vector of individual line segments to be marked in addition to part files
};

// converts a string to lower case.  Used by some other routines, so called out in this header
string as_lower(string input);

// Helper function to read general parameters tab of an Excel configuration file
void readGeneralParameters(BasicExcelWorksheet* sheet2, AMconfig* configData, string tabName);

// Helper function to read the velocity profile tab of an Excel configuration file
void readVelocityProfiles(BasicExcelWorksheet* sheet3, AMconfig* configData, string tabName);

// Helper function to read the segment-style profile tab of an Excel configuration file
void readSegmentStyles(BasicExcelWorksheet* sheet4, AMconfig* configData, string tabName);

// Helper function to read the region profile tab of an Excel configuration file
void readRegionProfiles(BasicExcelWorksheet* sheet5, AMconfig* configData, string tabName);

// Helper function to read part file tab of an Excel configuration file
void readPartFiles(BasicExcelWorksheet* sheet6, AMconfig* configData, string tabName);

// Helper function to read trajectory processing tab of an Excel configuration file
void readTrajProcessing(BasicExcelWorksheet* sheet7, AMconfig* configData, string tabName);

// Helper function to read optional single-stripe tab of an Excel configuration file.
// If the tab is not included in the Excel file, stripeList will be empty and allStripesMarked set to true.
// Otherwise, stripeList will contain one or more line-segment stripes and allStripesMarked will be false
void readStripes(BasicExcelWorksheet* sheet8, AMconfig* configData, string tabName);

// Read an Excel configuration file and create an AMconfig structure
AMconfig AMconfigRead(const std::string& configFilename);
