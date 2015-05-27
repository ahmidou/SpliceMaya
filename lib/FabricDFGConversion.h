
#ifndef __FABRICDFGCONVERSION_H__
#define __FABRICDFGCONVERSION_H__

#include <iostream>
#include <vector>
#include <map>
#include <maya/MDataBlock.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MFnData.h>
#include <maya/MFnNumericData.h>
#include <maya/MStringArray.h>
#include <maya/MDataHandle.h>

#include <FabricCore.h>
#include <DFGWrapper/DFGWrapper.h>

typedef void(*DFGPlugToPortFunc)(MPlug &plug, MDataBlock &data, FabricServices::DFGWrapper::PortPtr & port);
typedef void(*DFGExecPortToPlugFunc)(FabricServices::DFGWrapper::PortPtr & port, MPlug &plug, MDataBlock &data);

DFGPlugToPortFunc getDFGPlugToPortFunc(const std::string & dataType, const FabricServices::DFGWrapper::PortPtr port = NULL);
DFGExecPortToPlugFunc getDFGExecPortToPlugFunc(const std::string & dataType, const FabricServices::DFGWrapper::PortPtr port = NULL);

#endif
