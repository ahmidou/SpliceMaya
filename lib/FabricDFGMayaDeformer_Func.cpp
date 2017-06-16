//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricDFGMayaDeformer_Func.h"

MTypeId FabricDFGMayaDeformer_Func::id(0x0011AE4B);

FabricDFGMayaDeformer_Func::FabricDFGMayaDeformer_Func()
  : FabricDFGMayaDeformer( &CreateDFGBinding )
{
}

void* FabricDFGMayaDeformer_Func::creator()
{
  return new FabricDFGMayaDeformer_Func();
}

FabricCore::DFGBinding FabricDFGMayaDeformer_Func::CreateDFGBinding(
  FabricCore::DFGHost &dfgHost)
{
  FabricCore::DFGBinding dfgBinding = dfgHost.createBindingToNewFunc();
  FabricCore::DFGExec dfgExec = dfgBinding.getExec();
  dfgExec.addExtDep( "Geometry" );
  dfgExec.setCode("\
dfgEntry {\n\
  // result = a + b;\n\
}\n\
");
  dfgExec.addExecPort(
    "meshes",
    FEC_DFGPortType_IO,
    "PolygonMesh[]"
    );
  return dfgBinding;
}
