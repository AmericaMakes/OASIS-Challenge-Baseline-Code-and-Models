# Multilaser_Precompiled-binaries
Compiled code and configuration files to generate layers and scanpaths in the ALSAM3024 multi-laser XML schema

This repository is separate from the source code repo to allow users to sync to the source code repo without also syncing up all new builds added to their local copy of precompiled-binaries

One external package, slic3r, must be downloaded separately to use the layer-generation code.  Slic3r has incompatible license terms and is larger than the entire ALSAM package, so it was not compiled with the ALSAM source code. Per the get-started guide, download slic3r version 1.3.0 for 64-bit Windows from www.slic3r.org and extract the contents into the slic3r_130 folder already present in the Executables folder.  The get-started guide includes a screenshot of the intended folder and file structure

To execute the code, run generateScanpaths.exe (found within the Executables folder).  You will be prompted to select an Excel configuration file, which defines the parts (STL files) and scanpath methods included in a particular project.  Several examples of the configuration file are included in the Project Examples folder, along with a blank file for your use.  Note that the configuration file must be in .xls format rather than not .xlsx (this permitted fully open-source code since no ready open-source packages for reading .xlsx format were easily located).  Excel files in this format may be edited with the OpenOffice software suite

See the get-started guide and additional documentation for more
