
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
#include <FabricSplice.h>

#include <FTL/CStrRef.h>

struct DFGConversionTimers
{
  FabricSplice::Logging::AutoTimer * globalTimer;

  DFGConversionTimers()
  {
    globalTimer = NULL;
  }

  void stop()
  {
    if(globalTimer != NULL)
      globalTimer->stop();
  }

  void resume()
  {
    if(globalTimer != NULL)
      globalTimer->resume();
  }
};

typedef void(*DFGPlugToArgFunc)(
  MPlug &plug,
  MDataBlock &data,
  FabricCore::DFGBinding & binding,
  FabricCore::LockType lockType,
  char const * argName,
  DFGConversionTimers * timers
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

#endif
