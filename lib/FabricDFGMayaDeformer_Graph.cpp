//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "Foundation.h"
#include "FabricDFGMayaDeformer_Graph.h"

MTypeId FabricDFGMayaDeformer_Graph::id(0x0011AE48);

FabricDFGMayaDeformer_Graph::FabricDFGMayaDeformer_Graph()
  : FabricDFGMayaDeformer( &CreateDFGBinding )
{
}

void* FabricDFGMayaDeformer_Graph::creator()
{
  return new FabricDFGMayaDeformer_Graph();
}

FabricCore::DFGBinding FabricDFGMayaDeformer_Graph::CreateDFGBinding(
  FabricCore::DFGHost &dfgHost)
{
  return dfgHost.createBindingToNewGraph();
}
