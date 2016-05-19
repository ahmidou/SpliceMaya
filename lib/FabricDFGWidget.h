//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include "DFGUICmdHandler_Maya.h"
#include <QtGui/QWidget>
#include <QtGui/QSplitter>

#include <map>

#include <maya/MStatus.h>
#include <maya/MString.h>

#include <DFG/DFGCombinedWidget.h>
#include <DFG/Dialogs/DFGBaseDialog.h>

#include <FabricSplice.h>

using namespace FabricServices;
using namespace FabricUI;

class FabricDFGBaseInterface;

class FabricDFGWidget : public DFG::DFGCombinedWidget {

  Q_OBJECT
  
public:
  FabricDFGWidget(QWidget * parent);
  ~FabricDFGWidget();

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

private slots:
  void onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog * dialog);
  void onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog * dialog, FTL::JSONObjectEnc<> * additionalMetaData);

protected:
  virtual void initMenu();
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
