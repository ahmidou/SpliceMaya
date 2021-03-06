#
# Copyright 2010-2013 Fabric Engine Inc. All rights reserved.
#

import os, sys, platform, copy

Import('parentEnv', 'FABRIC_CAPI_DIR', 'FABRIC_SPLICE_VERSION', 'STAGE_DIR', 'FABRIC_BUILD_OS', 'FABRIC_BUILD_TYPE', 'MAYA_INCLUDE_DIR', 'MAYA_LIB_DIR','MAYA_VERSION', 'sharedCapiFlags', 'spliceFlags')

env = parentEnv.Clone()

mayaFlags = {
  'CPPPATH': [
      MAYA_INCLUDE_DIR,
      os.path.join(MAYA_INCLUDE_DIR, 'Qt')
    ],
  'LIBPATH': [
    MAYA_LIB_DIR
  ],
}

mayaFlags['LIBS'] = ['OpenMaya', 'OpenMayaAnim', 'OpenMayaUI', 'Foundation']
if FABRIC_BUILD_OS == 'Windows':
  mayaFlags['CCFLAGS'] = ['/DNT_PLUGIN']
  mayaFlags['LIBS'].extend(['QtCore4', 'QtGui4'])
elif FABRIC_BUILD_OS == 'Linux':
  mayaFlags['CCFLAGS'] = ['-DLINUX']
  mayaFlags['LIBS'].extend(['QtCore', 'QtGui'])
else:
  qtCoreLib = File(os.path.join(MAYA_LIB_DIR, 'QtCore'))
  qtGuiLib = File(os.path.join(MAYA_LIB_DIR, 'QtGui'))
  mayaFlags['LIBS'].extend([
    qtCoreLib,
    qtGuiLib,
    File(os.path.join(MAYA_LIB_DIR, 'QtGui'))
  ])

env.MergeFlags(mayaFlags)
env.Append(CPPDEFINES = ["_SPLICE_MAYA_VERSION="+str(MAYA_VERSION[:4])])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

target = 'FabricSpliceMaya' + MAYA_VERSION

mayaModule = None
sources = env.Glob('*.cpp')

if FABRIC_BUILD_OS == 'Darwin':
  # a loadable module will omit the 'lib' prefix name on Os X
  target += '.bundle'
  mayaModule = env.LoadableModule(target = target, source = sources)
else:
  libSuffix = '.so'
  if FABRIC_BUILD_OS == 'Windows':
    libSuffix = '.mll'
  mayaModule = env.SharedLibrary(target = target, source = sources, SHLIBSUFFIX=libSuffix, SHLIBPREFIX='')

sedCmd = 'sed'
if FABRIC_BUILD_OS == 'Windows':  
  envPaths = os.environ['PATH'].split(os.pathsep)
  for envPath in envPaths:
    sedPath = os.path.join(envPath, 'sed.exe')
    if os.path.exists(sedPath):
      sedCmd = sedPath

moduleFileMayaVersion = MAYA_VERSION
moduleFileMayaVersion = moduleFileMayaVersion[:moduleFileMayaVersion.find('201')+4]

env.Append(BUILDERS = {
  'SubstMayaModuleFile': Builder(action = [
    [
      sedCmd,
      '-e', 's/{{MAYA_VERSION}}/'+moduleFileMayaVersion+'/g ',
      '<$SOURCE', '>$TARGET',
    ]
  ])
})

mayaModuleFile = env.SubstMayaModuleFile(
  env.File('FabricSpliceMaya' + MAYA_VERSION + '.mod'),
  env.Dir('Module').File('FabricSpliceMaya.mod.template')
)[0]

mayaFiles = []
mayaFiles.append(env.Install(STAGE_DIR, mayaModuleFile))
mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), os.path.join('Module', 'plug-ins', 'FabricSpliceManipulation.py')))
mayaFiles.append(env.Install(STAGE_DIR, env.File('license.txt')))

for script in ['FabricSpliceMenu', 'FabricSpliceUI', 'FabricSpliceTool', 'FabricSpliceToolValues', 'FabricSpliceToolProperties']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'scripts'), os.path.join('Module', 'scripts', script+'.mel')))
for script in ['AEspliceMayaNodeTemplate']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'scripts'), os.path.join('Module', 'scripts', script+'.py')))
for ui in ['FabricSpliceEditor']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'ui'), os.path.join('Module', 'ui', ui+'.ui')))
for png in ['FE_logo']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'ui'), os.path.join('Module', 'ui', png+'.png')))
installedModule = env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), mayaModule)
mayaFiles.append(installedModule)

# also install the FabricCore dynamic library
mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.so'))))
mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.dylib'))))
mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.dll'))))

# install PDB files on windows
if FABRIC_BUILD_TYPE == 'Debug' and FABRIC_BUILD_OS == 'Windows':
  env['CCPDBFLAGS']  = ['${(PDB and "/Fd%s_incremental.pdb /Zi" % File(PDB)) or ""}']
  pdbSource = mayaModule[0].get_abspath().rpartition('.')[0]+".pdb"
  pdbTarget = os.path.join(STAGE_DIR.abspath, os.path.split(pdbSource)[1])
  copyPdb = env.Command( 'copy', None, 'copy "%s" "%s" /Y' % (pdbSource, pdbTarget) )
  env.Depends( copyPdb, installedModule )
  env.AlwaysBuild(copyPdb)

# todo: install the python client

alias = env.Alias('splicemaya', mayaFiles)
spliceData = (alias, mayaFiles)
Return('spliceData')
