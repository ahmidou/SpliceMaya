//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <QDialog>
#include <DFG/DFGUICmdHandler_QUndo.h>
#include <ModelItems/BindingModelItem.h>
#include <ValueEditor/VEEditorOwner.h>
#include "FabricExportPatternCommand.h"

class FabricExportPatternDialog : public QDialog
{
  Q_OBJECT
  
public:
  FabricExportPatternDialog(QWidget * parent, FabricCore::Client client, FabricCore::DFGBinding binding, const FabricExportPatternSettings & settings);
  virtual ~FabricExportPatternDialog();

  bool wasAccepted() const { return m_wasAccepted; }

public slots:
  void onAccepted();
  void onRejected() { m_wasAccepted = false; close(); }
  void onScaleChanged(const QString & text);
  void onStartTimeChanged(const QString & text);
  void onEndTimeChanged(const QString & text);
  void onFPSChanged(const QString & text);
  void onSubStepsChanged(const QString & text);

private:

  FabricExportPatternSettings m_settings;

  QUndoStack * m_stack;
  FabricCore::Client m_client;
  FabricCore::DFGBinding m_binding;
  FabricUI::DFG::DFGUICmdHandler * m_handler;
  FabricUI::ModelItems::BindingModelItem * m_bindingItem;
  FabricUI::ValueEditor::VEEditorOwner * m_owner;
  bool m_wasAccepted;
};
