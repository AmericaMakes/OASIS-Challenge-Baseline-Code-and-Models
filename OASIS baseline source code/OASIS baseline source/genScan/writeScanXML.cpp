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
writeScanXML.cpp contains functions which generate an XML structure
(based on the Microsoft Domain Object Model, version 6) from the
scanpaths generated for an individual layer.  The resulting XML
is formatted as per ALSAM 3024 documentation
//============================================================*/

#include "writeScanXML.h"
#include <sstream>
#include "simple_svg_1.0.0.hpp"
#include "constants.h"


// Helper that allocates the BSTR param for the caller.
HRESULT CreateElement(IXMLDOMDocument *pXMLDom, PCWSTR wszName, IXMLDOMElement **ppElement)
{
	HRESULT hr = S_OK;
	*ppElement = NULL;

	BSTR bstrName = SysAllocString(wszName);
	CHK_ALLOC(bstrName);
	CHK_HR(pXMLDom->createElement(bstrName, ppElement));

CleanUp:
	SysFreeString(bstrName);
	return hr;
}

// Helper function to append a child to a parent node.
HRESULT AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent)
{
	HRESULT hr = S_OK;
	IXMLDOMNode *pChildOut = NULL;
	CHK_HR(pParent->appendChild(pChild, &pChildOut));

CleanUp:
	SAFE_RELEASE(pChildOut);
	return hr;
}

// Helper function to create and add a processing instruction to a document node.
HRESULT CreateAndAddPINode(IXMLDOMDocument *pDom, PCWSTR wszTarget, PCWSTR wszData)
{
	HRESULT hr = S_OK;
	IXMLDOMProcessingInstruction *pPI = NULL;

	BSTR bstrTarget = SysAllocString(wszTarget);
	BSTR bstrData = SysAllocString(wszData);
	CHK_ALLOC(bstrTarget && bstrData);

	CHK_HR(pDom->createProcessingInstruction(bstrTarget, bstrData, &pPI));
	CHK_HR(AppendChildToParent(pPI, pDom));

CleanUp:
	SAFE_RELEASE(pPI);
	SysFreeString(bstrTarget);
	SysFreeString(bstrData);
	return hr;
}

// Helper function to create and add a comment to a document node.
HRESULT CreateAndAddCommentNode(IXMLDOMDocument *pDom, PCWSTR wszComment)
{
	HRESULT hr = S_OK;
	IXMLDOMComment *pComment = NULL;

	BSTR bstrComment = SysAllocString(wszComment);
	CHK_ALLOC(bstrComment);

	CHK_HR(pDom->createComment(bstrComment, &pComment));
	CHK_HR(AppendChildToParent(pComment, pDom));

CleanUp:
	SAFE_RELEASE(pComment);
	SysFreeString(bstrComment);
	return hr;
}

// Helper function to create and add an attribute to a parent node.
HRESULT CreateAndAddAttributeNode(IXMLDOMDocument *pDom, PCWSTR wszName, PCWSTR wszValue, IXMLDOMElement *pParent)
{
	HRESULT hr = S_OK;
	IXMLDOMAttribute *pAttribute = NULL;
	IXMLDOMAttribute *pAttributeOut = NULL; // Out param that is not used

	BSTR bstrName = NULL;
	VARIANT varValue;
	VariantInit(&varValue);

	bstrName = SysAllocString(wszName);
	CHK_ALLOC(bstrName);
	CHK_HR(VariantFromString(wszValue, varValue));

	CHK_HR(pDom->createAttribute(bstrName, &pAttribute));
	CHK_HR(pAttribute->put_value(varValue));
	CHK_HR(pParent->setAttributeNode(pAttribute, &pAttributeOut));

CleanUp:
	SAFE_RELEASE(pAttribute);
	SAFE_RELEASE(pAttributeOut);
	SysFreeString(bstrName);
	VariantClear(&varValue);
	return hr;
}

// Helper function to create and append a text node to a parent node.
HRESULT CreateAndAddTextNode(IXMLDOMDocument *pDom, PCWSTR wszText, IXMLDOMNode *pParent)
{
	HRESULT hr = S_OK;
	IXMLDOMText *pText = NULL;

	BSTR bstrText = SysAllocString(wszText);
	CHK_ALLOC(bstrText);

	CHK_HR(pDom->createTextNode(bstrText, &pText));
	CHK_HR(AppendChildToParent(pText, pParent));

CleanUp:
	SAFE_RELEASE(pText);
	SysFreeString(bstrText);
	return hr;
}

// Helper function to create and append a CDATA node to a parent node.
HRESULT CreateAndAddCDATANode(IXMLDOMDocument *pDom, PCWSTR wszCDATA, IXMLDOMNode *pParent)
{
	HRESULT hr = S_OK;
	IXMLDOMCDATASection *pCDATA = NULL;

	BSTR bstrCDATA = SysAllocString(wszCDATA);
	CHK_ALLOC(bstrCDATA);

	CHK_HR(pDom->createCDATASection(bstrCDATA, &pCDATA));
	CHK_HR(AppendChildToParent(pCDATA, pParent));

CleanUp:
	SAFE_RELEASE(pCDATA);
	SysFreeString(bstrCDATA);
	return hr;
}

// Helper function to create and append an element node to a parent node, and pass the newly created element node to caller if it wants.
HRESULT CreateAndAddElementNode(IXMLDOMDocument *pDom, PCWSTR wszName, PCWSTR wszNewline, IXMLDOMNode *pParent, IXMLDOMElement **ppElement)
{
	HRESULT hr = S_OK;
	IXMLDOMElement* pElement = NULL;

	CHK_HR(CreateElement(pDom, wszName, &pElement));
	// Add NEWLINE+TAB for identation before this element.
	CHK_HR(CreateAndAddTextNode(pDom, wszNewline, pParent));
	// Append this element to parent.
	CHK_HR(AppendChildToParent(pElement, pParent));

CleanUp:
	if (ppElement)
		*ppElement = pElement;  // Caller is repsonsible to release this element.
	else
		SAFE_RELEASE(pElement); // Caller is not interested on this element, so release it.

	return hr;
}

vector<trajectory> identifyTrajectories(AMconfig &configData, layer &L, int layerNum)
{
	// This routine identifies trajectories found in the current layer (corresponding to actual parts, not single stripes), and
	// adds in one or more trajectories for single-stripes, if they are included in this layer.
	// L.s is the slice structure which contains a list of regions, rList.
	trajectory t;  // placeholder for a single trajectory and its regions
	vector<trajectory> tl;	// output:  list of all trajectories and their regions
	vector<int> tlValues; // temp variable which lists the trajNum values in tl for easy reference
	int index;
	#if printTrajectories
		cout << "Total number of regions " << L.s.rList.size() << endl;
	#endif

	// 1. Determine if there are any single stripes on this layer.  If so, identify their trajectory#'s and add to trajectoryList
	vector<int> stripeTrajectoriesThisLayer;
	if (configData.allStripesMarked == false)
	{
		stripeTrajectoriesThisLayer = singleStripeCount(layerNum, configData);
	}
	//cout << "*****single stripe count: " << stripeCountThisLayer << endl;
	if (stripeTrajectoriesThisLayer.size() > 0)
	{
		// we have to create at least one stripe trajectory for this layer
		for (int st = 0; st < stripeTrajectoriesThisLayer.size(); st++) {
			// create a trajectory numbered stripeTrajectoriesThisLayer[st] and append to trajectoryList
			trajectory stripeTraj;
			stripeTraj.trajectoryNum = stripeTrajectoriesThisLayer[st];
			stripeTraj.pathProcessingMode = "sequential";
			path stripePath = singleStripes(layerNum, stripeTrajectoriesThisLayer[st], configData);  // define marks and jumps for the stripe path within a particular trajectory#
			stripeTraj.vecPath.push_back(stripePath);
			// add stripe trajectory to trajectoryList.  stripes get written first, in trajectory# order
			tl.push_back(stripeTraj);
			tlValues.push_back(stripeTraj.trajectoryNum);
		}
	}

	// 2. Iterate across the regions found in a layer and record their trajectory#'s.
	// Within r we refer to contourTraj and hatchTraj to identify trajectories
	for (vector<region>::iterator r = L.s.rList.begin(); r != L.s.rList.end(); ++r)
	{	
		#if printTrajectories
			cout << "Looking for contourTraj#" << (*r).contourTraj << endl;
		#endif
		// 1a. See if r.contourTraj exists in tl.trajNum values.
		std::vector<int>::iterator it = std::find(tlValues.begin(), tlValues.end(), (*r).contourTraj);
		if (it != tlValues.end())
		{	// this trajectory# already exists in tl.  Add the region# to that trajectory's region list
			index = std::distance(tlValues.begin(), it);
			tl[index].trajRegions.push_back(std::distance(L.s.rList.begin(), r));
			tl[index].trajRegionTypes.push_back("contour");
			tl[index].trajRegionTags.push_back((*r).tag);
			tl[index].trajRegionIsHatched.push_back(false);
			tl[index].trajRegionLinks.push_back(&*r);
			#if printTrajectories
				cout << "  Added region " << std::distance(L.s.rList.begin(), r) << " to trajectory " << tl[index].trajectoryNum << endl;
			#endif
		}
		else
		{	// r.contourTraj not found; create new trajectory entry.
			// first add the new trajectory# to the list of trajectories
			tlValues.push_back((*r).contourTraj);
			t.trajectoryNum = (*r).contourTraj;
			t.trajRegions.clear();
			t.trajRegionTypes.clear();
			t.trajRegionTags.clear();
			t.trajRegionIsHatched.clear();
			t.trajRegionLinks.clear();
			t.trajRegions.push_back(std::distance(L.s.rList.begin(), r));
			t.trajRegionTypes.push_back("contour");
			t.trajRegionTags.push_back((*r).tag);
			t.trajRegionIsHatched.push_back(false);
			t.trajRegionLinks.push_back(&*r);
			tl.push_back(t);
			#if printTrajectories
				cout << "  Defined trajectory number " << t.trajectoryNum << endl;
			#endif
		}

		// 2a. See if r.hatchTraj exists in tl.trajNum values
		#if printTrajectories
			cout << "Looking for hatchTraj#" << (*r).hatchTraj << endl;
		#endif
		it = std::find(tlValues.begin(), tlValues.end(), (*r).hatchTraj);
		if (it != tlValues.end())
		{	// this trajectory# already exists in tl.  Add the region# to that trajectory's region list
			index = std::distance(tlValues.begin(), it);
			tl[index].trajRegions.push_back(std::distance(L.s.rList.begin(), r));
			tl[index].trajRegionTypes.push_back("hatch");
			tl[index].trajRegionTags.push_back((*r).tag);
			tl[index].trajRegionIsHatched.push_back(false);
			tl[index].trajRegionLinks.push_back(&*r);
			#if printTrajectories
				cout << "  Added region " << std::distance((*L).s.rList.begin(), r) << " to trajectory " << tl[index].trajectoryNum << endl;
			#endif
		}
		else
		{	// r.hatchTraj not found; create new trajectory entry.
			// first add the new trajectory# to the list of trajectories
			tlValues.push_back((*r).hatchTraj);
			t.trajectoryNum = (*r).hatchTraj;
			t.trajRegions.clear();
			t.trajRegionTypes.clear();
			t.trajRegionTags.clear();
			t.trajRegionIsHatched.clear();
			t.trajRegionLinks.clear();
			t.trajRegions.push_back(std::distance(L.s.rList.begin(), r));
			t.trajRegionTypes.push_back("hatch");
			t.trajRegionTags.push_back((*r).tag);
			t.trajRegionIsHatched.push_back(false);
			t.trajRegionLinks.push_back(&*r);
			tl.push_back(t);
			#if printTrajectories
				cout << "  Defined trajectory number " << t.trajectoryNum << endl;
			#endif
		}  // end else hatchTraj
	}  // end for r
	
	// 3. After generating tl, match each trajectory# to a processing instruction in trajProcList, or assign "sequential" if not found
	for (vector<trajectory>::iterator itl = tl.begin(); itl != tl.end(); ++itl)
	{
		(*itl).pathProcessingMode = "sequential";
		// See if (*itl).trajNum is found in (*inputConfig).trajProcList
		for (std::vector<trajectoryProc>::iterator tp = configData.trajProcList.begin();
			tp != configData.trajProcList.end(); ++tp)
		{
			if ((*tp).trajectoryNum == (*itl).trajectoryNum)
				(*itl).pathProcessingMode = (*tp).trajProcessing;
		}
	}

	return tl;
}


void createSCANxmlFile(string fullXMLpath, int layerNum, AMconfig &configData, vector<trajectory> &trajectoryList)
{
	// 1. initialization and setup

	// Define temporary variables
	HRESULT hr = S_OK;
	IXMLDOMElement *pRoot = NULL;
	IXMLDOMElement *trajectoryListNode = NULL;
	BSTR bstrXML = NULL;
	VARIANT varFileName;
	VariantInit(&varFileName);
	wstring wfn(fullXMLpath.begin(), fullXMLpath.end());
	LPCWSTR  wszValue = wfn.c_str();

	// Initialize the Domain Object Model (DOM) and define root element, pRoot
	CHK_HR(CreateAndInitDOM(&pXMLDomScan));
	// Create processing instruction and comment elements.
	CHK_HR(CreateAndAddPINode(pXMLDomScan, L"xml", L"version='1.0'"));
	CHK_HR(CreateAndAddCommentNode(pXMLDomScan, L"Scan file created using MSXML 6.0."));
	// Create the root element, which will be called Layer and is linked by pRoot
	CHK_HR(CreateElement(pXMLDomScan, L"Layer", &pRoot));

	// 2. Add the header to XML
	// TO DO:  add the comment that indicates min / max values across the build(TBD)
	addXMLheader(pRoot, layerNum, configData.layerThickness_mm, configData.dosingFactor);
	
	// 3. Add the velocity profile list to XML
	addXMLvelocityProfileList(pRoot, configData);

	// 4. Add the segment style list to XML
	addXMLsegmentStyleList(pRoot, configData);

	// 5. Add trajectory list to XML, but only if there is at least one trajectory on this layer
	// otherwise, the trajectoryList XML will be improperly terminated as <TrajectoryList/>
	if (trajectoryList.size() > 0) {
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"TrajectoryList", L"\n", pRoot, &trajectoryListNode));
		for (int i = 0; i != trajectoryList.size(); ++i)
		{
			addXMLtrajectory(trajectoryListNode, trajectoryList[i]);
		}
		SAFE_RELEASE(trajectoryListNode);
	}

	// 6. Write the XML DOM to a file
	//
	// Add pRoot and accumulated contents (sub-nodes) to the output
	CHK_HR(AppendChildToParent(pRoot, pXMLDomScan));
	// Prepare to output the XML
	CHK_HR(pXMLDomScan->get_xml(&bstrXML));
	// save the DOM to the target filename
	CHK_HR(VariantFromString(wszValue, varFileName));
	CHK_HR(pXMLDomScan->save(varFileName));

	// 7. Cleanup
	SAFE_RELEASE(pRoot);
	SysFreeString(bstrXML);
	VariantClear(&varFileName);
	return;

CleanUp:
	SAFE_RELEASE(pRoot);
	SysFreeString(bstrXML);
	VariantClear(&varFileName);
	return;
}


void addXMLheader(IXMLDOMElement *pRoot, int layerNum, double thickness, double dosingfactor)
{
	// Temporary vars and definitions
	HRESULT hr = S_OK;
	string st;
	wstring ws;
	IXMLDOMElement *headerNode = NULL;
	IXMLDOMElement *layerNumNode = NULL;
	IXMLDOMElement *layerHeightNode = NULL;
	IXMLDOMElement *thicknessNode = NULL;
	IXMLDOMElement *dosingFactorNode = NULL;
	IXMLDOMElement *schemaVersionNode = NULL;
	IXMLDOMElement *buildDescriptionNode = NULL;	// currently a placeholder - to be replaced with max power, velocity, x, y

	// Define header section
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Header", L"\n", pRoot, &headerNode));

	// Create schema version field
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"AmericaMakesSchemaVersion", L"\n\t", headerNode, &schemaVersionNode));
	ws = wstring(schemaVersion.begin(), schemaVersion.end());
	CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), schemaVersionNode));
	SAFE_RELEASE(schemaVersionNode);

	// Create layer number field
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"LayerNum", L"\n\t", headerNode, &layerNumNode));
	st = to_string(layerNum);
	ws = wstring(st.begin(), st.end());
	CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), layerNumNode));
	SAFE_RELEASE(layerNumNode);

	// Create layer thickness field
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"LayerThickness", L"\n\t", headerNode, &thicknessNode));
	st = d2s((double)thickness);
	ws = wstring(st.begin(), st.end());
	CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), thicknessNode));
	SAFE_RELEASE(thicknessNode);

	// Create absolute height field
	// (In this scanpath code we use constant layer height, so cumulative height = layerNum * thickness)
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"AbsoluteHeight", L"\n\t", headerNode, &layerHeightNode));
	st = d2s((double)(thickness*layerNum));
	ws = wstring(st.begin(), st.end());
	CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), layerHeightNode));
	SAFE_RELEASE(layerHeightNode);

	// Create dosing factor field
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"DosingFactor", L"\n\t", headerNode, &dosingFactorNode));
	st = d2s((double)dosingfactor);
	ws = wstring(st.begin(), st.end());
	CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), dosingFactorNode));
	SAFE_RELEASE(dosingFactorNode);

	// Create build description field
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"BuildDescription", L"\n\t", headerNode, &buildDescriptionNode));
	ws = L"Placeholder";
	CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), buildDescriptionNode));
	SAFE_RELEASE(buildDescriptionNode);
	
	// Close the header
	SAFE_RELEASE(headerNode);
	return;

CleanUp:
	SAFE_RELEASE(headerNode);
	return;
}


void addXMLvelocityProfileList(IXMLDOMElement *pRoot, AMconfig &configData)
{
	// Create the velocity profile list section for an XML SCAN file

	// Temporary vars and definitions
	HRESULT hr = S_OK;
	string st;
	wstring ws;
	// Define velocity profile elements
	IXMLDOMElement *VlNode = NULL;
	IXMLDOMElement *VPNode = NULL;
	IXMLDOMElement *idNode = NULL;
	IXMLDOMElement *VNode = NULL;
	IXMLDOMElement *mNode = NULL;
	IXMLDOMElement *tv1Node = NULL;	// these next five elements are delay values, using their original terms
	IXMLDOMElement *tv2Node = NULL;
	IXMLDOMElement *tl1Node = NULL;
	IXMLDOMElement *tl2Node = NULL;
	IXMLDOMElement *tl3Node = NULL;

	// Create velocity profile section
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"VelocityProfileList", L"\n", pRoot, &VlNode));
	for (vector<velocityProfile>::iterator it = (configData.VPlist).begin(); it != (configData.VPlist).end(); ++it)
	{
		if ((*it).isUsed == true)	// only output the profiles which are actually used in the build and/or this specific layer
		{
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"VelocityProfile", L"\n\t", VlNode, &VPNode));

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"ID", L"", VPNode, &idNode));
			if (configData.outputIntegerIDs == true) {  // determine whether to output the original velocity profile ID string or an auto-generated integer ID
				st = to_string((*it).integerID);
			}
			else {
				st = (*it).ID;
			}
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), idNode));
			SAFE_RELEASE(idNode);
			
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Velocity", L"\n\t\t", VPNode, &VNode));
			st = d2s((*it).velocity);
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), VNode));
			SAFE_RELEASE(VNode);
			
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Mode", L"", VPNode, &mNode));
			st = (*it).mode;
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), mNode));
			SAFE_RELEASE(mNode);

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"LaserOnDelay", L"\n\t\t", VPNode, &tv1Node));
			st = d2s((*it).laserOnDelay);
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), tv1Node));
			SAFE_RELEASE(tv1Node);

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"LaserOffDelay", L"", VPNode, &tv2Node));
			st = d2s((*it).laserOffDelay);
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), tv2Node));
			SAFE_RELEASE(tv2Node);

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"JumpDelay", L"", VPNode, &tl1Node));
			st = d2s((*it).jumpDelay);
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), tl1Node));
			SAFE_RELEASE(tl1Node);

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"MarkDelay", L"\n\t\t", VPNode, &tl2Node));
			st = d2s((*it).markDelay);
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), tl2Node));
			SAFE_RELEASE(tl2Node);

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"PolygonDelay", L"", VPNode, &tl3Node));
			st = d2s((*it).polygonDelay);
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), tl3Node));
			SAFE_RELEASE(tl3Node);

			SAFE_RELEASE(VPNode);
		} // end if isUsed
	}
	SAFE_RELEASE(VlNode);
	return;

CleanUp:
	SAFE_RELEASE(VlNode);
	return;
}


void addXMLsegmentStyleList(IXMLDOMElement *pRoot, AMconfig &configData)
{	// Create the segment style list section for an XML SCAN file

	// Temporary vars and definitions
	HRESULT hr = S_OK;
	string st;
	wstring ws;
	// Define segment style elements
	IXMLDOMElement *SsListNode = NULL;
	IXMLDOMElement *SsNode = NULL;
	IXMLDOMElement *SSIdNode = NULL;
	IXMLDOMElement *VpIdNode = NULL;
	IXMLDOMElement *LaserModeNode = NULL;
	IXMLDOMElement *TravelerNode = NULL;
	IXMLDOMElement *TravIdNode = NULL;
	IXMLDOMElement *PowerNode = NULL;
	IXMLDOMElement *SyncDelayNode = NULL;
	IXMLDOMElement *SpotSizeNode = NULL;
	IXMLDOMElement *WobbleNode = NULL;
	IXMLDOMElement *WobbleOnNode = NULL;
	IXMLDOMElement *WobFreqNode = NULL;
	IXMLDOMElement *WobShapeNode = NULL;
	IXMLDOMElement *WobTransAmpNode = NULL;
	IXMLDOMElement *WobLongAmpNode = NULL;

	//*********************************************
	// Create Segment Style section
	/* Iterate over the SSlist vector
	if the Style is used in the build, continue
	output the ID, VPid and LaserMode (which should be pre-populated)
	If traveler1 ID is populated, output section including ID, syncdelay, power, spotsize, wobble
	if wobble1 is true, populate its fields
	if traveler2 ID is (also) populated, output ID, syncdelay, power, spotsize, wobble
	if wobble2 is true, populate its fields
	*/
	// Define the segment style list node
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SegmentStyleList", L"\n", pRoot, &SsListNode));
	for (vector<segmentStyle>::iterator it = (configData.segmentStyleList).begin(); it != (configData.segmentStyleList).end(); ++it)
	{
		if ((*it).isUsed == true)	// only output the styles which are actually used in the build and/or this specific layer
		{
			// Define an individual segment style
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SegmentStyle", L"\n\t", SsListNode, &SsNode));

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"ID", L"", SsNode, &SSIdNode));
			if (configData.outputIntegerIDs == true) {  // determine whether to output the original segment style ID string or an auto-generated integer ID
				st = to_string((*it).integerID);
			}
			else {
				st = (*it).ID;
			}
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), SSIdNode));
			SAFE_RELEASE(SSIdNode);

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"VelocityProfileID", L"\n\t\t", SsNode, &VpIdNode));
			if (configData.outputIntegerIDs == true) {  // determine whether to output the velocity profile ID string, or the auto-generated integer ID
				st = to_string((*it).vpIntID);
			}
			else {
				st = (*it).vpID;
			}
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), VpIdNode));
			SAFE_RELEASE(VpIdNode);

			if ((*it).laserMode != "")
			{
				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"LaserMode", L"", SsNode, &LaserModeNode));
				st = (*it).laserMode;
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), LaserModeNode));
				SAFE_RELEASE(LaserModeNode);
			}

			// If lead laser ID is populated, output a Traveler section including ID, syncdelay, power, spotsize, wobble
			if ((*it).leadLaser.travelerID != "")
			{
				// Create a Traveler node
				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Traveler", L"\n\t\t", SsNode, &TravelerNode));

				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"ID", L"", TravelerNode, &TravIdNode));
				st = (*it).leadLaser.travelerID;
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), TravIdNode));
				SAFE_RELEASE(TravIdNode);

				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SyncDelay", L"\n\t\t\t", TravelerNode, &SyncDelayNode));
				st = d2s((*it).leadLaser.syncOffset);
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), SyncDelayNode));
				SAFE_RELEASE(SyncDelayNode);

				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Power", L"", TravelerNode, &PowerNode));
				st = d2s((*it).leadLaser.power);
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), PowerNode));
				SAFE_RELEASE(PowerNode);

				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SpotSize", L"", TravelerNode, &SpotSizeNode));
				st = d2s((*it).leadLaser.spotSize);
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), SpotSizeNode));
				SAFE_RELEASE(SpotSizeNode);

				// If wobble is enabled for this style, also output the remaining wobble parameters
				if ((*it).leadLaser.wobble == true)
				{
					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Wobble", L"\n\t\t\t", TravelerNode, &WobbleNode));

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"On", L"", WobbleNode, &WobbleOnNode));
					st = "1"; // 1 = wobble is on
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobbleOnNode));
					SAFE_RELEASE(WobbleOnNode);

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Freq", L"", WobbleNode, &WobFreqNode));
					st = d2s((*it).leadLaser.wobFrequency);
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobFreqNode));
					SAFE_RELEASE(WobFreqNode);

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Shape", L"", WobbleNode, &WobShapeNode));
					st = to_string((*it).leadLaser.wobShape);
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobShapeNode));
					SAFE_RELEASE(WobShapeNode);

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"TransAmp", L"", WobbleNode, &WobTransAmpNode));
					st = d2s((*it).leadLaser.wobTransAmp);
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobTransAmpNode));
					SAFE_RELEASE(WobTransAmpNode);

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"LongAmp", L"", WobbleNode, &WobLongAmpNode));
					st = d2s((*it).leadLaser.wobLongAmp);
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobLongAmpNode));
					SAFE_RELEASE(WobLongAmpNode);

					SAFE_RELEASE(WobbleNode);
				} // end leadLaser.wobble
				SAFE_RELEASE(TravelerNode);
			}

			// If traveler2 ID is populated, output a Traveler section including ID, syncdelay, power, spotsize, wobble
			if ((*it).trailLaser.travelerID != "")
			{
				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Traveler", L"\n\t\t", SsNode, &TravelerNode));

				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"ID", L"", TravelerNode, &TravIdNode));
				st = (*it).trailLaser.travelerID;
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), TravIdNode));
				SAFE_RELEASE(TravIdNode);

				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SyncDelay", L"\n\t\t\t", TravelerNode, &SyncDelayNode));
				st = d2s((*it).trailLaser.syncOffset);
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), SyncDelayNode));
				SAFE_RELEASE(SyncDelayNode);

				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Power", L"", TravelerNode, &PowerNode));
				st = d2s((*it).trailLaser.power);
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), PowerNode));
				SAFE_RELEASE(PowerNode);

				CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SpotSize", L"", TravelerNode, &SpotSizeNode));
				st = d2s((*it).trailLaser.spotSize);
				ws = wstring(st.begin(), st.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), SpotSizeNode));
				SAFE_RELEASE(SpotSizeNode);

				// If wobble is enabled for this style, also output the remaining wobble parameters
				if ((*it).trailLaser.wobble == true)
				{
					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Wobble", L"\n\t\t\t", TravelerNode, &WobbleNode));

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"On", L"", WobbleNode, &WobbleOnNode));
					st = "1"; // 1 = wobble is on
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobbleOnNode));
					SAFE_RELEASE(WobbleOnNode);

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Freq", L"", WobbleNode, &WobFreqNode));
					st = d2s((*it).trailLaser.wobFrequency);
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobFreqNode));
					SAFE_RELEASE(WobFreqNode);

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Shape", L"", WobbleNode, &WobShapeNode));
					st = to_string((*it).trailLaser.wobShape);
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobShapeNode));
					SAFE_RELEASE(WobShapeNode);

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"TransAmp", L"", WobbleNode, &WobTransAmpNode));
					st = d2s((*it).trailLaser.wobTransAmp);
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobTransAmpNode));
					SAFE_RELEASE(WobTransAmpNode);

					CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"LongAmp", L"", WobbleNode, &WobLongAmpNode));
					st = d2s((*it).trailLaser.wobLongAmp);
					ws = wstring(st.begin(), st.end());
					CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), WobLongAmpNode));
					SAFE_RELEASE(WobLongAmpNode);

					SAFE_RELEASE(WobbleNode);
				} // end leadLaser.wobble
				SAFE_RELEASE(TravelerNode);
			}
			SAFE_RELEASE(SsNode);
		} // end if isUsed
	}
	SAFE_RELEASE(SsListNode);
	return;

CleanUp:
	SAFE_RELEASE(SsListNode);
	return;
}


void addXMLtrajectory(IXMLDOMElement *trajList, trajectory &T)
{	// Adds an individual trajectory section to the XML SCAN file
	
	// Temporary vars and definitions
	HRESULT hr = S_OK;
	wstring ws;
	string st;
	int p = scanCoordPrecision;
	
	// Define trajectory elements
	IXMLDOMElement *TNode = NULL;
	IXMLDOMElement *tidNode = NULL;
	IXMLDOMElement *pathProcNode = NULL;
	IXMLDOMElement *sdNode = NULL;
	IXMLDOMElement *ptNode = NULL;
	IXMLDOMElement *tyNode = NULL;
	IXMLDOMElement *tgNode = NULL;
	IXMLDOMElement *szNode = NULL;
	IXMLDOMElement *skyWrNode = NULL;
	IXMLDOMElement *stNode = NULL;
	IXMLDOMElement *sxNode = NULL;
	IXMLDOMElement *syNode = NULL;
	IXMLDOMElement *svNode = NULL;
	IXMLDOMElement *sidNode = NULL;
	IXMLDOMElement *fNode = NULL;
	IXMLDOMElement *fxNode = NULL;
	IXMLDOMElement *fyNode = NULL;
	IXMLDOMElement *segStyleNode = NULL;
	IXMLDOMDocumentFragment *pDF = NULL;
	BSTR bstrXML = NULL;
	VARIANT varFileName;
	VariantInit(&varFileName);

	// create Trajectory node, which will encompass everything else for this trajectory
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Trajectory", L"\n", trajList, &TNode));

	// add trajectoryID
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"TrajectoryID", L"\n\t", TNode, &tidNode));
	st = to_string(T.trajectoryNum);
	ws = wstring(st.begin(), st.end());
	CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), tidNode));
	SAFE_RELEASE(tidNode);
	
	// add path processing mode
	CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"PathProcessingMode", L"\n\t", TNode, &pathProcNode));
	st = T.pathProcessingMode;
	ws = wstring(st.begin(), st.end());
	CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), pathProcNode));
	SAFE_RELEASE(pathProcNode);

	// Iterate over the paths within this trajectory
	for (vector<path>::iterator pt = (T.vecPath).begin(); pt != (T.vecPath).end(); ++pt)
	{
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Path", L"\n\t", TNode, &ptNode));
		// Add path type (hatch or contour)
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Type", L"\n\t\t", ptNode, &tyNode));
		st = (*pt).type;
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), tyNode));
		SAFE_RELEASE(tyNode);

		// Add path tag
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Tag", L"\n\t\t", ptNode, &tgNode));
		st = (*pt).tag;
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), tgNode));
		SAFE_RELEASE(tgNode);

		// Add number of segments
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"NumSegments", L"\n\t\t", ptNode, &szNode));
		st = to_string(((*pt).vecSg).size());
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), szNode));
		SAFE_RELEASE(szNode);

		// Add skywriting mode
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SkyWritingMode", L"\n\t\t", ptNode, &skyWrNode));
		st = d2s((*pt).SkyWritingMode);
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), skyWrNode));
		SAFE_RELEASE(skyWrNode);

		// Add start coordinate for the segment list
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Start", L"\n\t\t", ptNode, &stNode));
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"X", L"", stNode, &sxNode));
		std::stringstream startx;	// using stringstream allows us to set precision
		startx << std::fixed << std::setprecision(p) << (((*pt).vecSg[0]).start).x;
		st = startx.str();
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), sxNode));
		SAFE_RELEASE(sxNode);
		CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Y", L"", stNode, &syNode));
		std::stringstream starty;	// using stringstream allows us to set precision
		starty << std::fixed << std::setprecision(p) << (((*pt).vecSg[0]).start).y;
		st = starty.str();
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), syNode));
		SAFE_RELEASE(syNode);
		SAFE_RELEASE(stNode);

		// Add the list of segments
		for (vector<segment>::iterator gt = ((*pt).vecSg).begin(); gt != ((*pt).vecSg).end(); ++gt)
		{
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Segment", L"\n\t\t", ptNode, &svNode));
/*			// SegmentID is currently omitted, since this program is not currently generating individual segment identifiers
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SegmentID", L"\n\t", svNode, &sidNode));
			st = d2s((*gt).id);
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), sidNode));
			SAFE_RELEASE(sidNode);	*/

			// Add segment style
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"SegStyle", L"", svNode, &segStyleNode));
			st = (*gt).idSegStyl;  // a string containing the style's integer or string ID, as coded in hatch() or contour() functions
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), segStyleNode));
			SAFE_RELEASE(segStyleNode);

			// Add endpoint coordinate for this segment
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"End", L"", svNode, &fNode));
			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"X", L"", fNode, &fxNode));

			std::stringstream strx;	// using stringstream allows us to set precision
			strx << std::fixed << std::setprecision(p) << ((*gt).end).x;
			st = strx.str();
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), fxNode));
			SAFE_RELEASE(fxNode);

			CHK_HR(CreateAndAddElementNode(pXMLDomScan, L"Y", L"", fNode, &fyNode));
			std::stringstream stry;	// using stringstream allows us to set precision
			stry << std::fixed << std::setprecision(p) << ((*gt).end).y;
			st = stry.str();
			ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomScan, ws.c_str(), fyNode));
			SAFE_RELEASE(fyNode);
			SAFE_RELEASE(fNode);
			SAFE_RELEASE(svNode);
		}
		SAFE_RELEASE(ptNode);
	} // end for list of paths

	SAFE_RELEASE(TNode);
	return;

CleanUp:
	SAFE_RELEASE(TNode);
	return;
}


void scan2SVG(string fn, vector<trajectory> &tList, int dim, double mag, double xo, double yo)
{
	svg::Dimensions dimensions(dim, dim);
	svg::Document doc(fn, svg::Layout(dimensions, svg::Layout::TopLeft));
	int numTrajectories = tList.size();
	vertex s, f;
	double sx, sy, fx, fy;
	int flip = 0;
	int start = 0;

	// iterate over all trajectories
	for (int t = 0; t != numTrajectories; ++t)
	{
		for (int i = 0; i < (int)(tList[t].vecPath).size(); i++)
		{
			vector<segment> SP = (tList[t].vecPath[i]).vecSg;
			string type = (tList[t].vecPath[i]).type;
			string tag = (tList[t].vecPath[i]).tag;
			s = SP[0].start;
			for (vector<segment>::iterator it = SP.begin(); it != SP.end(); ++it)
			{
				f = (*it).end;
				sx = s.x * mag + xo;
				sy = s.y * mag + yo;
				fx = f.x * mag + xo;
				fy = f.y * mag + yo;
				if ((*it).isMark == 1) // show only marks, not jumps
				{
					svg::Line line(svg::Point(sx, dim - sy), svg::Point(fx, dim - fy), svg::Stroke(0.25, svg::Color::Black));
					doc << line;
				}
				//else { cout << "encounteredJump" << endl; }  // testing purposes
				s = f;
			}
		}
	}

	doc.save();
};

PCWSTR d2lp(double in)
{
	wstringstream wss;
	wss << in;
	return (wss.str()).c_str();
}

PCWSTR i2lp(int in)
{
	wstringstream wss;
	wss << in;
	return (wss.str()).c_str();
}

string d2s(double d)
{
	std::ostringstream oss;
	oss.precision(std::numeric_limits<double>::digits10);
	oss << std::fixed << d;
	std::string str = oss.str();
	std::size_t pos1 = str.find_last_not_of("0");
	if (pos1 != std::string::npos)
		str.erase(pos1 + 1);

	std::size_t pos2 = str.find_last_not_of(".");
	if (pos2 != std::string::npos)
		str.erase(pos2 + 1);

	return str;
}
