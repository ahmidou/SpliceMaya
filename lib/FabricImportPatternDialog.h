//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <QDialog>
#include <QSettings>
#include <QCheckBox>
#include <DFG/DFGUICmdHandler_QUndo.h>
#include <ModelItems/BindingModelItem.h>
#include <ValueEditor/VEEditorOwner.h>
#include <ValueEditor/VETreeWidget.h>
#include "FabricImportPatternCommand.h"

class FabricImportPatternDialog : public QDialog
{
  Q_OBJECT

  friend class FabricExportPatternDialog;
  
public:
  FabricImportPatternDialog(QWidget * parent, FabricCore::Client client, FabricCore::DFGBinding binding, FabricImportPatternSettings settings);
  virtual ~FabricImportPatternDialog();

  bool wasAccepted() const { return m_wasAccepted; }
  static bool isPreviewRenderingEnabled() { return s_previewRenderingEnabled; }

public slots:
  void onAccepted();
  void onRejected();
  void onUserAttributesChanged(int state);
  void onAttachToExistingChanged(int state);
  void onAttachToSceneTimeChanged(int state);
  void onEnableMaterialsChanged(int state);
  void onScaleChanged(const QString & text);
  void onNameSpaceChanged(const QString & text);
  void onStripNameSpacesChanged(int state);

protected:
  virtual void closeEvent(QCloseEvent * event);
  static void storeSettings(FabricCore::Client client, MString patternPath, FabricCore::DFGBinding binding);
  static void restoreSettings(FabricCore::Client client, MString patternPath, FabricCore::DFGBinding binding);

private:

  static void BindingNotificationCallback(void * userData, char const *jsonCString, uint32_t jsonLength);
  void updatePreviewRendering(bool enableIfDisabled);
  void disablePreviewRendering();

  FabricImportPatternSettings m_settings;

  QUndoStack * m_stack;
  FabricCore::Client m_client;
  FabricCore::DFGBinding m_binding;
  FabricUI::DFG::DFGUICmdHandler * m_handler;
  FabricUI::ModelItems::BindingModelItem * m_bindingItem;
  FabricUI::ValueEditor::VEEditorOwner * m_owner;
  bool m_wasAccepted;
  static bool s_previewRenderingEnabled;

  QCheckBox * m_stripNameSpacesCheckbox;
};
