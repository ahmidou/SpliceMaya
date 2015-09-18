
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

typedef void(*DFGPlugToArgFunc)(
  MPlug &plug,
  MDataBlock &data,
  FabricCore::DFGBinding & binding,
  FabricCore::LockType lockType,
  char const * argName
  );

typedef void(*DFGArgToPlugFunc)(
  FabricCore::DFGBinding & binding,
  FabricCore::LockType lockType,
  char const * argName,
  MPlug &plug,
  MDataBlock &data
  );

DFGPlugToArgFunc getDFGPlugToArgFunc(const std::string & dataType);
DFGArgToPlugFunc getDFGArgToPlugFunc(const std::string & dataType);

#endif
