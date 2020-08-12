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
main_genLayer.cpp contains the main() function of the Layer Generator
program. Different functions are called from inside of main()
to perform various tasks of reading the STL file, slicing it.
combining multiple files and writing the output in XML and
SVG formats. This routine requires a slic3r.exe file
which must be downloaded separately by the end user
//============================================================*/

#include "SliceFuns.h"
#include "writeLayerXML.h"
#include "simple_svg_1.0.0.hpp"
#include <cctype>
#include <algorithm>

#include "constants.h"
#include "readExcelConfig.h"
#include "BasicExcel.hpp"
#include "errorChecks.h"
#include "io_functions.h"

using namespace std;

//function to write output in svg format
//takes as input the output filename fn, layer L and the user defined configuration file
void rlayer2SVG(string fn, layer L, AMconfig configData)
{
	svg::Dimensions dimensions(configData.dim, configData.dim);
	svg::Document docH(fn, svg::Layout(dimensions, svg::Layout::TopLeft));
	double sx, sy, fx, fy;// co-ordinates of vertices of an edge
	slice s = L.us;
	vector<vertex> vList = L.vList;

	//scaling factors so that svg fills the screen
	double mag = configData.vMag;
	double offx = configData.vOffx;
	double offy = configData.vOffy;
	int numSec = (configData.vF).size();

	//go through the layer structure and generate individual vectors to write to the svg file
	for (vector<region>::iterator rt = (s.rList).begin(); rt != (s.rList).end(); ++rt)
	{
		for (vector<edge>::iterator et = ((*rt).eList).begin(); et != ((*rt).eList).end(); ++et)
		{
			sx = (vList[(*et).start_idx - 1]).x * mag + offx;
			sy = (vList[(*et).start_idx - 1]).y * mag + offy;
			fx = (vList[(*et).end_idx - 1]).x * mag + offx;
			fy = (vList[(*et).end_idx - 1]).y * mag + offy;
			int bCont = 1;
			int i = 0;
			while (bCont)
			{
				//use different color for different type of regions
				if (!((*rt).tag).compare((configData.vF[i]).Tag))
				{
					bCont = 0;
					if (!((*rt).type).compare("Inner"))
					{
						svg::Line line(svg::Point(sx, configData.dim -sy), svg::Point(fx, configData.dim -fy), svg::Stroke(1, svg::Color::Blue));
						docH << line;
					}
					else
					{
						svg::Line line(svg::Point(sx, configData.dim -sy), svg::Point(fx, configData.dim -fy), svg::Stroke(1, svg::Color::Black));
						docH << line;
					}
				}
				if (i == numSec)
				{
					bCont = 0;
					svg::Line line(svg::Point(sx, configData.dim -sy), svg::Point(fx, configData.dim -fy), svg::Stroke(1, svg::Color::Red));
					docH << line;
				}
				++i;
			}
		}
	}
	docH.save();
}


int	main(int argc, char **argv)
// Required argument for genLayer.exe:
// <filename>.xls = full path to configuration file (surrounded by "").  If not specified, error will be reported
{
	// To enhance memory management, this program is called from an external script. Every time this program is called, 
	// it reads the last layer number generated from a *cfg file, generates a specific number of layers, and write the 
	// final layer number in the *cfg file for next time (or write if it is finished).

	// Determine where we are (current path)
	TCHAR filePath[MAX_PATH + 1] = "";
	GetCurrentDirectory(MAX_PATH, filePath);
	string currentPath = &filePath[0];

	// 1. Read the configuration file indicated by command-line arguments
	AMconfig configData;
	errorCheckStructure errorData;
	// Parse the command-line arguments, if any.
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
	
	configData = AMconfigRead(configFilename);
	configData.executableFolder = currentPath;  // assume we begin in the executable folder, slic3r folder is located

	// 2. Read the status file to determine the last layer completed by prior run of genLayer.exe, if any
	sts cst = readStatus("gl_sts.cfg"); // read ending layer number of the last iteration (or check if it is the first layer)
	int started; // flag to check if slicing of the part has already started
	int finished; // flag to write if slicing of the part is complete
	int sLayer; // starting layer number for a particular function call
	int fLayer; // final layer number for a particular call
	finished = 0; // by default, finished flag is set to zero
	started = cst.started;
	sLayer = cst.lastLayer+1;
	fLayer = sLayer + numLayersPerCall; // we do multiple layers in each call of this function. Increasing this number will increase memory
	// requirements between calls, but fewer iterations will make it run faster. 
	int close = 0;

	vector<obj> vOBJ;	// vOBJ contains information about each stl file
	vector<int> nLayer;	// number of layers in stl files
	vector<int> soffset;// z offset for stl files
	vector<double> vs;	// span of the stl files, needed to scale the svg display

	//intermediate information to generate span
	vector<double> vL, vR, vB, vT;  // will hold min/max x and y values for each part.
									// vL=left (min x), vR=right (max x), vB=bottom (min y), vT=top (max y)

	// 3. Iterate through the list of STL part files and run Slic3r on any which haven't been sliced
	// This generates points of intersections of the triangulated surfaces with
	// desired planes and determine the span and total number of layers for each stl file
	// We save info from previously-sliced parts in vv_array and layersPerFile to avoid re-slicing identical parts
	//
	vector<vertex> vv;	// Bounding box data for a specific part
		// vv[0] is min x,y,z across entire part, vv[1].x = min x, vv[2].x = max x, vv[3].y = min y, vv[4].y = max y
	vector<vector<vertex>> vv_array;  // bounding box info for each part
	vector<int> layersPerFile;  // number of layers per STL file - not including z offset
	obj o;
	bool fileExists;	// whether this stl file is found in working directory
	//
	for (int i = 0; i < (int)(configData.vF).size(); i++)  // iterate across STL files, creating one SVG for each unique filename
	{
		// Determine whether we've already sliced this filename
		bool previouslySliced = false;
		int priorPartnum = 0;
		while ((!previouslySliced) & (priorPartnum<i)) {
			// compare current filename to previously sliced parts
			if ((configData.vF[priorPartnum]).fn == (configData.vF[i]).fn) { 
				previouslySliced = true; 
			}
			else {
				priorPartnum++;
			}
		}

		// 3a. If this part hasn't been sliced, verify that the STL file exists
		fileExists = false;  // assume there's no file until we find it
		if (previouslySliced) {
			fileExists = true;	// assume previously-sliced files have already been verified
		}
		else {
			// evaluate attributes of this file
			DWORD ftyp = GetFileAttributesA((configData.vF[i]).fn.c_str());
			if (ftyp == INVALID_FILE_ATTRIBUTES) {
				fileExists = false;  // STL file cannot be found
				string errMsg = "The STL file named " + configData.vF[i].fn + " cannot be found in the same folder as the configuration file\n";
				updateErrorResults(errorData, true, "genLayer main", errMsg, "", configData.configFilename, configData.configPath);
				return -1;
			}
			else {
				if (ftyp) {
					fileExists = true;   // STL file exists
				}
			}
		}
		
		// 3b. Determine bounding box of the part.  If it hasn't been sliced, compute from STL file, otherwise pull from prior data
		if (previouslySliced) { 
			vv = vv_array[priorPartnum];	// avoid recomputing boundary box; pull data from prior part
		} else {
			vv = findBoundary((configData.vF[i]).fn);  // recompute bounding box
		}
		vv_array.push_back(vv);	// save bounding box of this part into array of bounding boxes
		vL.push_back(vv[1].x + (configData.vF[i]).x_offset);	// vL = minimum x value for this part
		vR.push_back(vv[2].x + (configData.vF[i]).x_offset);	// vR = max x value for this part
		vB.push_back(vv[3].y + (configData.vF[i]).y_offset);	// vB = minimum y value for this part
		vT.push_back(vv[4].y + (configData.vF[i]).y_offset);	// vT = max y value for this part
		double xspan = vv[2].x - vv[1].x;
		double yspan = vv[4].y - vv[3].y;
		double span = max(xspan, yspan);
		vs.push_back(span);

		//find offsets
		(configData.vF[i]).x_offset = (configData.vF[i]).x_offset + vv[0].x;
		(configData.vF[i]).y_offset = (configData.vF[i]).y_offset + vv[0].y;
		(configData.vF[i]).z_offset = (configData.vF[i]).z_offset + vv[0].z;

		// 3c. If not previously sliced, run slic3r on the part
		if ((!started) & (!previouslySliced)) {
			int slicerReturnValue = runSlic3r((configData.vF[i]).fn, configData.layerThickness_mm, configData.executableFolder);
			if (slicerReturnValue != 0) {
				// slic3r encountered an issue with an STL file
				string errMsg = "Slic3r was not able to slice " + configData.vF[i].fn + "\n";
				updateErrorResults(errorData, true, "SliceFuns", errMsg, "", configData.configFilename, configData.configPath);
				return -1;
			}
		}
		// 3d. Compute number of layers in this part above z=0, incorporating z offset
		int len = strlen((configData.vF[i]).fn.c_str());
		o.fn = (configData.vF[i]).fn.substr(0, len - 4);
		// save information about this part:  filename without extension, z offset in layers, total layer count
		o.cntOffset = (int)(((configData.vF[i]).z_offset) / configData.layerThickness_mm);
		soffset.push_back(o.cntOffset);
		string sfn = o.fn + ".svg";
		o.totLayer = (getNumLayer(sfn) + o.cntOffset);
		vOBJ.push_back(o);
		nLayer.push_back(o.totLayer);
	}
	
	// 4. Determine maximum number of layers in the entire build
	// 		Total number of layers is the maximum layer number across all individual files
	//		as well as any single stripes defined in the configuration file (which are not part files)
	if (configData.stripeList.size() > 0)
	{
		int maxStripeLayer = 0;
		for (int i = 0; i < configData.stripeList.size(); i++) {
			if (configData.stripeList[i].stripeLayerNum > maxStripeLayer) maxStripeLayer = configData.stripeList[i].stripeLayerNum;
		}
		nLayer.push_back(maxStripeLayer -1);  // include factor of -1 because stripeLayerNum orders from 1 rather than from 0
	}
	int totLayer = *(max_element(nLayer.begin(), nLayer.end())) +1;
	// if the final layer number is less than the ending layer of the current call, set the finished flag
	if (totLayer <= fLayer)
	{
		fLayer = totLayer;
		finished = 1;
	}

	// 5. Generate scaling factors for svg visualization and save to a text file to be later used by scan generator.
	// We compute span as the max x or y range when all parts are in their assigned (offset) locations
	//
	// First assess x/y positions of any single-stripes included in the build
	includeStripesInBBox(configData, vL, vR, vB, vT);
	// Now compute min/max x and y across all parts and single-stripes
	double minX = *(min_element(vL.begin(), vL.end()));
	double maxX = *(max_element(vR.begin(), vR.end()));
	double minY = *(min_element(vB.begin(), vB.end()));
	double maxY = *(max_element(vT.begin(), vT.end()));
	double spanX = maxX - minX;
	double spanY = maxY - minY;
	double span = max(spanX, spanY);
	double mag = 1400 / span;
	double xo, yo;
	xo = 150 - minX*mag;
	yo = 25 - minY*mag;
	configData.dim = 2000;	// Sets size of view window
	configData.vMag = mag;
	configData.vOffx = xo;
	configData.vOffy = yo;
	ofstream fout;
	fout.open(configData.layerOutputFolder + "\\" + "vConfig.txt");
	fout << mag << "," << xo << "," << yo << endl;
	fout.close();

	if (started == 0) { cout << "Total number of layers: " << totLayer << "\n\n"; }

	vector<layer> vLayer;// layer from all stl files
	layer L;  // temporary layer var
	layer Lc; // combined layer with appropriate tags
	vector<Linfo> Lhdr; // information to be written in the header file

	// set up variables for cursor control
	COORD cursorPosition;
	HANDLE hStdout;
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	// Identify current cursor position
	cursorPosition = GetConsoleCursorPosition(hStdout);

	//**************************************
	// 6. PROCESS A NUMBER OF LAYERS from sLayer (starting layer for this genLayer instance) to fLayer (final layer for this instance)
	// By generating combined layer structure with appropriate tags and write them to XML and SVG formats
	for (int i = sLayer; i <= fLayer; i++)
	{
		cout << "Processing layer " << i << " of " << totLayer;
		// reset cursor position for next iteration
		SetConsoleCursorPosition(hStdout, cursorPosition);

		// clear the large re-used variables to help with memory leakage
		vLayer.clear();
		vLayer.shrink_to_fit();
		//
		Lc.ls.lpList.clear();  // clear the combined layer (this is done once per layer)
		Lc.ls.rList.clear();
		Lc.us.lpList.clear();
		Lc.us.rList.clear();
		Lc.vList.clear();
		//
		Lc.ls.lpList.shrink_to_fit();
		Lc.ls.rList.shrink_to_fit();
		Lc.us.lpList.shrink_to_fit();
		Lc.us.rList.shrink_to_fit();
		Lc.vList.shrink_to_fit();

		// 6a. Iterate across parts listed in vOBJ, generate a layer structure for each and append to vLayer
		//		This includes all regions for the part on just the current layer
		for (int j = 0; j < (int)vOBJ.size(); j++)
		{
			L.ls.lpList.clear();  // clear the temporary layer var (this is done in between reading parts for this layer)
			L.ls.rList.clear();
			L.us.lpList.clear();
			L.us.rList.clear();
			L.vList.clear();
			//
			L.ls.lpList.shrink_to_fit();
			L.ls.rList.shrink_to_fit();
			L.us.lpList.shrink_to_fit();
			L.us.rList.shrink_to_fit();
			L.vList.shrink_to_fit();

			L.isEmpty = 1;  // will be set to 0 (not empty) if anything is found for part j on layer i

			if (i > (vOBJ[j].cntOffset) && i <= (vOBJ[j].totLayer+1))
			{
				L.isEmpty = 0;
				// Read the SVG file corresponding to this point, extract info for one layer and apply tags and trajectory#'s
				readFile(vOBJ[j].fn + ".svg", (i-1) - vOBJ[j].cntOffset, &L, (configData.vF[j]).Tag, "R", (configData.vF[j]).contourTraj, (configData.vF[j]).hatchTraj);
				// "R" is the coordinate system, short for "Rectangular"
			}
			scaleLayer(&L, configData.pMag, (configData.vF[j]).x_offset, (configData.vF[j]).y_offset);
			vLayer.push_back(L);
		}

		// 6b. Combine all parts into one layer structure
		Lc = combLayer(vLayer);
		
		// 6c. Clean up layer structure and convert it to the desired form
		refineLayer(&Lc);
		Lc.thickness = configData.layerThickness_mm;

		// generate filenames by appending appropriate numbers
		string zs;
		for (int k = 0; k < (int)(to_string(totLayer)).size() - (int)(to_string(i)).size(); k++)
		{
			zs = zs + "0";
		};

		// 6d. Optionally, generate an SVG visualization file displaying just this layer
		if ( (configData.createLayerSVG == 1) && ((i % configData.layerSVGinterval == 0) | (i==1)) ) {
			//generate SVG file then move to SVG subfolder
			string nfn = "layer_" + zs + to_string(i) + ".svg";
			string fullSVGpath = configData.layerOutputFolder + "\\SVGdir\\" + nfn;
			rlayer2SVG(fullSVGpath, Lc, configData);
		}

		// 6e. Generate an XML layer file from the layer structure, using the Microsoft DOM
		string xfn = "layer_" + zs + to_string(i) + ".xml";
		string fullXMLpath = configData.layerOutputFolder + "\\XMLdir\\" + xfn;
		HRESULT hr = CoInitialize(NULL);
		if (SUCCEEDED(hr))
		{
			writeLayer(fullXMLpath, Lc);		
			Linfo li;
			li.fn = xfn;
			li.zHeight = Lc.zHeight;
			Lhdr.push_back(li);
			CoUninitialize();
		}
	} // end for (int i = sLayer; i <= fLayer; i++)

	// 7. Target number of layers are complete for this instance of genLayer.  Create a single XML file containing header information from the DOM
	string hfn = "\"" + configData.layerOutputFolder + "\\XMLdir\\layer_header.xml\"";
	HRESULT hr = CoInitialize(NULL);

	if (SUCCEEDED(hr))
	{
		writeHeader(hfn, Lhdr, totLayer);
		CoUninitialize();
	}

	// 8. Write ending layer number and whether all are completed to gl_sts.cfg file for communication with createScanpaths
	ofstream stfile;
	stfile.open("gl_sts.cfg");
	stfile << 1 << endl;	// set started to 1, which will avoid re-slicing parts
	stfile << fLayer << endl;
	stfile << finished << endl;
	stfile << configData.layerOutputFolder << endl;
	stfile.close();

	return 0;
}
