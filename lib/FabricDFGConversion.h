//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

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
#include <FabricSplice.h>

#include <FTL/CStrRef.h>

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

DFGPlugToArgFunc getDFGPlugToArgFunc(const FTL::CStrRef &dataType);
DFGArgToPlugFunc getDFGArgToPlugFunc(const FTL::CStrRef &dataType);
