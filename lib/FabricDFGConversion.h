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

#include <FTL/StrRef.h>

typedef void(*DFGPlugToArgFunc)(
  MPlug &plug,
  MDataBlock &data,
  FabricCore::DFGBinding & binding,
  FabricCore::LockType lockType,
  char const * argName
  );

typedef void(*DFGArgToPlugFunc)(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug,
  MDataBlock &data
  );

DFGPlugToArgFunc getDFGPlugToArgFunc(const FTL::StrRef &dataType);
DFGArgToPlugFunc getDFGArgToPlugFunc(const FTL::StrRef &dataType);
