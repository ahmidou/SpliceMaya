//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricDFGMayaNode_Graph.h"

MTypeId FabricDFGMayaNode_Graph::id(0x0011AE47);

FabricDFGMayaNode_Graph::FabricDFGMayaNode_Graph()
  : FabricDFGMayaNode( &CreateDFGBinding )
{
}

void* FabricDFGMayaNode_Graph::creator(){
  return new FabricDFGMayaNode_Graph();
}

FabricCore::DFGBinding FabricDFGMayaNode_Graph::CreateDFGBinding(
  FabricCore::DFGHost &dfgHost
  )
{
  return dfgHost.createBindingToNewGraph();
}
