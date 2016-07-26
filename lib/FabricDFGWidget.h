//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include "DFGUICmdHandler_Maya.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QSplitter>

#include <map>

#include <maya/MStatus.h>
#include <maya/MString.h>

#include <SceneHub/DFG/SHDFGCombinedWidget.h>
#include <DFG/Dialogs/DFGBaseDialog.h>

#include "FabricSpliceRenderCallback.h"

#include <FabricSplice.h>

using namespace FabricServices;
using namespace FabricUI;

class FabricDFGBaseInterface;

// SH-279
// We have to define a new typedef FabricDFGWidgetBaseClass outside of the class declaration
// so Q-Moc (signal/slots compilation) compiles over the right derived class.
// Using #ifdef #endif within the declaration as :
// #ifdef FABRIC_SCENEHUB
//  class FabricDFGWidget : public DFG::SHDFGCombinedWidget {
// #else
//  class FabricDFGWidget : public DFG::DFGCombinedWidget {
// #endif
// breaks the Moc-build in the way that it's not referring to the right parent class and all the signal/slots are broken.

#ifdef FABRIC_SCENEHUB
typedef DFG::SHDFGCombinedWidget FabricDFGWidgetBaseClass;
#else
typedef DFG::DFGCombinedWidget FabricDFGWidgetBaseClass; 
#endif
 
class FabricDFGWidget : public FabricDFGWidgetBaseClass {
 

  Q_OBJECT
  
public:
  FabricDFGWidget(QWidget * parent);
  virtual ~FabricDFGWidget();

  static QWidget * creator(QWidget * parent, const QString & name);
 
  static FabricDFGWidget *Instance();
  static void Destroy();

  static void SetCurrentUINodeName(const char * node);

  static FabricCore::Client &GetCoreClient()
  {
    if ( !s_coreClient )
      s_coreClient = FabricSplice::ConstructClient();
    return s_coreClient;
  }

  FabricCore::DFGHost &getDFGHost()
  {
    return m_dfgHost;
  }

public slots:
  virtual void onUndo();
  virtual void onRedo();
  virtual void onSelectCanvasNodeInDCC();
  virtual void onImportGraphInDCC();
  virtual void onExportGraphInDCC();

private slots:
  void onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog * dialog);
  void onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog * dialog, FTL::JSONObjectEnc<> * additionalMetaData);

protected:
  virtual void refreshScene();
  void setCurrentUINodeName(const char * node);

private:

  DFGUICmdHandler_Maya m_cmdHandler;
  FabricCore::DFGHost m_dfgHost;
  bool m_initialized;

  static FabricDFGWidget *s_widget;

  // [andrew 20150918] this must also be static as it may be required by the
  // BaseInterface without initializing a FabricDFGWidget in Maya batch mode
  static FabricCore::Client s_coreClient;
};
