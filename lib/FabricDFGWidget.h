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
#include <SceneHub/DFG/SHDFGCombinedWidget.h>
#include <DFG/Dialogs/DFGBaseDialog.h>
#include <FabricSplice.h>

using namespace FabricServices;
using namespace FabricUI;

class FabricDFGBaseInterface;
class FabricDFGWidget : public DFG::SHDFGCombinedWidget {

  Q_OBJECT
  
  public:
    FabricDFGWidget(QWidget * parent);
    ~FabricDFGWidget();
    FabricCore::DFGHost &getDFGHost() { return m_dfgHost; }

    static QWidget * creator(QWidget *parent, const QString &name);
    static FabricDFGWidget *Instance();
    static void Destroy();
    static void SetCurrentUINodeName(const char * node);
    static FabricCore::Client &GetCoreClient() {
      if(!s_coreClient) s_coreClient = FabricSplice::ConstructClient();
      return s_coreClient;
    }

  public slots:
    virtual void onUndo();
    virtual void onRedo();
    
  private slots:
    void onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog * dialog);
    void onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog * dialog, FTL::JSONObjectEnc<> * additionalMetaData);

  protected:
    void setCurrentUINodeName(const char * node);

  private:
    virtual void refresh();  

    DFGUICmdHandler_Maya m_cmdHandler;
    std::string m_currentUINodeName;
    FabricCore::DFGHost m_dfgHost;
    bool m_initialized;
    static FabricDFGWidget *s_widget;
    // [andrew 20150918] this must also be static as it may be required by the
    // BaseInterface without initializing a FabricDFGWidget in Maya batch mode
    static FabricCore::Client s_coreClient;
};
