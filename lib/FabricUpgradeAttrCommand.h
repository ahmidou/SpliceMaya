
#ifndef _FABRICUPGRADEATTRCOMMAND_H_
#define _FABRICUPGRADEATTRCOMMAND_H_

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>

class FabricUpgradeAttrCommand: public MPxCommand{
public:
  static void* creator();
  static MSyntax newSyntax();

  MStatus doIt(const MArgList &args);

private:
};

#endif 
