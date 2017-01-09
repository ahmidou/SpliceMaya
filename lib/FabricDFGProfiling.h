//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <FTL/Profiling.h>

class FabricMayaProfilingEvent : public FTL::AutoProfilingEvent
{
public:

  FabricMayaProfilingEvent(char const * label)
  : FTL::AutoProfilingEvent(label, s_stack)
  {
  }

  static void startProfiling();
  static void stopProfiling();

private:

  static FTL::ProfilingStack s_stack;
};
