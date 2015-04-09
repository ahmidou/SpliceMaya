
#ifndef _FABRICDFGCOMMANDS_H_
#define _FABRICDFGCOMMANDS_H_

// #include "FabricSpliceEditorWidget.h"

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include "FabricDFGCommandStack.h"
#include "FabricDFGBaseInterface.h"
#include <DFG/DFGController.h>

class FabricDFGBaseCommand: public MPxCommand
{
public:

  ~FabricDFGBaseCommand();

  virtual const char * getName() = 0;
  virtual MStatus doIt(const MArgList &args);
  virtual MStatus undoIt();
  virtual MStatus redoIt();
  virtual bool isUndoable() const;

protected:

  FabricDFGBaseInterface * getInterf();

  MString m_node;
  FabricDFGCommandStack::Info m_cmdInfo;
};

class FabricDFGAddNodeCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddNode"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGRemoveNodeCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRemoveNode"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGRenameNodeCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRenameNode"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGAddEmptyFuncCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddEmptyFunc"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGAddEmptyGraphCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddEmptyGraph"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGAddConnectionCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddConnection"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGRemoveConnectionCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRemoveConnection"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGRemoveAllConnectionsCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRemoveAllConnections"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGAddPortCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddPort"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGRemovePortCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRemovePort"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGRenamePortCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRenamePort"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetArgCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetArg"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetDefaultValueCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetDefaultValue"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetCodeCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetCode"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGGetDescCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgGetDesc"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGImportJSONCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgImportJSON"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGExportJSONCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgExportJSON"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetNodeCacheRuleCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetNodeCacheRule"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGCopyCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgCopy"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGPasteCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgPaste"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

#endif 
