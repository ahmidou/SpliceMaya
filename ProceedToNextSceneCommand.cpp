#include "ProceedToNextSceneCommand.h"

#include <maya/MStringArray.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>

#include <boost/filesystem.hpp>

MSyntax ProceedToNextSceneCommand::newSyntax()
{
  MSyntax syntax;
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* ProceedToNextSceneCommand::creator()
{
  return new ProceedToNextSceneCommand;
}

MStatus ProceedToNextSceneCommand::doIt(const MArgList &args)
{
  MString sceneFileName;
  MGlobal::executeCommand("file -q -sceneName", sceneFileName);

  boost::filesystem::path currentSample = sceneFileName.asChar();
  boost::filesystem::path samplesDir = currentSample.parent_path();

  while(samplesDir.stem().string() != "Samples" && samplesDir.stem().string() != "Splice") {
    samplesDir = samplesDir.parent_path();
    if(samplesDir.empty()) {
      MGlobal::displayError("You can only use proceedToNextScene on the Fabric Engine sample scenes.");
      return MStatus::kFailure;
    }
  }

  std::vector<boost::filesystem::path> sampleScenes;
  std::vector<boost::filesystem::path> folders;
  folders.push_back(samplesDir);
  for(size_t i=0;i<folders.size();i++)
  {
    if(!boost::filesystem::exists(folders[i]))
      continue;
    if(!boost::filesystem::is_directory(folders[i]))
      continue;
    
    boost::filesystem::directory_iterator end_iter;
    for( boost::filesystem::directory_iterator dir_iter(folders[i]) ; dir_iter != end_iter ; ++dir_iter)
    {
      if(boost::filesystem::is_directory(dir_iter->path()))
      {
        folders.push_back(dir_iter->path());
      }
      else if(dir_iter->path().extension().string() == ".ma" || 
        dir_iter->path().extension().string() == ".MA" ||
        dir_iter->path().extension().string() == ".mb" ||
        dir_iter->path().extension().string() == ".MB")
      {
        sampleScenes.push_back(dir_iter->path());
      }
    }
  }

  boost::filesystem::path nextSample;
  for(size_t i=0;i<sampleScenes.size();i++) {

    if(sampleScenes[i] == currentSample)
    {
      if(i < sampleScenes.size() - 1)
        nextSample = sampleScenes[i+1];
      else
        nextSample = sampleScenes[0];
      break;
    }
  }

  if(!nextSample.empty()) {

    MGlobal::displayInfo("Opening next scene: "+MString(nextSample.string().c_str()));
    std::string nextSampleStr = nextSample.string();
    for(unsigned int i=0;i<nextSampleStr.length();i++)
    {
      if(nextSampleStr[i] == '\\')
        nextSampleStr[i] = '/';
    }
    MGlobal::executeCommandOnIdle("file -f -o \""+MString(nextSampleStr.c_str())+"\"");
  }

  return MStatus::kSuccess;
}

