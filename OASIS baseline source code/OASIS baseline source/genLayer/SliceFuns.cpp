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
SliceFuns.cpp contains functions to slice an STL file into layers
(via a user-downloaded slic3r.exe file), and then traverse the
layers of the resulting SVG file to identify vertices and edges
//============================================================*/

#include "SliceFuns.h"

int runSlic3r(string fn, double layerThickness_mm, string executableFolder)
{   
	// executableFolder is the directory in which generateScanpaths.exe appears.  Slic3r should be in a slic3r_130 folder below that
	// fn is the stl filename including full path and .stl extension
	//
	// Updated to force GUI off and include a first layer height
	// Notes on slic3r:  
	//	1. adding the --repair option precludes SVG output (you get .obj only)
	//	2. need to specify the first layer height separately from general layer height else you get a slic3r default (perhaps 0.3mm)
	//	3. specifying a non-zero resolution value helps to avoid slight offsets in contours which are magnified in scan generation.
	//		however we needed to choose the resolution to balance accuracy and error.  5um (0.005mm) seems to work well
	
	// create a Slic3r command that includes filename, SVG slice & export, layer height and other options
	string command = "slic3r_130\\slic3r.exe \"" + fn + "\" --export-svg --no-gui --layer-height ";		// slic3r call plus filename
	command += to_string(layerThickness_mm) + " --first-layer-height " + to_string(layerThickness_mm);	// options
	command += " --resolution 0.005";	// additional options (added 2020-07-21)
	cout << "Slicing " << fn << "... ";
	// system call.  slic3r always returns 0 even when there's an error, so we don't evaluate the return value
	system(command.c_str());
	cout << "Done\n";

	// check whether the SVG file was actually created
	int outputFlag = -1;  // return value where 0 = valid svg file
	// Generate the expected svg filename by replacing .stl with .svg
	int len = strlen(fn.c_str());
	string svg_fn = fn.substr(0, len - 4) + ".svg";
	// evaluate attributes of this file
	DWORD ftyp = GetFileAttributesA(svg_fn.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES) {
		outputFlag = -1;      // SVG file was not created
	} else {
		if (ftyp) {
			outputFlag = 0;   // SVG file exists!
		}
	}

	return outputFlag;
}


int readFile(string fn, long numLayer, layer* L, string rTag, string cSys, int cTraj, int hTraj)
{
	size_t			pos = 0;
	string			line, sub;
	long ctLayer = 0;
	int bRead = 0;
	int				currentLayer = 0, kk = 0;
	int				Labelval[100] = { -1 };
	double			Units = 0.0;
	long			Layer = 0, Power = 0, Speed = 0, Focus = 0;//for iterating data per layer
	ifstream		file(fn);
	
	vector<loop> lp;
	slice sl;
	int eof = 1;
	int clayer = 0;

	while (getline(file, line)) 
	{
		
		if (string::npos != line.find("<g"))
		{		
			pos = line.find('id=');  // does not work if line.find("id=")
			line = line.substr(pos + 1);
			istringstream	ss;
			ss.str(line);
			getline(ss, sub, ' ');			
			int len = strlen(sub.c_str());			
			int nlayer = atoi((sub.substr(6, len - 7)).c_str());
			if (nlayer == numLayer)
			{
				eof = 0;
				clayer = 1;
			}
				
			if (clayer == 1)
			{
				
				getline(ss, sub, ' ');
				len = strlen(sub.c_str());
				L->zHeight = atof((sub.substr(10, len - 12)).c_str());
				lp.clear();				
			}					
		}

		if (string::npos != line.find("<polygon"))
		{
			if (clayer == 1)
			{
				pos = line.find('slic');
				line = line.substr(pos + 1);
				istringstream ss;
				ss.str(line);
				getline(ss, sub, ' ');
				int len = strlen(sub.c_str());					
				string looptype = (sub.substr(9, len - 10));
				if (!looptype.compare("contour"))
				{					
					string vlist = getVlist(ss.str(),1);
					vector<vertex> vl;
					getVertices(vlist, vl, cSys);
					loop r;
					r.type = "Outer";
					r.tag = rTag;
					r.contourTraj = cTraj;
					r.hatchTraj = hTraj;
					r.vList = vl;
					lp.push_back(r);
				}
				else if (!looptype.compare("hole"))
				{
					string vlist = getVlist(ss.str(),2);
					vector<vertex> vl;
					getVertices(vlist, vl, cSys);
					loop r;
					r.type = "Inner";
					r.tag = rTag;
					r.contourTraj = cTraj;
					r.hatchTraj = hTraj;
					r.vList = vl;
					lp.push_back(r);
				}
			}
		}
		if (string::npos != line.find("</g>"))
		{
			
			if (clayer == 1)
			{			
				sl.lpList = lp;
				L->us = sl;
				return eof;
			}			
		}
	}
	return eof;
}


string getVlist(string s, int type)
{
	string vlist = s;
	int diff = 49;  // set a default value
	size_t vpos = 0;
	vpos = vlist.find("points=");
	int vlen = strlen(vlist.c_str());
	if (type == 1)
		diff = 49;
	else if (type == 2)
		diff = 46;
	vlist = vlist.substr(vpos + 8, vlen - diff);
	return vlist;
}

void getVertices(string vs, vector<vertex>& vl, string cSys)
{
	vertex v;
	istringstream vss;
	size_t vpos = 0;
	vss.str(vs);
	string sub;
	int eol = 0;
	while (!eol)
	{
		getline(vss, sub, ' ');
		// detect end of line
		string ec = sub.substr(strlen(sub.c_str()) - 1, 1);
		if (!ec.compare("\""))
		{
			eol = 1;
		}
		//separate coordinates
		stringstream vsp;
		vsp.str(sub);
		string sx;
		getline(vsp, sx, ',');
		string sy;
		getline(vsp, sy, ' ');
		if (eol)
			sy = sy.substr(0, strlen(sy.c_str()) - 1);
		v.x = atof(sx.c_str());
		v.y = atof(sy.c_str());
		v.cord_sys = cSys;
		vl.push_back(v);
	}
	
}

void displayLayer(layer L)
{
	cout << "Z Height: " << L.zHeight << endl;
	slice s = L.us;
	cout << "Number of Loops : " << (s.lpList).size() << endl;
	for (vector<loop>::iterator lt = (s.lpList).begin(); lt != (s.lpList).end(); ++lt)
	{
		cout << "Loop Type:  " << (*lt).type << endl;
		cout << "No. of vertices: " << ((*lt).vList).size() << endl;
	}		
}

void refineLayer(layer *L)
{
	slice s = L->us;	// extract the upper slice bounding layer L
	vector<vertex> vList;	// vList initially has no contents
	vector<edge> eList;
	edge e;
	int idx;
	vertex sv, fv;
	int cnt = 0;
	for (vector<loop>::iterator lt = (s.lpList).begin(); lt != (s.lpList).end(); ++lt)	// iterate across loops in the upper slice s
	{
		eList.clear();	
		region r;
		sv = (*lt).vList[0];	// sv is the first vertex of the current loop lt
		int idx_s = findVertex(vList, sv);
		if (idx_s >= 0)
			e.start_idx = idx_s;
		else
		{
			vList.push_back(sv);
			e.start_idx = vList.size();
			idx_s = e.start_idx;
		}	
		
		for (vector<vertex>::iterator vt = ((*lt).vList).begin()+1; vt != ((*lt).vList).end(); ++vt)
		{			
			fv = *vt;
			idx = findVertex(vList, fv);
			if (idx >= 0)
				e.end_idx = idx;
			else
			{
				vList.push_back(fv);
				e.end_idx = vList.size();
			}
			e.curvetype = "Linear";
			eList.push_back(e);
			e.start_idx = e.end_idx;
		}
		e.end_idx = idx_s;
		eList.push_back(e);
		r.eList = eList;
		r.type = (*lt).type;
		r.tag = (*lt).tag;
		r.contourTraj = (*lt).contourTraj;
		r.hatchTraj = (*lt).hatchTraj;
		(s.rList).push_back(r);
	}
	L->vList = vList;
	L->us = s;
}

int cmpVertex(vertex v1, vertex v2)
{
	int match = 0;
	if (v1.x == v2.x)
	{
		if (v1.y == v2.y)
			match = 1; 
	}
	return match;
}

int findVertex(vector<vertex> vList, vertex v)
{
	for (vector<vertex>::iterator vt = vList.begin(); vt != vList.end(); ++vt)
	{
		if (vt->x == v.x)
		{
			if (vt->y == v.y)
				return std::distance(vList.begin(), vt);
		}
	}
	return -1;
}

/* // original version of findVertex
int findVertex(vector<vertex> vList, vertex v)
{
	int pos = 0;
	//int match = 0;	// not used in this function
	for (vector<vertex>::iterator vt = vList.begin(); vt != vList.end(); ++vt)
	{
		if (cmpVertex(*vt, v) == 1)	// would it be faster to merge cmpVertex code here, rather than using function call?
			return pos;
		pos++;		
	}
	return -1;
}
*/

void displayFLayer(layer L)
{

	cout << "Z Height: " << L.zHeight << endl;
	slice s = L.us;
	cout << "===================VertexList=====================" << endl;
	for (vector<vertex>::iterator vt = (L.vList).begin(); vt != (L.vList).end(); ++vt)
	{
		cout << (*vt).x << "," << (*vt).y << endl;
	}
	cout << "===================Slice=====================" << endl;
	cout << "Number of Regions: " << (s.rList).size() << endl;
	for (vector<region>::iterator rt = (s.rList).begin(); rt != (s.rList).end(); ++rt)
	{
		cout << "Region Type:  " << (*rt).type << endl;
		for (vector<edge>::iterator et = ((*rt).eList).begin(); et != ((*rt).eList).end(); ++et)
		{
			cout << (*et).start_idx << "," << (*et).end_idx << endl;
		}
		cout << "=============================================" << endl;
	}
}

int getNumLayer(string fn)
{
	size_t			pos = 0;
	string			line, sub;	
	ifstream		file(fn);
	int nlayer = 0;
	while (getline(file, line))
	{

		if (string::npos != line.find("<g"))
		{
			pos = line.find('id=');  // does not work if line.find("id=")
			line = line.substr(pos + 1);
			istringstream	ss;
			ss.str(line);
			getline(ss, sub, ' ');
			int len = strlen(sub.c_str());
			nlayer = atoi((sub.substr(6, len - 7)).c_str());	
			}			
	}
	return nlayer;
}

layer combLayer(vector<layer> vL)
{
	layer L;
	int firstLayer = 1;
	for (vector<layer>::iterator lt = vL.begin(); lt != vL.end(); ++lt)
	{
		if (!(*lt).isEmpty)
		{
			if (firstLayer)
			{
				L = *lt;
				firstLayer = 0;
			}
			else
			{
				slice s, st;
				s = L.us;
				st = (*lt).us;
				(s.lpList).insert((s.lpList).end(), (st.lpList).begin(), (st.lpList).end());
				L.us = s;
				firstLayer = 0;
			}
		}
	}
	return L;
}

void scaleLayer(layer *L, double mag, double xo, double yo)
{
	slice s = L->us;	
	vertex v;	
	for (vector<loop>::iterator lt = (s.lpList).begin(); lt != (s.lpList).end(); ++lt)
	{

		for (vector<vertex>::iterator vt = ((*lt).vList).begin(); vt != ((*lt).vList).end(); ++vt)
		{
			v.x = (*vt).x*mag + xo*mag;
			v.y = (*vt).y*mag + yo*mag;
			(*vt) = v;
		}		
	}
	L->us = s;
}

vertex findOffset(string fn)
{
	vertex vMin, vTmp;

	//get first vertex and assign it to vMax and vMin;
	size_t			pos = 0;
	string			line, sub;
	ifstream		file(fn);
	int isFirst = 1;
	while (getline(file, line))
	{
		if (string::npos != line.find("vertex"))
		{
			pos = line.find("vertex");
			line = line.substr(pos + 7);
			istringstream	ss;
			ss.str(line);
			getline(ss, sub, ' ');
			getline(ss, sub, ' ');
			vTmp.x = atof(sub.c_str());
			getline(ss, sub, ' ');
			vTmp.y = atof(sub.c_str());
			getline(ss, sub, ' ');
			vTmp.z = atof(sub.c_str());
			if (isFirst)
			{
				isFirst = 0;
				vMin = vTmp;
			}
			else
			{
				if (vTmp.x < vMin.x)
					vMin.x = vTmp.x;
				if (vTmp.y < vMin.y)
					vMin.y = vTmp.y;
				if (vTmp.z < vMin.z)
					vMin.z = vTmp.z;
			}
		}

	}
	return vMin;
}

int ckFile(string fn)
{
	int length;
	ifstream is(fn.c_str(), ios::in | ios::binary);
	is.seekg(0, ios::end);
	length = is.tellg();	
	is.seekg(0, ios::beg);
	char header[80];
	char nTrig[4];
	is.read(header, 80);
	is.read(nTrig, 4);
	if (length == (*((int *)nTrig)) * 50 + 84)
	{
		is.close();
		return 1;
	}
	else if (length>15)
	{
		return 0;
	}
	else
	{
		is.close();
		return 2;
	}

}

vector<vertex> findBoundary(string fn)
{
	vector<vertex> vv;
	int isBinary = ckFile(fn);
	if (isBinary)
	{
		// Binary STL format
		ifstream is(fn.c_str(), ios::in | ios::binary);
		char header[80];
		char nTrig[4];
		char facet[50];
		is.read(header, 80);
		is.read(nTrig, 4);
		int iFaceCount = *((int *)nTrig);
		
		float v[12]; // normal=3, vertices=3*3 = 12
		int isFirst = 1;
		vertex vMin, vTmp,vL,vR,vT,vB;

		// Every Face is 50 Bytes: Normal(3*float), Vertices(9*float), 2 Bytes Spacer
		for (size_t i = 0; i<iFaceCount; i++)
		{
			is.read(facet, 50);
			for (size_t j = 0; j<12; ++j) 
			{
				v[j] = *((float*)facet + j);	
			}
			for (int k = 1; k < 4; k++)
			{
				vTmp.x = v[k*3];
				vTmp.y = v[k*3+1];
				vTmp.z = v[k*3+2];
				if (isFirst)
				{
					isFirst = 0;
					vMin = vTmp;
					vL = vTmp;	// vertex to Left
					vR = vTmp;	// vertex to Right
					vT = vTmp;	// vertex to Top
					vB = vTmp;	// vertex to Bottom
				}
				else
				{
					if (vTmp.x < vMin.x)
						vMin.x = vTmp.x;
					if (vTmp.y < vMin.y)
						vMin.y = vTmp.y;
					if (vTmp.z < vMin.z)
						vMin.z = vTmp.z;
					if (vTmp.x < vL.x)
						vL = vTmp;
					if (vTmp.x > vR.x)
						vR = vTmp;
					if (vTmp.y < vB.y)
						vB = vTmp;
					if (vTmp.y > vT.y)
						vT = vTmp;					
				}
			}			
		}		
		vv.push_back(vMin);	// vv[0] = vMin (left-most X value, bottom-most Y value)
		vv.push_back(vL);	// vv[1].x = vL (left-most X value, same as vv[0].x
		vv.push_back(vR);	// vv[2].x = vR (right-most X value)
		vv.push_back(vB);	// vv[3].y = vB (bottom-most Y value, same as vv[0].y
		vv.push_back(vT);	// vv[4].y = vT (top-most Y value)
		// vv[1].y, vv[2].y, vv[3].x and vv[4].x are irrelevant values
		is.close();
	}
	else
	{
		// Text-based STL format.  We permit variable number of delimiters (spaces) between values
		vertex vMin, vTmp, vL, vR, vT, vB;
		//get first vertex and assign it to vMax and vMin;
		size_t			pos = 0;
		string			line, sub;
		ifstream		file(fn);
		int isFirst = 1;
		while (getline(file, line))
		{
			//cout << "line= " << (string)line << endl;
			if (string::npos != line.find("vertex"))
			{
				pos = line.find("vertex");
				line = line.substr(pos + 7);
				//cout << "line after vertex= " << (string)line << endl;
				istringstream ss;
				ss.str(line);
				vector<string> tokens;
				copy(istream_iterator<string>(ss),
					istream_iterator<string>(),
					back_inserter(tokens));
				vTmp.x = atof(tokens[0].c_str());
				vTmp.y = atof(tokens[1].c_str());
				vTmp.z = atof(tokens[2].c_str());
				//cout << "X Y Z = " << vTmp.x << " " << vTmp.y << " " << vTmp.z << endl;
				if (isFirst)
				{
					isFirst = 0;
					vMin = vTmp;
					vL = vTmp;
					vR = vTmp;
					vT = vTmp;
					vB = vTmp;
					//cout << "First vertex x y z = " << vTmp.x << " " << vTmp.y << " " << vTmp.z << endl;
				}
				else
				{
					if (vTmp.x < vMin.x)
						vMin.x = vTmp.x;
					if (vTmp.y < vMin.y)
						vMin.y = vTmp.y;
					if (vTmp.z < vMin.z)
						vMin.z = vTmp.z;
					if (vTmp.x < vL.x)
						vL = vTmp;
					if (vTmp.x > vR.x)
						vR = vTmp;
					if (vTmp.y < vB.y)
						vB = vTmp;
					if (vTmp.y > vT.y)
						vT = vTmp;
				}
			}
		}
		vv.push_back(vMin);
		vv.push_back(vL);
		vv.push_back(vR);
		vv.push_back(vB);
		vv.push_back(vT);
	}
	return vv;
}

void includeStripesInBBox(AMconfig &configData, vector<double> &vL, vector<double> &vR, vector<double> &vB, vector<double> &vT)
{
	// Iterate over configData.stripeList to find min/max x and y coordinates
	if (configData.stripeList.size() > 0)
	{
		// single stripes are included in this build.  Set up temp vars based on first stripe
		double xMin = configData.stripeList[0].startX;
		double xMax = configData.stripeList[0].startX;
		double yMin = configData.stripeList[0].startY;
		double yMax = configData.stripeList[0].startY;
		
		// iterate over stripeList to find actual min/max values
		for (int stp = 0; stp < configData.stripeList.size(); stp++)
		{
			xMin = min( min( xMin, configData.stripeList[stp].startX), configData.stripeList[stp].endX );
			xMax = max( max( xMax, configData.stripeList[stp].startX), configData.stripeList[stp].endX );
			yMin = min( min( yMin, configData.stripeList[stp].startY), configData.stripeList[stp].endY );
			yMax = max( max( yMax, configData.stripeList[stp].startY), configData.stripeList[stp].endY );
		}

		// append the stripe min/max values to vL ... vT
		vL.push_back(xMin);
		vR.push_back(xMax);
		vB.push_back(yMin);
		vT.push_back(yMax);		
	}

	return;
}
