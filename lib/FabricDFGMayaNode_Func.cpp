//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "Foundation.h"
#include "FabricDFGMayaNode_Func.h"

MTypeId FabricDFGMayaNode_Func::id(0x0011AE4A);

FabricDFGMayaNode_Func::FabricDFGMayaNode_Func()
  : FabricDFGMayaNode( &CreateDFGBinding )
{
}

void* FabricDFGMayaNode_Func::creator()
{
  return new FabricDFGMayaNode_Func();
}

FabricCore::DFGBinding FabricDFGMayaNode_Func::CreateDFGBinding(
  FabricCore::DFGHost &dfgHost)
{
  FabricCore::DFGBinding dfgBinding = dfgHost.createBindingToNewFunc();
  FabricCore::DFGExec dfgExec = dfgBinding.getExec();
  dfgExec.setCode("\
dfgEntry {\n\
  // result = a + b;\n\
}\n\
");
  return dfgBinding;
}
