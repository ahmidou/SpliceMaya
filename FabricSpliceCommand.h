
#ifndef _CREATIONSPLICECOMMAND_H_
#define _CREATIONSPLICECOMMAND_H_

#include "FabricSpliceEditorWidget.h"

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>

class FabricSpliceCommand: public MPxCommand{
public:
  static void* creator();
  static MSyntax newSyntax();

  MStatus doIt(const MArgList &args);

private:
};

#endif 
