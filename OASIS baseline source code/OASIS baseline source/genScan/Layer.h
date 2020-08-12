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
Layer.h contains data structures to represent the ALSAM 3024
layer XML format
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

#define SHOWVERTEX 1

using namespace std;

// vertex is a single point
struct vertex
{	
	double x;	// coordinates in mm
	double y;	
};

// edge is two points defining a line
struct edge
{	
	vertex s;	//start vertex
	vertex f;	//end vertex
};

// region is an enclosed area, which may be an outer contour or an inner "hole"
struct region
{	
	string type;		// type is either outer (enclosed area is to be hatched) or inner (enclosed area is to be empty)
	string tag;			// user-defined tag which indicates a regionProfile for scan parameters
	vector<edge> eList;	// edge list
	int contourTraj;	// Trajectory# for contours, originally based on the part this region came from
	int hatchTraj;		// Trajectory# for hatches, originally based on the part this region came from
};

// slice is a collection of regions on the same plane
struct slice
{	
	vector<region> rList;
};

// layer is a collection of vertices
struct layer
{
	double thickness;
	slice s;				// collection of regions on the layer.  Note that there couuld be two slices, upper and lower
	vector<vertex> vList;	// list of vertices (points) on the layer
};

struct Linfo
{
	double z_height;
	string fn;
};

struct Hdr
{
	int numLayer;
	vector<Linfo> vLi;
};

void displayLayer(layer L);	//display the contents of the layer L
