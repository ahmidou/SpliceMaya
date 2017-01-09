//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricDFGProfiling.h"
#include <maya/MGlobal.h>

FTL::ProfilingStack FabricMayaProfilingEvent::s_stack(false);

void FabricMayaProfilingEvent::startProfiling()
{
  s_stack = FTL::ProfilingStack(true);
}

void FabricMayaProfilingEvent::stopProfiling()
{
  // todo: print
  MString report = s_stack.getReportString().c_str();
  MGlobal::displayInfo("\n" + report);
  s_stack = FTL::ProfilingStack(false);
}
