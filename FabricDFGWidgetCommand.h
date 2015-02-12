
#ifndef _FABRICDFGWIDGETCOMMAND_H_
#define _FABRICDFGWIDGETCOMMAND_H_

// #include "FabricSpliceEditorWidget.h"

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>

class FabricDFGWidgetCommand: public MPxCommand{
public:
  static void* creator();
  static MSyntax newSyntax();

  MStatus doIt(const MArgList &args);
  
private:
};

#endif 
