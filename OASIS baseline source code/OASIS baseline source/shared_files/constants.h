/*============================================================//
Copyright (c) 2020 America Makes
All rights reserved
Created under ALSAM project 3024

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================*/

// constants.h contains various constants and settings used by the entire ALSAM 3024 solution

#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
#include <SDKDDKVer.h>

#include <string>

using namespace std;

constexpr auto AMconfigFileVersion = 3;	// Updated 2020-07-20.  Compatible America Makes configuration file version for this code
// Includes numerous updates versus original config file across most of the tabs

static const string schemaVersion = "2020-03-23";
// This is the date of the XML schema used by this scanpath generation code.
// schemaVersion will be output to each XML scan file

static const int layerCoordPrecision = 6;
static const int scanCoordPrecision = 3;
// Indicates the floating point precision for x/y coordinates in XML output.  Units are mm, so 3 digits is 1um resolution
// Intricate STL files require high LAYER-file resolution to avoid artifacts which will crop up later in SCAN generation.
// Six digits appears to be the minimum which works for any layer file, whereas just three digits is fine for scan files

static const int numLayersPerCall = 25;
// number of layers to be processed per call to genLayer.exe or genScan.exe
// Increasing this number will increase memory requirements between calls, 
// but fewer calls will make it run faster.  For large files, memory usage increases linearly with layer count

static const bool outputCoordSystem = false;
// if true, genLayer will indicate the coordinate system ("Cartesian") for every single vertex in layer XML files
// if false, the coordinate system will only be included for the first vertex in each file

// name of the text file which will be created in the config-file directory if errors occur.
// the file will be created in the same folder as the configuration file, unless that folder is somehow inaccessible,
// in which case the file will be created in the same folder as generateScanpaths.exe
static const string errorReportFilename = "ALSAM_Scanpath_errors.txt";

// intersectRange defines how "close enough" an edge must be to a hatch line be to be considered intersecting, in mm.
// This allows a small amount of leeway in findIntersection (between hatches & edges) due to floating point precision
static const double intersectRange = 0.00002;

// overlapRange defines how close two segments must be to be considered overlapping, in mm.
// This value is used in eliminatedDuplicates to exclude duplicate hatch/edge intersections, as can happen when a
// hatch grid line runs through a vertex in the part -- resulting in intersections with all the edges that meet at that vertex.
// overlapRange is larger than intersectRange to catch overlaps in cases where the hatch grid line is nearly parallel to an edge.
// In case of duplicates, only one of the duplicates is retained
static const double overlapRange = 0.0002;

// minDeterminant is the absolute value of the determinant between edge and hatch which indicates they are not parallel.
// if the determinant is less than this, we declare the two segments to be parallel and non-intersecting
// note that we normalize the determinant to the length of the edge in question; minDeterminant is then compared to the normalized determinant
static const double minDeterminant = 0.001;  // a determinant value of 0.001 corresponds to <1 micron difference between two lines across a 400mm plate
