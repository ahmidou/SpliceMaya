Fabric Splice for Autodesk Maya
===================================
A Fabric Splice integration for Maya.

Fabric Splice for Maya allows you to make use of the Fabric Core inside of Maya and use KL to perform computations inside of Maya using a custom node.

Repository Status
=================

This repository will be maintained and kept up to date by Fabric Technologies to match the latest Fabric Core / Fabric Splice.

Supported platforms
===================

To date all three major platforms (windows, linux, osx) are supported, if you build the thirdparty dependencies for the corresponding platform.

Building
========

A scons (http://www.scons.org/) build script is provided. Fabric Splice for Maya depends on
* A static build of boost (http://www.boost.org/), version 1.55 or higher.
* A dynamic build of Fabric Core (matching the latest version).
* The SpliceAPI repository checked out one level above (http://github.com/fabric-engine/SpliceAPI)

Fabric Splice for Maya requires a certain folder structure to build properly. You will need to have the SpliceAPI cloned as well on your drive, as such:

    SpliceAPI
    Applications/SpliceMaya

You can use the bash script below to clone the repositories accordingly:

    git clone git@github.com:fabric-engine/SpliceAPI.git
    mkdir Applications
    cd Applications
    git clone git@github.com:fabric-engine/SpliceMaya.git
    cd SpliceMaya
    scons

To inform scons where to find the Fabric Core includes as well as the thirdparty libraries, you need to set the following environment variables:

* FABRIC_CAPI_DIR: Should point to Fabric Engine's Core folder.
* BOOST_DIR: Should point to the boost root folder (containing boost/ (includes) and lib/ for the static libraries).
* FABRIC_SPLICE_VERSION: Refers to the version you want to build. Typically the name of the branch (for example 1.13.0)
* MAYA_INCLUDE_DIR: The include folder of the Autodesk Maya installation. (for example: C:\Program Files\Autodesk\Maya2015\include)
* MAYA_LIB_DIR: The library folder of the Autodesk Maya installation. (for example: C:\Program Files\Autodesk\Maya2015\lib)
* MAYA_VERSION: The Maya version to use including eventual SP suffix. (for example: 2014SP2)

The temporary files will be built into the *.build* folder, while the structured output files will be placed in the *.stage* folder.

License
==========

The license used for this DCC integration can be found in the root folder of this repository.
