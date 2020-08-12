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
ScanPath.h defines the data structure and the functions needed
to generate scan paths from the layer files and user input.

See also the structures and functions in Layer.h (based on info the layer XML file) and
readExcelConfig.h (containing important inputs to be applied to the scan paths).

Most of the important functions are contained in this file. 
New hatching/contouring functions should also be defined here. 
//============================================================*/


#pragma once
#include "Layer.h"	// structures corresponding to info from an XML layer file
#include <algorithm>
#include <math.h>
#include "clipper.hpp"

#include "readExcelConfig.h"

#define OPTHATCH 1
#define MAXCOST 99999


//data structure representing a straight-line segment at constant power (mark or jump), the smallest unit of the laser path
struct segment
{
	int id;				// unique segment identifier (optional)
	vertex start, end;	// start, end coordinates.  This code only uses the start coordinate for most segments
								// and omits the end coordinate - which is taken from the next segment in a series.
	string idSegStyl;	// SegmentStyleID associated with the segment, which contains power/speed/focus/velocity/wobble
	int isMark = 0;		// Indicates whether the segment is a mark or jump.  Simplifies the creation of SVG output
};

//data structure to define path, a collection of similar feautred segment
struct path
{
	vector<segment> vecSg;	// collection of segments that define a path
	string type;			// keyword identifying generic ideas to group similar paths, e.g., hatch, contour, etc. 
	string tag;				// user defined tag for the region in which the part resides.  Corresponds to a region profile
	int SkyWritingMode = 0;	// Defines how and whether skywriting should be enabled for this path
};

//data structure to define a single trajectory, a collection of paths.
//In the multi-laser schema, trajectories are not dedicated to a single laser and may contain paths that utilize different lasers.
struct trajectory
{
	int trajectoryNum = 1;			// Identifies this trajectory in the build order (starting from lowest number)
	string pathProcessingMode = "sequential"; // Indicates how multiple paths within this trajectory will be built.  sequential or concurrent
	vector<path> vecPath;			// collection of laser paths, created after hatching and/or contouring the associated regions
	//
	// The following trajRegion... vectors should all be the same length, and each element refer to the region linked in trajRegionLinks
	vector<int> trajRegions;		// list of regions under this trajectory number as identified by their order in the region list from XML.
		// NOTE - if contour and hatch for a region use the same trajectory#, they will appear twice
		// in trajRegions and trajRegionTypes because the two components are different paths and have different trajRegionTypes values
	vector<string> trajRegionTypes;	// identifies which of the two types each element of trajRegions refers to.  contour or hatch
	vector<string> trajRegionTags;	// indicates the tag assigned to the corresponding region in trajRegions
	vector<bool> trajRegionIsHatched; // indicates whether each region has already been hatched, to enable multiple regions to be done concurrently
	vector<region*> trajRegionLinks;// link to the actual regions listed in trajRegions, to simplify referencing
};

//data structure to represent a subset of path
struct hRegion
{
	vertex start;
	vertex end;
	vector<segment> vecSg;
};

//data structure to represent a vector
struct ray 
{
	double x;// component along x axis
	double y;//component along y axis
};

//function to add two vectors
ray rAdd(ray r1, ray r2);

//function to calculate the absolute value of a vector
double rMod(ray r);

//function to find the angle of a vector
double rAngle(ray r1, ray r2);

//function to convert an edge to a vector
ray e2r(edge e);

//given an vector of edge lists (assumed to be closed contours), their types (inner/outer) and offset, this function calculates a new set of offseted edges.
// only one of edgeListOut or polyVectorsOut will be populated, based on returnPolyVectors.  true --> polyVectorsOut is used, false --> edgeListOut instead
void edgeOffset(layer &L, vector<int> regionIndex, vector<edge> &edgeListOut, vector<vector<edge>> &polyVectorsOut, double offset, bool returnPolyVectors);

// helper function to determine endpoints of hatching (min/max x or y coordinates)
void findHatchBoundary(vector<vertex> &in, double hatchAngle, double *a_min, double *a_max);

//helper function to find intersection between an edge and a hatch line using hatch angle and x or y intercept
int findIntersection(vertex *out, double hatchAngle, double hatchAngle_rads, vector<vertex> BB, double intercept, edge e, double hatchFunctionValue);

//helper function to sort vertices with ascending y coordinates
void yAsc(vector<vertex> &vertexList);

//helper function to sort vertices with descending y coordinates
void yDsc(vector<vertex> &vertexList);

//helper function to sort vertices with ascending x coordinates
void xAsc(vector<vertex> &vertexList);

//helper function to sort vertices with descending x coordinates
void xDsc(vector<vertex> &vertexList);

//helper function that removes duplicate entries, which may arise when a hatch line crosses precisely through a vertex of multiple edges
vector<vertex> eliminateDuplicateVertices(vector<vertex> &vertexList);

//function to calculate distance between two vertices
double dist(vertex &v1, vertex &v2);

// function to determine whether single-stripes remain to be marked on this layer or higher
vector<int> singleStripeCount(int layerNum, AMconfig &configData);  // return value is vector of stripe trajectories appearing on this layer

// function to create marks and jumps corresponding to the single-stripe inputs for a particular layer and stripe trajectory#
path singleStripes(int layerNum, int trajectoryNum, AMconfig &configData);

// function to determine hatching path for all regions with a particular tag, without any constraints on distance
path hatch(layer &L, vector<int> regionIndex, regionProfile &rProfile, double offset, double hatchAngle, double a_min, double a_max, bool outputIntegerIDs, vector<vertex> boundingBox);

// function to determine hatching path for all regions with a particular tag - while minimizing total travel distance
path hatchOPT(layer &L, vector<int> regionIndex, regionProfile &rProfile, double offset, double hatchAngle, double a_min, double a_max, bool outputIntegerIDs, vector<vertex> boundingBox);

// function to create a contouring path for the inner or outer boundary of a specific region
path contour(layer &L, vector<int> regionIndex, regionProfile &rProfile, double offset, vector<vertex> BB, bool outputIntegerIDs);

//decides whether edges are turing CW or CCW to determine whether that portion of the curve is convex or not
int getTurnDir(edge ev1, edge ev2);

//display laser path
void displayPath(path P);

//find the bounding box of the layer
vector<vertex> getBB(layer &L);

//failsafe to see if any hatch/contour is leaving the bounding box. The line in question is defined by the vertices
int findInt(vector<vertex> &BB, vertex v1, vertex v2);
