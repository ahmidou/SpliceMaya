
#ifndef _CREATIONSPLICEEDITORWIDGET_H_
#define _CREATIONSPLICEEDITORWIDGET_H_

#include <QtGui/QTabWidget>
#include <QtGui/QTextEdit>
#include <QtCore/QEvent>

#include <map>

#include "FabricSpliceComboWidget.h"
#include "FabricSpliceButtonWidget.h"
#include "FabricSpliceListWidget.h"
#include "FabricSpliceKLSourceCodeWidget.h"
#include "FabricSpliceBaseInterface.h"

#include <maya/MStatus.h>

class FabricSpliceEditorWidget : public QTabWidget {
public:
  FabricSpliceEditorWidget(QWidget * parent, const QString & name);
  ~FabricSpliceEditorWidget();

  void update(MStatus * status = NULL);
  void updateNodeDropDown(MStatus * status = NULL);
  void updateOperatorDropDown(MStatus * status = NULL);
  void setOperatorName(const std::string & opName, MStatus * status = NULL);
  void setNodeDropDown(std::string nodeName, MStatus * status = NULL);
  void reportCompilerError(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc, MStatus * status = NULL);

  static QWidget * creator(QWidget * parent, const QString & name);
  static FabricSpliceEditorWidget * getFirst();
  static FabricSpliceEditorWidget * getFirstOrOpen(bool onIdle = true);

  static void postUpdateAll(MStatus * status = NULL);
  static void updateAll(MStatus * status = NULL);
  static void updateAllNodeDropDowns(MStatus * status = NULL);
  static void updateAllOperatorDropDowns(MStatus * status = NULL);
  static void setAllNodeDropDowns(std::string nodeName, bool onIdle = true, MStatus * status = NULL);
  static void setAllOperatorNames(std::string opName, bool onIdle = true, MStatus * status = NULL);
  static void reportAllCompilerError(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc, MStatus * status = NULL);

  // callbacks  
  static void onNodeChanged(void * userData);
  static void onOperatorChanged(void * userData);
  static void compilePressed(void * userData);
  static void addAttrPressed(void * userData);
  static void removeAttrPressed(void * userData);
  static void addOpPressed(void * userData);
  static void editOpPressed(void * userData);
  static void removeOpPressed(void * userData);

  static const char * getSourceCodeForOperator(const char * graphName, const char * opName);

private:
  typedef std::map<std::string, FabricSpliceEditorWidget*> widgetMap;
  typedef widgetMap::iterator widgetIt;
  typedef std::pair<std::string, FabricSpliceEditorWidget*> widgetPair;

  std::string mName;
  std::string mOpName;
  std::string mLastSourceCode;
  static widgetMap gWidgets;
  static bool gUpdatePosted;

  FabricSpliceComboWidget * mNodeCombo;
  FabricSpliceListWidget * mAttrList;
  FabricSpliceListWidget * mOperatorList;
  FabricSpliceKLLineNumberWidget * mLineNumbers;
  FabricSpliceKLSourceCodeWidget * mSourceCode;

  FabricSpliceButtonWidget * mAddAttrButton;
  FabricSpliceButtonWidget * mRemoveAttrButton;
  FabricSpliceButtonWidget * mAddOpButton;
  FabricSpliceButtonWidget * mEditOpButton;
  FabricSpliceButtonWidget * mRemoveOpButton;
  FabricSpliceButtonWidget * mCompileButton;

  QTextEdit * mErrorLog;
  QTextEdit * mPortText;

  std::string getCurrentNodeName();
  FabricSpliceBaseInterface * getCurrentBaseInterface();

protected:
  virtual bool event ( QEvent * event );
  virtual QSize sizeHint () const;
  virtual void bringToFront();
};

#endif 
