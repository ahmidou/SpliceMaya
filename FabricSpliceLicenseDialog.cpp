#include <QtGui/QApplication>
#include <QtGui/QTextBrowser>
#include <QtGui/QLabel>
#include <QtGui/QMessageBox>

#include "FabricSpliceLicenseDialog.h"
#include "FabricSpliceButtonWidget.h"
#include "FabricSpliceLogoWidget.h"
#include "plugin.h"
#include <FabricSplice.h>

#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>


FabricSpliceLicenseDialog::FabricSpliceLicenseDialog(const char * message, QWidget * parent)
: QDialog(parent)
{
  setLayout(new QVBoxLayout());
  setModal(true);

  setWindowTitle("Fabric:Splice Licensing");
  setMinimumWidth(400);
  setMaximumWidth(400);

  // setup the logo
  MString moduleFolder = getModuleFolder();
  MString logoPath = moduleFolder + "/ui/FE_logo.png";
  FabricSpliceLogoWidget * logo = new FabricSpliceLogoWidget(logoPath.asChar(), this);
  layout()->addWidget(logo);

  QTextBrowser * info = new QTextBrowser(this);
  info->setReadOnly(true);
  info->setFrameStyle(QFrame::NoFrame);
  info->setOpenExternalLinks(true);
  info->setMinimumHeight(20);
  layout()->addWidget(info);

  // extract the host id from json
  FabricCore::Variant json = FabricCore::Variant::CreateFromJSON(message);
  const FabricCore::Variant * hostIds = json.getDictValue("hostIds");
  QString macAddress;
  if(hostIds)
  {
    const FabricCore::Variant * hostId = hostIds->getArrayElement(0);
    macAddress = hostId->getStringData();
  }

  QTextCursor infoCursor = info->textCursor();
  infoCursor.insertText("Your copy of Fabric:Splice is unlicensed. Unlicensed copies will show this dialog and pause the execution for 15 seconds upon startup.\n\n");
  infoCursor.insertText("Please enter the license text OR the server below.\nThe server format needs to be rlm://host:port,\nso for example rlm://127.0.0.1:5053.\n\n");
  infoCursor.insertText("You may also click the link below to request a new license.\n");
  if(macAddress.length() > 0)
    infoCursor.insertText("Use this mac adress when requesting a license:\n"+macAddress+"\n");
  infoCursor.insertText("\n");

  QPalette palette(qApp->palette());
  QTextCharFormat format;
  format.setAnchor(true);
  format.setForeground(palette.color(QPalette::Highlight));
  format.setAnchorHref("http://fabricengine.com/creation/rlm-license/");
  infoCursor.insertText("http://fabricengine.com/creation/rlm-license/", format);

  layout()->addWidget(new QLabel("License Text", this));
  mLicenseText = new QTextEdit(this);
  mLicenseText->setMaximumHeight(80);
  layout()->addWidget(mLicenseText);

  layout()->addWidget(new QLabel("License Server", this));
  mLicenseServer = new QLineEdit(this);
  layout()->addWidget(mLicenseServer);

  QWidget * buttons = new QWidget(this);
  layout()->addWidget(buttons);

  buttons->setLayout(new QHBoxLayout());
  buttons->layout()->setContentsMargins(0, 0, 0, 0);

  FabricSpliceButtonWidget * cancelButton = new FabricSpliceButtonWidget("Cancel", buttons);
  buttons->layout()->addWidget(cancelButton);
  FabricSpliceButtonWidget * okButton = new FabricSpliceButtonWidget("OK", buttons);
  buttons->layout()->addWidget(okButton);

  // callbacks
  okButton->setCallback(onOkPressed, this);
  cancelButton->setCallback(onCancelPressed, this);
  okButton->setDefault(true);

  qApp->setOverrideCursor(Qt::ArrowCursor);
}

FabricSpliceLicenseDialog::~FabricSpliceLicenseDialog()
{
  qApp->restoreOverrideCursor();
}

void FabricSpliceLicenseDialog::onOkPressed(void * userData)
{
  FabricSpliceLicenseDialog * dialog = (FabricSpliceLicenseDialog *)userData;

  std::string licenseText = getStdStringFromQString(dialog->mLicenseText->toPlainText());
  std::string licenseServer = getStdStringFromQString(dialog->mLicenseServer->text());

  if(licenseText.length() > 0 || licenseServer.length() > 0)
  {
    if(licenseText.length() > 0)
      FabricSplice::setStandaloneLicense(licenseText.c_str());
    else
      FabricSplice::setLicenseServer(licenseServer.c_str());

    if(FabricSplice::isLicenseValid())
    {
      QMessageBox msgBox;
      msgBox.setWindowTitle("Fabric:Splice");
      msgBox.setIcon(QMessageBox::Information);
      msgBox.setText("Your license has been validated successfully.");
      msgBox.exec();      
    }
    else
    {
      QMessageBox msgBox;
      msgBox.setWindowTitle("Fabric:Splice");
      msgBox.setIcon(QMessageBox::Warning);
      msgBox.setText("Invalid license.");
      msgBox.exec();      
    }
  }

  dialog->close();
}

void FabricSpliceLicenseDialog::onCancelPressed(void * userData)
{
  FabricSpliceLicenseDialog * dialog = (FabricSpliceLicenseDialog *)userData;
  dialog->close();
}

std::string FabricSpliceLicenseDialog::getStdStringFromQString(QString input)
{
  #ifdef _WIN32
    return input.toLocal8Bit().constData();
  #else
    return input.toUtf8().constData();
  #endif
}
