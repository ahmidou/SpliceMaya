//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <QDialog>
#include <DFG/DFGUICmdHandler_QUndo.h>
#include <ModelItems/BindingModelItem.h>
#include <ValueEditor/VEEditorOwner.h>
#include "FabricImportPatternCommand.h"

class FabricImportPatternDialog : public QDialog
{
  Q_OBJECT
  
public:
  FabricImportPatternDialog(QWidget * parent, FabricCore::DFGBinding binding, FabricImportPatternSettings * settings);
  virtual ~FabricImportPatternDialog();

  bool wasAccepted() const { return m_wasAccepted; }

public slots:
  void onAccepted();
  void onRejected() { m_wasAccepted = false; close(); }
  void onEnableMaterialsChanged(int state);
  void onScaleChanged(const QString & text);
  void onNameSpaceChanged(const QString & text);

private:

  FabricImportPatternSettings * m_settings;

  QUndoStack * m_stack;
  FabricCore::DFGBinding m_binding;
  FabricUI::DFG::DFGUICmdHandler * m_handler;
  FabricUI::ModelItems::BindingModelItem * m_bindingItem;
  FabricUI::ValueEditor::VEEditorOwner * m_owner;
  bool m_wasAccepted;
};
