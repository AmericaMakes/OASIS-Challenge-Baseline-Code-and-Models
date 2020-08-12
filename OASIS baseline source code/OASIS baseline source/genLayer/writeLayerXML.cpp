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
writeLayerXML.cpp contains functions to convert the vertices and edges
of a SVG-formatted layer file into an XML format, based on
the Microsoft Domain Object Model (DOM) version 6, as described
in msxml6.h

The output XML format is described in ALSAM3024 documentation,
and describes edges and vertices and assigned a region tag
to each inner/outer outline based on the part as directed by
the Excel configuration file
//============================================================*/

#include <sstream>
#include "writeLayerXML.h"

#include "constants.h"
#include "readExcelConfig.h"

//IXMLDOMDocument *pXMLDomLayer;


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

// Helper function to create and append an element node to a parent node, and pass the newly created
// element node to caller if it wants.
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

HRESULT CreateAndInitDOM(IXMLDOMDocument **ppDoc)
{
	HRESULT hr = CoCreateInstance(__uuidof(DOMDocument60), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(ppDoc));
	if (SUCCEEDED(hr))
	{
		// these methods should not fail so don't inspect result
		(*ppDoc)->put_async(VARIANT_FALSE);
		(*ppDoc)->put_validateOnParse(VARIANT_FALSE);
		(*ppDoc)->put_resolveExternals(VARIANT_FALSE);
		(*ppDoc)->put_preserveWhiteSpace(VARIANT_TRUE);
	}
	return hr;
}

void writeLayer(string fn, layer L)
{
	int p = layerCoordPrecision;

	HRESULT hr = S_OK;
	IXMLDOMElement *pRoot = NULL;
	IXMLDOMElement *VlNode = NULL;
	IXMLDOMElement *VNode = NULL;
	IXMLDOMElement *XNode = NULL;
	IXMLDOMElement *YNode = NULL;
	IXMLDOMElement *CNode = NULL;
	IXMLDOMElement *tNode = NULL;
	IXMLDOMElement *SNode = NULL;
	IXMLDOMElement *RNode = NULL;
	IXMLDOMElement *ENode = NULL;
	IXMLDOMElement *NNode = NULL;
	IXMLDOMElement *NxNode = NULL;
	IXMLDOMElement *NyNode = NULL;
	IXMLDOMElement *NzNode = NULL;
	IXMLDOMElement *IsNode = NULL;
	IXMLDOMElement *IfNode = NULL;
	IXMLDOMDocumentFragment *pDF = NULL;
	BSTR bstrXML = NULL;
	VARIANT varFileName;
	VariantInit(&varFileName);
	slice s = L.us;
	vector<region> rlist = s.rList;
	wstring wfn(fn.begin(), fn.end());
	LPCWSTR  wszValue = wfn.c_str();
	wstring ws;
	string st;
	IXMLDOMDocument *pXMLDomLayer = NULL;
	CHK_HR(CreateAndInitDOM(&pXMLDomLayer));
	
	// Create a processing instruction element.
	CHK_HR(CreateAndAddPINode(pXMLDomLayer, L"xml", L"version='1.0'"));
	// Create a comment element.
	CHK_HR(CreateAndAddCommentNode(pXMLDomLayer, L"America Makes layer file created using MSXML 6.0"));
	// Create the root element.
	CHK_HR(CreateElement(pXMLDomLayer, L"Layer", &pRoot));
	// Create an attribute for the <root> element, with name "created" and value "using dom".
	CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Thickness", L"\n\t", pRoot, &tNode));
		st = d2s(L.thickness);
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), tNode));
	SAFE_RELEASE(tNode);
	CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"VertexList", L"\n\t", pRoot, &VlNode));
	bool firstVertex = true;
	for (vector<vertex>::iterator it = (L.vList).begin(); it != (L.vList).end(); ++it)
	{
		CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Vertex", L"\n\t", VlNode, &VNode));
		CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"X", L"\n\t", VNode, &XNode));
		std::stringstream vert_x;	// using stringstream allows us to set precision
		vert_x << std::fixed << std::setprecision(p) << (*it).x;
		st = vert_x.str();
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), XNode));
		SAFE_RELEASE(XNode);
		CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Y", L"\n\t", VNode, &YNode));
		std::stringstream vert_y;	// using stringstream allows us to set precision
		vert_y << std::fixed << std::setprecision(p) << (*it).y;
		st = vert_y.str();
		ws = wstring(st.begin(), st.end());
		CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), YNode));
		SAFE_RELEASE(YNode);
		// Depending on the setting of the constant outputCoordSystem, output coordinate system for every vertex or only the first
		if ((firstVertex == true) | (outputCoordSystem == true))
		{
			CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Co-ordinate_system", L"\n\t", VNode, &CNode));
				CHK_HR(CreateAndAddTextNode(pXMLDomLayer, L"Cartesian", CNode));
			SAFE_RELEASE(CNode);
			firstVertex = false;
		}
		SAFE_RELEASE(VNode);		
	}
	SAFE_RELEASE(VlNode);
		
	CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Slice", L"\n\t", pRoot, &SNode));
	for (vector<region>::iterator it = rlist.begin(); it != rlist.end(); ++it)
	{
		string tag = (*it).tag;
		string type = (*it).type;
		int contourTraj = (*it).contourTraj;
		int hatchTraj = (*it).hatchTraj;
		// Region
		CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Region", L"\n\t", SNode, &RNode));
			// Region tag
			CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Tag", L"\n\t", RNode, &tNode));
				ws = wstring(tag.begin(), tag.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), tNode));
			SAFE_RELEASE(tNode);
			// Contour and hatch trajectory numbers, a proxy for build ordering
			CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"contourTraj", L"\n\t", RNode, &tNode));
				st = to_string(contourTraj);
				ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), tNode));
			SAFE_RELEASE(tNode);
			CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"hatchTraj", L"\n\t", RNode, &tNode));
				st = to_string(hatchTraj);
				ws = wstring(st.begin(), st.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), tNode));
			SAFE_RELEASE(tNode);
			// Type of loop (inner or outer)
			CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Type", L"\n\t", RNode, &tNode));
				ws = wstring(type.begin(), type.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomLayer,ws.c_str(), tNode));
			SAFE_RELEASE(tNode);
			vector<edge> elist = (*it).eList;
			for (vector<edge>::iterator et = elist.begin(); et != elist.end(); ++et)
			{
				CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Edge", L"\n\t", RNode, &ENode));
					CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Start", L"\n\t", ENode, &IsNode));
						std::stringstream eStart;	// using stringstream allows us to set precision
						eStart << std::fixed << std::setprecision(p) << (*et).start_idx;
						st = eStart.str();
						ws = wstring(st.begin(), st.end());
						CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), IsNode));
					SAFE_RELEASE(IsNode);
					CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"End", L"\n\t", ENode, &IfNode));
						std::stringstream eEnd;	// using stringstream allows us to set precision
						eEnd << std::fixed << std::setprecision(p) << (*et).end_idx;
						st = eEnd.str();
						ws = wstring(st.begin(), st.end());
						CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), IfNode));
					SAFE_RELEASE(IfNode);
					CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Normal", L"\n\t", ENode, &NNode));
						CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Nx", L"\n\t", NNode, &NxNode));
							CHK_HR(CreateAndAddTextNode(pXMLDomLayer, L"0", NxNode));
						SAFE_RELEASE(NxNode);
						CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Ny", L"\n\t", NNode, &NyNode));
							CHK_HR(CreateAndAddTextNode(pXMLDomLayer, L"0", NyNode));
						SAFE_RELEASE(NyNode);
						CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Nz", L"\n\t", NNode, &NzNode));
							CHK_HR(CreateAndAddTextNode(pXMLDomLayer, L"0", NzNode));
						SAFE_RELEASE(NzNode);
					SAFE_RELEASE(NNode);					
				SAFE_RELEASE(ENode);
			}
		SAFE_RELEASE(RNode);
	}
	SAFE_RELEASE(SNode);
	CHK_HR(CreateAndAddTextNode(pXMLDomLayer, L"\n", pRoot));

	// add <root> to document
	CHK_HR(AppendChildToParent(pRoot, pXMLDomLayer));
	CHK_HR(pXMLDomLayer->get_xml(&bstrXML));
	CHK_HR(VariantFromString(wszValue, varFileName));
	CHK_HR(pXMLDomLayer->save(varFileName));
CleanUp:
	SAFE_RELEASE(pRoot);
	SAFE_RELEASE(VlNode);
	SAFE_RELEASE(pDF);
	SysFreeString(bstrXML);
	VariantClear(&varFileName);
};

void writeHeader(string fn, vector<Linfo> li, int numLayer)
{
	HRESULT hr = S_OK;
	IXMLDOMElement *pRoot = NULL;
	IXMLDOMElement *nNode = NULL;
	IXMLDOMElement *lNode = NULL;
	IXMLDOMElement *lhNode = NULL;
	IXMLDOMElement *lnNode = NULL;
	IXMLDOMDocumentFragment *pDF = NULL;
	BSTR bstrXML = NULL;
	VARIANT varFileName;
	VariantInit(&varFileName);	
	wstring wfn(fn.begin(), fn.end());
	LPCWSTR  wszValue = wfn.c_str();
	wstring ws;
	string s;
	IXMLDOMDocument *pXMLDomLayer = NULL;
	CHK_HR(CreateAndInitDOM(&pXMLDomLayer));
	// Create a processing instruction element.
	CHK_HR(CreateAndAddPINode(pXMLDomLayer, L"xml", L"version='1.0'"));
	// Create a comment element.
	CHK_HR(CreateAndAddCommentNode(pXMLDomLayer, L"Header file created using MSXML 6.0."));
	// Create the root element.
	CHK_HR(CreateElement(pXMLDomLayer, L"Object", &pRoot));
		CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"No._of_Layers", L"\n\t", pRoot, &nNode));
			s = d2s((double)numLayer+1);
			ws = wstring(s.begin(), s.end());
			CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), nNode));
		SAFE_RELEASE(nNode);
	for (vector<Linfo>::iterator it = li.begin(); it != li.end(); ++it)
	{		
		CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Layer_info", L"\n\t", pRoot, &lNode));
			CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"z_Height", L"\n\t", lNode, &lhNode));
				s = d2s((*it).zHeight);
				ws = wstring(s.begin(),s.end());
				CHK_HR(CreateAndAddTextNode(pXMLDomLayer,ws.c_str(), lhNode));
			SAFE_RELEASE(lhNode);
			CHK_HR(CreateAndAddElementNode(pXMLDomLayer, L"Layer_filename", L"\n\t", lNode, &lnNode));
				ws = wstring(((*it).fn).begin(), ((*it).fn).end());
				CHK_HR(CreateAndAddTextNode(pXMLDomLayer, ws.c_str(), lnNode));
			SAFE_RELEASE(lnNode);
		SAFE_RELEASE(lNode);
		
	}
	CHK_HR(CreateAndAddTextNode(pXMLDomLayer, L"\n", pRoot));

	// add <root> to document
	CHK_HR(AppendChildToParent(pRoot, pXMLDomLayer));
	CHK_HR(pXMLDomLayer->get_xml(&bstrXML));
	CHK_HR(VariantFromString(wszValue, varFileName));
	CHK_HR(pXMLDomLayer->save(varFileName));

CleanUp:
	SAFE_RELEASE(pRoot);
	SAFE_RELEASE(pDF);
	SysFreeString(bstrXML);
	VariantClear(&varFileName);
}

PCWSTR d2lp(double in)
{
	wstringstream wss;
	wss << in;
	return (wss.str()).c_str();
}

PCWSTR s2lp(string in)
{
	wstring win;
	win = wstring(in.begin(), in.end());	
	LPCWSTR out = win.c_str();
	return out;
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