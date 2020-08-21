# OASIS-Challenge-Baseline-Code-and-Models
This repository contains the baseline code and models for the OASIS Challenge

The code generates scanpaths in an open-source XML format which can be used for laser fusion additive builds.  Full documentation on the the XML scan schema and the baseline code (its intent, capabilities, limitations and usage) is located within this repository at 'OASIS baseline source code/Documentation/'

The 'OASIS baseline precompiled binaries' folder contains executables and several example builds, which each consist of an Excel configuration file and one or more parts in STL format. As documented in the quick-start guide, to execute the baseline compiled code, users will need to download the free Slic3r package from www.slic3r.org and unpack it into the empty slic3r_130 folder located within 'OASIS baseline precompiled binaries/Executables/'

The baseline code applies a single (uniform) scan strategy to each part as referenced by the Region tab of the configuration file.  Competitors may improve upon the performance of this code by applying different region profiles to various areas of a part based on geometry or other factors

The OASIS baseline source code is divided into layer and scan file generation as described in the source code documentation.  All scanpath-generation functions are performed except for STL-file slicing, which is handled by a call to slic3r.exe

What to do...

1.) Clone or download the files in this repository.

2.) Thoroughly review the "OASIS Challenge Documentation_202000821.pdf".

3.) Thoroughly review all the documentation contained within 'OASIS baseline source code/Documentation/'

4.) Happy Coding!!
