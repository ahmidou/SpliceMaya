//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <QDialog>
#include <QSettings>
#include <DFG/DFGUICmdHandler_QUndo.h>
#include <ModelItems/BindingModelItem.h>
#include <ValueEditor/VEEditorOwner.h>
#include "FabricImportPatternCommand.h"

class FabricImportPatternDialog : public QDialog
{
  Q_OBJECT

  friend class FabricExportPatternDialog;
  
public:
  FabricImportPatternDialog(QWidget * parent, FabricCore::Client client, FabricCore::DFGBinding binding, FabricImportPatternSettings settings);
  virtual ~FabricImportPatternDialog();

  bool wasAccepted() const { return m_wasAccepted; }

public slots:
  void onAccepted();
  void onRejected() { m_wasAccepted = false; close(); }
  void onUserAttributesChanged(int state);
  void onAttachToExistingChanged(int state);
  void onEnableMaterialsChanged(int state);
  void onScaleChanged(const QString & text);
  void onNameSpaceChanged(const QString & text);

protected:
  static void storeSettings(FabricCore::Client client, MString patternPath, FabricCore::DFGBinding binding);
  static void restoreSettings(FabricCore::Client client, MString patternPath, FabricCore::DFGBinding binding);

private:

  FabricImportPatternSettings m_settings;

  QUndoStack * m_stack;
  FabricCore::Client m_client;
  FabricCore::DFGBinding m_binding;
  FabricUI::DFG::DFGUICmdHandler * m_handler;
  FabricUI::ModelItems::BindingModelItem * m_bindingItem;
  FabricUI::ValueEditor::VEEditorOwner * m_owner;
  bool m_wasAccepted;
};
