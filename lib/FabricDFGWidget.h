//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QWidget>
#include "Foundation.h"
#include <FabricSplice.h>
#include <maya/MString.h>
#include <maya/MDGMessage.h>
#include <maya/MDagMessage.h>
#include <maya/MModelMessage.h>
#include "DFGUICmdHandler_Maya.h"
#include <DFG/Dialogs/DFGBaseDialog.h>
#include <SceneHub/DFG/SHDFGCombinedWidget.h>

using namespace FabricUI;
using namespace FabricServices;

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
 
class FabricDFGWidget : public FabricDFGWidgetBaseClass 
{
  Q_OBJECT
  
public:

  FabricDFGWidget(
    QWidget * parent
    );
  
  virtual ~FabricDFGWidget();

  static QWidget * creator(
    QWidget * parent, 
    const QString & name
    );
 
  static FabricDFGWidget *Instance(
    bool createIfNull=true
    );

  static void Destroy();

  static void SetCurrentUINodeName(
    const char * node
    );

  static FabricCore::Client GetCoreClient();
  
  static FabricDFGBaseInterface *getBaseInterface();

  static void OnSelectCanvasNodeInDCC(
    void *client
    );

  FabricCore::DFGHost &getDFGHost()
  {
    return m_dfgHost;
  }

  virtual void keyPressEvent(
    QKeyEvent * event
    );

public slots:

  virtual void onUndo();
  
  virtual void onRedo();
  
  virtual void onSelectCanvasNodeInDCC();
  
  virtual void onImportGraphInDCC();
  
  virtual void onExportGraphInDCC();

private slots:

  void onPortEditDialogCreated(
    FabricUI::DFG::DFGBaseDialog * dialog
    );
  
  void onPortEditDialogInvoked(
    FabricUI::DFG::DFGBaseDialog * dialog, 
    FTL::JSONObjectEnc<> * additionalMetaData
    );

protected:

  virtual void refreshScene();

  void setCurrentUINodeName(
    const char * node
    );

private:

  MCallbackId m_onSelectionChangedCallbackId;

  DFGUICmdHandler_Maya m_cmdHandler;
  FabricCore::DFGHost m_dfgHost;
  bool m_initialized;

  static FabricDFGWidget *s_widget;
};
