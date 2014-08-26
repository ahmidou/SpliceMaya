
#ifndef _CREATIONSPLICEEDITORCMD_H_
#define _CREATIONSPLICEEDITORCMD_H_

#include <map>
#include <ostream>

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>

class FabricSpliceEditorCmd: public MPxCommand{
public:
  static void* creator();
  static MSyntax newSyntax();
  MStatus doIt(const MArgList &args);

private:
};

#endif 
