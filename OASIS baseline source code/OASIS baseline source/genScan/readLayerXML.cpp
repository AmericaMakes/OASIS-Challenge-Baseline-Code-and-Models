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
readLayerXML.cpp contains functions to read a layer file in ALSAM XML-layer
format and process it into data structures in preparation for 
scanpath generation
//============================================================*/

#include "readLayerXML.h"
#include "ScanPath.h"

HRESULT VariantFromString(PCWSTR wszValue, VARIANT &Variant)
{
	HRESULT hr = S_OK;
	BSTR bstr = SysAllocString(wszValue);
	CHK_ALLOC(bstr);

	V_VT(&Variant) = VT_BSTR;
	V_BSTR(&Variant) = bstr;

CleanUp:
	return hr;
}

HRESULT CreateAndInitDOM(IXMLDOMDocument **ppDoc)
{
	HRESULT hr = CoCreateInstance(__uuidof(DOMDocument60), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(ppDoc));
	if (SUCCEEDED(hr))
	{
		(*ppDoc)->put_async(VARIANT_FALSE);
		(*ppDoc)->put_validateOnParse(VARIANT_FALSE);
		(*ppDoc)->put_resolveExternals(VARIANT_FALSE);
		(*ppDoc)->put_preserveWhiteSpace(VARIANT_TRUE);
	}
	return hr;
}

double b2d(BSTR in)
{
	double out;
	_bstr_t b_in(in);
	out = atof((char*)b_in);
	return out;
}

int b2i(BSTR in)
{
	int out;
	_bstr_t b_in(in);
	out = atoi((char*)b_in);
	return out;
}

string b2s(BSTR in)
{
	_bstr_t b_in(in);
	string out = string((char*)b_in);
	return out;
}


int loadDOM(PCWSTR wszValue)
{	// reads an XML file into the Microsoft Domain Object Model format, but does not parse the values
	HRESULT hr = S_OK;
	IXMLDOMParseError *pXMLErr = NULL;
	BSTR bstrErr = NULL;
	BSTR bstrXML = NULL;
	VARIANT_BOOL varStatus;
	VARIANT varFileName;
	VariantInit(&varFileName);
	CHK_HR(CreateAndInitDOM(&pXMLDomLayer));

	// XML file name to load
	CHK_HR(VariantFromString(wszValue, varFileName));
	CHK_HR(pXMLDomLayer->load(varFileName, &varStatus));
	if (varStatus == VARIANT_FALSE)
	{
		// Failed to load xml, get last parsing error
		CHK_HR(pXMLDomLayer->get_parseError(&pXMLErr));
		CHK_HR(pXMLErr->get_reason(&bstrErr));
		printf("Failed to load layer details from %S. %S\n", wszValue, bstrErr);
		return 1;
	}
#if printDOM
	printf("========================DOM Structure================================\n");
#else
	//	printf("====================================================================\n");
#endif

CleanUp:
	SAFE_RELEASE(pXMLErr);
	SysFreeString(bstrXML);
	SysFreeString(bstrErr);
	VariantClear(&varFileName);
	return 0;
}


layer traverseDOM()
{	// Parses values in the Domain Object Model into a layer structure
	layer L;
	slice s;
	vector<region> rList;
	vector<vertex> vertexList;
	vector<edge> eList;
	L.thickness = 0.0;  // initialize a value

	HRESULT hr = S_OK;
	IXMLDOMElement *pRoot = NULL;
	IXMLDOMNodeList *rchildList = NULL;
	IXMLDOMNode *node = NULL;
	IXMLDOMNode *node2 = NULL;
	IXMLDOMNode *node3 = NULL;
	BSTR rootName = NULL;
	BSTR nodeName = NULL;
	BSTR nodeText;
	
	//get root node and its children
	CHK_HR(pXMLDomLayer->get_documentElement(&pRoot));
	CHK_HR((pRoot)->get_baseName(&rootName));
#if printDOM
	printf("%S\n", rootName);
#endif
	CHK_HR(pRoot->get_childNodes(&rchildList));

	//get thickness
	CHK_HR(rchildList->get_item(1, &node));
	CHK_HR((node)->get_baseName(&nodeName));
#if printDOM
	//printf("\t+%S\n", nodeName);
#endif
	CHK_HR((node)->get_text(&nodeText));
#if printDOM
	printf("\t+%S: %f\n", nodeName, b2d(nodeText));
#endif
	L.thickness = b2d(nodeText);

	//get vertex list
	CHK_HR(rchildList->get_item(3, &node));
	CHK_HR((node)->get_baseName(&nodeName));
#if printDOM
	printf("\t+%S\n", nodeName);
#endif
	IXMLDOMNodeList *ncList = NULL;
	CHK_HR(node->get_childNodes(&ncList));
	long ncLength;
	CHK_HR(ncList->get_length(&ncLength));
	// get vertex type
	int i;
	int j;
	IXMLDOMNode *vtx = NULL;
	vertexList.clear();
	for (i = 0; i < ncLength; i++)
	{
		CHK_HR(ncList->nextNode(&vtx));
		CHK_HR((vtx)->get_baseName(&nodeName));
		vertex v;
		if (i % 2 == 1)
		{
#if printDOM
			printf("\t\t+%S\n", nodeName);
#endif
			//get coordinates
			IXMLDOMNodeList *pList;
			CHK_HR(vtx->get_childNodes(&pList));
			IXMLDOMNode *xNode = NULL;
			CHK_HR(pList->get_item(1, &xNode));
			CHK_HR((xNode)->get_baseName(&nodeName));
			CHK_HR((xNode)->get_text(&nodeText));
#if printDOM
			printf("\t\t\t\t+%S: %f\n", nodeName, b2d(nodeText));
#endif
			v.x=b2d(nodeText);
			IXMLDOMNode *yNode = NULL;
			CHK_HR(pList->get_item(3, &yNode));
			CHK_HR((yNode)->get_baseName(&nodeName));
			CHK_HR((yNode)->get_text(&nodeText));
#if printDOM
			printf("\t\t\t\t+%S: %f\n", nodeName, b2d(nodeText));
#endif
			v.y=b2d(nodeText);
			vertexList.push_back(v);
		}
	}
	L.vList = vertexList;

	//get layer information
	CHK_HR(rchildList->get_item(5, &node));
	CHK_HR((node)->get_baseName(&nodeName));
#if printDOM
	printf("\t+%S\n", nodeName);
#endif
	CHK_HR(node->get_childNodes(&ncList));
	CHK_HR(ncList->get_length(&ncLength));
	rList.clear();
	IXMLDOMNode *regionNode = NULL;
	for (i = 0; i < ncLength; i++)
	{
		region r;
		CHK_HR(ncList->nextNode(&regionNode));
		CHK_HR((regionNode)->get_baseName(&nodeName));
		if (i % 2 == 1)
		{
			
#if printDOM
			printf("\t\t\t+%S\n", nodeName);
#endif
			IXMLDOMNodeList *rcList = NULL;
			CHK_HR(regionNode->get_childNodes(&rcList));
			long rcLength;
			CHK_HR(rcList->get_length(&rcLength));
			// Items within rcList are ordered from 0, but only the odd-numbered items are actual XML values.
			// We iterate across the region, expecting to encounter Tag, contourTraj, hatchTraj, Type and then
			// an open-ended number of Edges
			IXMLDOMNode *rcNode = NULL;
			CHK_HR(rcList->get_item(1, &rcNode));	// read Tag
			CHK_HR((rcNode)->get_baseName(&nodeName));
			CHK_HR((rcNode)->get_text(&nodeText));	
			r.tag = (b2s(nodeText));	// tag should be found in region profile list
#if printDOM
			cout << "\t\t\t\t rcLength: " << rcLength << endl;
			printf("\t\t\t\t+%S: %s\n", nodeName, (b2s(nodeText)).c_str());
#endif
			// Addition of contour and trajectory numbers into LAYER schema (October 2019) to
			// enable trajectory and path ordering based on original part inputs, even when multiple parts
			// share the same Tag
			CHK_HR(rcList->get_item(3, &rcNode));	// read contour trajectory number
			CHK_HR((rcNode)->get_baseName(&nodeName));
			CHK_HR((rcNode)->get_text(&nodeText));
			r.contourTraj = stoi(b2s(nodeText));	// contour trajectory# should be integer
#if printDOM
			printf("\t\t\t\t+%S: %s\n", nodeName, (b2s(nodeText)).c_str());
#endif
			CHK_HR(rcList->get_item(5, &rcNode));	// read hatch trajectory number
			CHK_HR((rcNode)->get_baseName(&nodeName));
			CHK_HR((rcNode)->get_text(&nodeText));
			r.hatchTraj = stoi(b2s(nodeText));		// hatch trajectory# should be integer
#if printDOM
			printf("\t\t\t\t+%S: %s\n", nodeName, (b2s(nodeText)).c_str());
#endif
			CHK_HR(rcList->get_item(7, &rcNode));	// read region type
			CHK_HR((rcNode)->get_baseName(&nodeName));
			CHK_HR((rcNode)->get_text(&nodeText));
			r.type = (b2s(nodeText));				// region type should be Inner or Outer
#if printDOM
			printf("\t\t\t\t+%S: %s\n", nodeName, (b2s(nodeText)).c_str());
#endif
			CHK_HR(rcList->get_item(8, &rcNode));	// read edge list
			CHK_HR((rcNode)->get_baseName(&nodeName));			
			IXMLDOMNode *edgeNode = NULL;
			eList.clear();
			for (j = 0; j < rcLength; j++)
			{
				edge e;
				CHK_HR(rcList->nextNode(&edgeNode));
				CHK_HR((edgeNode)->get_baseName(&nodeName));
				if (j>8 && j % 2 == 1)
				{
#if printDOM
					printf("\t\t\t\t+%S\n", nodeName);
#endif

					IXMLDOMNodeList *ecList = NULL;
					CHK_HR(edgeNode->get_childNodes(&ecList));
					IXMLDOMNode *ecNode = NULL;
					CHK_HR(ecList->get_item(1, &ecNode));	// get Start
					CHK_HR((ecNode)->get_baseName(&nodeName));
					CHK_HR((ecNode)->get_text(&nodeText));
#if printDOM
					printf("\t\t\t\t\t+%S: %s\n", nodeName, (b2s(nodeText)).c_str());
#endif
					e.s = vertexList[atoi((b2s(nodeText)).c_str())-1];
					CHK_HR(ecList->get_item(3, &ecNode));	// get End
					CHK_HR((ecNode)->get_baseName(&nodeName));
					CHK_HR((ecNode)->get_text(&nodeText));
#if printDOM
					printf("\t\t\t\t\t+%S: %s\n", nodeName, (b2s(nodeText)).c_str());
#endif
					e.f = vertexList[atoi((b2s(nodeText)).c_str()) - 1];	// e.s, e.f should be real-valued

					eList.push_back(e);
				}
			}
			r.eList = eList;
			rList.push_back(r);			
		}	
	}
	s.rList = rList;
	L.s = s;
	SAFE_RELEASE(pRoot);
	SAFE_RELEASE(rchildList);
	SAFE_RELEASE(node);
	SAFE_RELEASE(node2);
	SAFE_RELEASE(node3);
	SysFreeString(rootName);
	SysFreeString(nodeName);
//	SysFreeString(nodeText);  // excluded because nodeText may be uninitialized
	return L;

CleanUp:
	SAFE_RELEASE(pRoot);
	SAFE_RELEASE(rchildList);
	SAFE_RELEASE(node);
	SAFE_RELEASE(node2);
	SAFE_RELEASE(node3);
	SysFreeString(rootName);
	SysFreeString(nodeName);
//	SysFreeString(nodeText);  // excluded because nodeText may be uninitialized
	return L;
}

int verifyLayerStructure(AMconfig &configData, string layerFilename, layer lyr, vector<string> tagList)
{	// Evaluates key values within the layer structure read from XML against region-tags and other expectations
	// We aggregate all errors and only halt at the end if an error was encountered
	// Currently we only check region metadata; we don't verify that all the vertices are real-valued or are properly closed
	errorCheckStructure errorData;
	string errorMsg;
	bool errFound = false;

	// check layer thickness
	if (lyr.thickness <= 0) {
		errorMsg = "Layer thickness is <= 0 mm in the header of " + layerFilename;
		errFound = true;
		updateErrorResults(errorData, false, "verifyLayerStructure", errorMsg, "", configData.configFilename, configData.configPath);
	}

	// perform various checks on each region
	for (int r = 0; r < lyr.s.rList.size(); r++) {

		// is region tag in config file region profile list?
		std::vector<string>::iterator it;
		it = std::find(tagList.begin(), tagList.end(), lyr.s.rList[r].tag);
		if (it == tagList.end()) {
			// region tag not found in config file list
			errorMsg = layerFilename + " contains a region tag (" + lyr.s.rList[r].tag + ") which is not listed on tab 5 of the configuration file";
			errFound = true;
			updateErrorResults(errorData, false, "verifyLayerStructure", errorMsg, "", configData.configFilename, configData.configPath);
		}

		// is the region type Inner or Outer?
		string rType = lyr.s.rList[r].type;
		std::transform(rType.begin(), rType.end(), rType.begin(),
			[](unsigned char c) { return std::tolower(c); });  // convert region type to lower case
		if (!((rType == "inner") | (rType == "outer")))
		{
			errorMsg = layerFilename + " contains a region type which is not Inner or Outer.  The type is " + lyr.s.rList[r].type;
			errFound = true;
			updateErrorResults(errorData, false, "verifyLayerStructure", errorMsg, "", configData.configFilename, configData.configPath);
		}

		// are the contour & hatch trajectory#'s integers?
		if (lyr.s.rList[r].contourTraj < 0) {
			errorMsg = layerFilename + " contains a contour trajectory number which is less than zero:  " + to_string(lyr.s.rList[r].contourTraj);
			errFound = true;
			updateErrorResults(errorData, false, "verifyLayerStructure", errorMsg, "", configData.configFilename, configData.configPath);
		}
		if (lyr.s.rList[r].hatchTraj < 0) {
			errorMsg = layerFilename + " contains a hatch trajectory number which is less than zero:  " + to_string(lyr.s.rList[r].hatchTraj);
			errFound = true;
			updateErrorResults(errorData, false, "verifyLayerStructure", errorMsg, "", configData.configFilename, configData.configPath);
		}
	}  // end checks on this region

	// if any errors were found, call one more time and halt
	if (errFound == true) {
		errorMsg = "One or more issues were encounted with " + layerFilename + " as listed in the error report file";
		updateErrorResults(errorData, true, "verifyLayerStructure", errorMsg, "", configData.configFilename, configData.configPath);
		return 1;
	}
	else { return 0; }
}
