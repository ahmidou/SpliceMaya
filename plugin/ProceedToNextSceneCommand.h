
#ifndef _PROCEEDTONEXTSCENECOMMAND_H_
#define _PROCEEDTONEXTSCENECOMMAND_H_

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>

class ProceedToNextSceneCommand: public MPxCommand{
public:
  static void* creator();
  static MSyntax newSyntax();

  MStatus doIt(const MArgList &args);

private:
};

#endif 
