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
writeScanXML.h defines  the functions needed to write the scan trajectory
data structure to an xml file. 
//============================================================*/

#pragma once

#ifndef WRITEXML_H
#define WRITEXML_H
#include <vector>
#include <msxml6.h>
#include "ScanPath.h"
#include "readLayerXML.h"
#include <comdef.h>
#include <string>
#include <windows.h>

#include "readExcelConfig.h"


/*  The items below are aleady defined in readLayerXML.h and do not need to be re-defined
#define CHK_HR(stmt)        do { hr=(stmt); if (FAILED(hr)) goto CleanUp; } while(0)
#define CHK_ALLOC(p)        do { if (!(p)) { hr = E_OUTOFMEMORY; goto CleanUp; } } while(0)
#define SAFE_RELEASE(p)     do { if ((p)) { (p)->Release(); (p) = NULL; } } while(0)\
*/

extern IXMLDOMDocument *pXMLDomScan;

using namespace std;

// If printTrajectories set to 1, a list of trajectories and regions as they are sent to scan XML will be printed to stdout.
// This complements printTraj, found in main_genScan, which identfies the trajectories as they are encountered in layer files
// Normally set printTrajectories to 0
#define printTrajectories 0

// Helper that allocates the BSTR param for the caller.
HRESULT CreateElement(IXMLDOMDocument *pXMLDom, PCWSTR wszName, IXMLDOMElement **ppElement);

// Helper function to append a child to a parent node.
HRESULT AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent);

// Helper function to create and add a processing instruction to a document node.
HRESULT CreateAndAddPINode(IXMLDOMDocument *pDom, PCWSTR wszTarget, PCWSTR wszData);

// Helper function to create and add a comment to a document node.
HRESULT CreateAndAddCommentNode(IXMLDOMDocument *pDom, PCWSTR wszComment);

// Helper function to create and add an attribute to a parent node.
HRESULT CreateAndAddAttributeNode(IXMLDOMDocument *pDom, PCWSTR wszName, PCWSTR wszValue, IXMLDOMElement *pParent);

// Helper function to create and append a text node to a parent node.
HRESULT CreateAndAddTextNode(IXMLDOMDocument *pDom, PCWSTR wszText, IXMLDOMNode *pParent);

// Helper function to create and append a CDATA node to a parent node.
HRESULT CreateAndAddCDATANode(IXMLDOMDocument *pDom, PCWSTR wszCDATA, IXMLDOMNode *pParent);

// Helper function to create and append an element node to a parent node, and pass the newly created
// element node to caller if it wants.
HRESULT CreateAndAddElementNode(IXMLDOMDocument *pDom, PCWSTR wszName, PCWSTR wszNewline, IXMLDOMNode *pParent, IXMLDOMElement **ppElement = NULL);

// Helper function to create a VT_BSTR variant from a null terminated string. 
//HRESULT VariantFromString(PCWSTR wszValue, VARIANT &Variant);

//HRESULT CreateAndInitDOM(IXMLDOMDocument **ppDoc);

//**************************
// Identify the trajectory numbers in a particular layer and the regions within each trajectory
vector<trajectory> identifyTrajectories(AMconfig &configData, layer &L, int layerNum);

// Manage the creation and writing of an XML SCAN file
void createSCANxmlFile(string fullXMLpath, int layerNum, AMconfig &configData, vector<trajectory> &trajectoryList);

// Create file header for XML SCAN file.
// TO DO:  generate min/max values to be included in header comment or other field
void addXMLheader(IXMLDOMElement *pRoot, int layerNum, double thickness, double dosingfactor);

// Create the velocity profile list section for an XML SCAN file
void addXMLvelocityProfileList(IXMLDOMElement *pRoot, AMconfig &configData);

// Create the segment style list section for an XML SCAN file
void addXMLsegmentStyleList(IXMLDOMElement *pRoot, AMconfig &configData);

// Create a single trajectory section for an XML SCAN file. This function will be called for each trajectory
void addXMLtrajectory(IXMLDOMElement *trajList, trajectory &T);

// Write scan output in SVG format.
// Takes as input the output filename, a trajectory set and the dimensions of the SVG file
void scan2SVG(string fn, vector<trajectory> &tList, int dim, double mag, double xo, double yo);

LPCWSTR d2lp(double in);

LPCWSTR i2lp(int in);

//PCWSTR s2lp(string in);

string d2s(double d);

#endif
