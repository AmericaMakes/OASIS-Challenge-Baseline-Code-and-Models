The following is a brief description how to open and use the Open-source Additive Scanning Implementation Strategy (OASIS) Challenge geometry.
Revision 2, release date: August 18, 2020
Michael R. Tucker, GE Research

The geometry contained within this folder has been exported to STL, STP (STEP214) and x_b (Parasolid Binary) formats, and PNG images are included for reference. The units are in millimeters, and the origin of the coordinate system for each part has been set to the origin of the build plate. The input to your scan path generator must be STL format. Note that the build plates and bounding boxes for the mystery part and the user's choice part are included for reference only and will not be part of the print. SOLIDWORKS users: the STP files containing Spiral_lattice_cone_2 do not appear to load correctly as of this release, so please use the Parasolid Binary files instead.

*OASIS_challenge_assy - this is the top-level assembly, including the build plates and bounding boxes. 

*OASIS_challenge_assy_wo_build_plates - same as OASIS_challenge_assy but without build plates. 

*OASIS_challenge_assy_released_parts_only - same as OASIS_challenge_assy_wo_build_plates but without bounding boxes. 

*Mystery_challenge_part_bounding_box - shows the bounding box where the mystery challenge part will be located. A dimensioned drawing of the box is included. The actual mystery part geometry will not be released as part of the challenge.

*Parameter_quality_nut - two of these parts will be printed to demonstrate various aspects of parameter quality, including surface roughness, bulk density, and geometric fidelity.

*Spiral_lattice_cone 1 and 2 - cone 1 is a unidirectional spiral lattice that gets thinner and increases pitch with Z. Cone 2 is similar to cone 1, but is bidirectional. These parts demonstrate the ability to slice and generate paths for parts with a large number of triangles, as well as to print thin lattice structures. SOLIDWORKS users: the STEP file for does not appear to load correctly as of this release, so a binary Parasolid (*.x_b) file has been included.

*Subminiature_Tensile_Bar_Blank - four of these parts will be printed. These parts will be machined into cylindrical test specimens and subjected to tensile testing.

*Users_choice_part_bounding_box - shows the bounding box where the user's choice geometry will be printed. A drawing is included for referenceUsers are free to draw any geometry that fits inside of this bounding box and occupies no more than 30,000 mm^3 of consolidated material, including any supports. Please include features to allow unfused powder to be removed. Note that the part will only be cut off the build plate and that no additional post-processing steps (e.g. support removal, polishing, heat treatment) will be done.

*NIST Test Artifact_ver3b - This is the NIST additive manufacturing test artifact as detailed here: https://www.nist.gov/topics/additive-manufacturing/resources/additive-manufacturing-test-artifact . This part will only be used for evaluating the scan path generation software and will not be printed as part of the challenge.

*OASIS allowable build parameter ranges.pdf - This is a table of the allowable ranges for the user-specifiable build parameters. As the focus of the OASIS challenge is on scan path generation, many process parameters have been fixed.

*OASIS_Input_Config_Build_Plate_Baseline.xls - Prefilled input configuration file for running the OASIS challenge geometry with the baseline precompiled scan path generation code.

*OASIS_Input_Config_NIST_Plate_Baseline.xls - Prefilled input configuration file for running the NIST artifact geometry as part of the OASIS challenge with the baseline precompiled scan path generation code.

Other notes regarding the printer:
* The gas flow direction is from +X to -X (i.e. soot is extracted at the -X end of the build chamber).
* The recoat direction is from +X to -X.