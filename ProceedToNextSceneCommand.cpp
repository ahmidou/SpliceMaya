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

#if BOOST_VERSION == 105500
  while(samplesDir.stem().string() != "Samples" && samplesDir.stem().string() != "Splice") {
#else
  while(samplesDir.stem() != "Samples" && samplesDir.stem() != "Splice") {
#endif
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
#if BOOST_VERSION == 105500
      else if(dir_iter->path().extension().string() == ".ma" || 
        dir_iter->path().extension().string() == ".MA" ||
        dir_iter->path().extension().string() == ".mb" ||
        dir_iter->path().extension().string() == ".MB")
#else
      else if(dir_iter->path().extension() == ".ma" || 
        dir_iter->path().extension() == ".MA" ||
        dir_iter->path().extension() == ".mb" ||
        dir_iter->path().extension() == ".MB")
#endif
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

#if BOOST_VERSION == 105500
    std::string nextSampleStr = nextSample.make_preferred().string();
#else
    std::string nextSampleStr = nextSample.string();
#endif
    MGlobal::displayInfo("Opening next scene: "+MString(nextSampleStr.c_str()));
    std::string nextSampleStrEscaped;
    for(unsigned int i=0;i<nextSampleStr.length();i++)
    {
      if(nextSampleStr[i] == '\\')
        nextSampleStrEscaped += "/";
      else
        nextSampleStrEscaped += nextSampleStr[i];
    }
    MString command = "file -f -options \"v=0\"  -typ \"mayaAscii\" -o \""+MString(nextSampleStrEscaped.c_str())+"\";";
    MGlobal::executeCommandOnIdle(command);
  }

  return MStatus::kSuccess;
}

