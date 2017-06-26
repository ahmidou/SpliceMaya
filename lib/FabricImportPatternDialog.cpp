//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QDialogButtonBox>

#include "FabricImportPatternDialog.h"
#include "FabricImportPatternCommand.h"
#include "FabricSpliceHelpers.h"

FabricImportPatternDialog::FabricImportPatternDialog(QWidget * parent, FabricCore::Client client, FabricCore::DFGBinding binding, FabricImportPatternSettings settings)
: QDialog(parent)
, m_settings(settings)
, m_qSettings(NULL)
, m_client(client)
, m_binding(binding)
, m_wasAccepted(false)
{
  setWindowTitle("Fabric Import Pattern");

  if(m_settings.useLastArgValues)
    restoreSettings(client, m_settings.filePath, m_binding, &m_qSettings);

  m_stack = new QUndoStack();
  m_handler = new FabricUI::DFG::DFGUICmdHandler_QUndo(m_stack);
  m_bindingItem = new FabricUI::ModelItems::BindingModelItem(m_handler, binding, true, false, false);
  m_owner = new FabricUI::ValueEditor::VEEditorOwner();
  m_owner->initConnections();
  m_owner->emitReplaceModelRoot( m_bindingItem );

  setSizePolicy(QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  setMinimumWidth(500);
  setMaximumWidth(1200);
  setMinimumHeight(800);

  QWidget * widget = m_owner->getWidget();
  widget->setSizePolicy(QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  widget->setParent(this);

  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);
  layout->addWidget(widget);

  QWidget * optionsWidget = new QWidget(this);
  optionsWidget->setContentsMargins(0, 0, 0, 0);
  QGridLayout * optionsLayout = new QGridLayout();
  optionsWidget->setLayout(optionsLayout);
  layout->addWidget(optionsWidget);

  QLabel * attachToExistingLabel = new QLabel("Attach to existing", optionsWidget);
  optionsLayout->addWidget(attachToExistingLabel, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QCheckBox * attachToExistingCheckbox = new QCheckBox(optionsWidget);
  attachToExistingCheckbox->setCheckState(m_settings.attachToExisting ? Qt::Checked : Qt::Unchecked);
  optionsLayout->addWidget(attachToExistingCheckbox, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(attachToExistingCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onAttachToExistingChanged(int)));

  QLabel * enableMaterialsLabel = new QLabel("Enable Materials", optionsWidget);
  optionsLayout->addWidget(enableMaterialsLabel, 1, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QCheckBox * enableMaterialsCheckbox = new QCheckBox(optionsWidget);
  enableMaterialsCheckbox->setCheckState(m_settings.enableMaterials ? Qt::Checked : Qt::Unchecked);
  optionsLayout->addWidget(enableMaterialsCheckbox, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(enableMaterialsCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onEnableMaterialsChanged(int)));

  QLabel * scaleLabel = new QLabel("Scale", optionsWidget);
  optionsLayout->addWidget(scaleLabel, 2, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * scaleLineEdit = new QLineEdit(optionsWidget);
  scaleLineEdit->setValidator(new QDoubleValidator());
  scaleLineEdit->setText(QString::number(m_settings.scale));
  optionsLayout->addWidget(scaleLineEdit, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(scaleLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onScaleChanged(const QString &)));

  QLabel * nameSpaceLabel = new QLabel("NameSpace", optionsWidget);
  optionsLayout->addWidget(nameSpaceLabel, 3, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * nameSpaceLineEdit = new QLineEdit(optionsWidget);
  nameSpaceLineEdit->setText(m_settings.nameSpace.asChar());
  optionsLayout->addWidget(nameSpaceLineEdit, 3, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(nameSpaceLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onNameSpaceChanged(const QString &)));

  QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  layout->addWidget(buttons);

  QObject::connect(buttons, SIGNAL(accepted()), this, SLOT(onAccepted()));
  QObject::connect(buttons, SIGNAL(rejected()), this, SLOT(onRejected()));
}

FabricImportPatternDialog::~FabricImportPatternDialog()
{
  delete m_owner;
  delete m_bindingItem;
  delete m_handler;
  delete m_stack;
  if(m_qSettings)
    delete(m_qSettings);
}

void FabricImportPatternDialog::onAccepted()
{
  FabricCore::DFGBinding binding = m_binding;
  FabricCore::Client client = m_client;
  FabricImportPatternSettings settings = m_settings;
  storeSettings(client, settings.filePath, binding, &m_qSettings);
  close();
  FabricImportPatternCommand().invoke(client, binding, settings);
}

void FabricImportPatternDialog::onAttachToExistingChanged(int state)
{
  m_settings.attachToExisting = state == Qt::Checked;
}

void FabricImportPatternDialog::onEnableMaterialsChanged(int state)
{
  m_settings.enableMaterials = state == Qt::Checked;
}

void FabricImportPatternDialog::onScaleChanged(const QString & text)
{
  m_settings.scale = text.toDouble();
}

void FabricImportPatternDialog::onNameSpaceChanged(const QString & text)
{
  m_settings.nameSpace = text.toUtf8().constData();
}

void FabricImportPatternDialog::storeSettings(FabricCore::Client client, MString patternPath, FabricCore::DFGBinding binding, QSettings ** settings)
{
  if(settings == NULL)
    return;
  if((*settings) == NULL)
  {
    MString app = "Fabric Engine Maya - Asset Pattern";
    (*settings) = new QSettings(app.asChar(), patternPath.asChar());
  }

  try
  {
    FabricCore::DFGExec exec = binding.getExec();
    for(unsigned int i=0;i<exec.getExecPortCount();i++)
    {
      if(exec.getExecPortType(i) != FabricCore::DFGPortType_In)
        continue;

      FTL::CStrRef type = exec.getExecPortResolvedType(i);
      if(type != "String" && type != "FilePath" && !FabricCore::GetRegisteredTypeIsShallow(client.getContext(), type.c_str()))
        continue;

      FTL::CStrRef name = exec.getExecPortName(i);

      FabricCore::RTVal value = binding.getArgValue(i);
      if(type == "FilePath")
        value = value.callMethod("String", "string", 0, 0);
      QString jsonValue = value.getJSON().getStringCString();
      (*settings)->setValue(name.c_str(), jsonValue);
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
  }
}

void FabricImportPatternDialog::restoreSettings(FabricCore::Client client, MString patternPath, FabricCore::DFGBinding binding, QSettings ** settings)
{
  if(settings == NULL)
    return;
  if((*settings) == NULL)
  {
    MString app = "Fabric Engine Maya - Asset Pattern";
    (*settings) = new QSettings(app.asChar(), patternPath.asChar());
  }

  try
  {
    FabricCore::DFGExec exec = binding.getExec();
    for(unsigned int i=0;i<exec.getExecPortCount();i++)
    {
      if(exec.getExecPortType(i) != FabricCore::DFGPortType_In)
        continue;

      FTL::CStrRef type = exec.getExecPortResolvedType(i);
      if(type != "String" && type != "FilePath" && !FabricCore::GetRegisteredTypeIsShallow(client.getContext(), type.c_str()))
        continue;

      FTL::CStrRef name = exec.getExecPortName(i);

      QVariant jsonVariant = (*settings)->value(name.c_str());
      if(!jsonVariant.isValid())
        continue;

      MString jsonString = jsonVariant.toString().toUtf8().constData();

      FabricCore::RTVal value;
      if(type == "FilePath")
      {
        FabricCore::RTVal valueStr = FabricCore::ConstructRTValFromJSON(client, "String", jsonString.asChar());
        value = FabricCore::RTVal::Create(client, "FilePath", 1, &valueStr);
      }
      else
      {
        value = FabricCore::ConstructRTValFromJSON(client, type.c_str(), jsonString.asChar());
      }
      binding.setArgValue(i, value);
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
  }
}
