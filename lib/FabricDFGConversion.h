//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once
#include <iostream>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <FTL/StrRef.h>
#include <FabricCore.h>
#include <FabricSplice.h>

typedef void(*DFGPlugToArgFunc)(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug,
  MDataBlock &data
  );

typedef void(*DFGArgToPlugFunc)(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug,
  MDataBlock &data
  );

DFGPlugToArgFunc getDFGPlugToArgFunc(
  const FTL::StrRef &dataType
  );

DFGArgToPlugFunc getDFGArgToPlugFunc(
  const FTL::StrRef &dataType
  );
