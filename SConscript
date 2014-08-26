#
# Copyright 2010-2013 Fabric Engine Inc. All rights reserved.
#

import os, sys, platform, copy

Import('parentEnv', 'FABRIC_CAPI_DIR', 'SPLICE_VERSION', 'STAGE_DIR', 'SPLICE_OS', 'MAYA_DIR', 'MAYA_VERSION', 'sharedCapiFlags', 'spliceFlags')

env = parentEnv.Clone()

print env['CPPPATH']

mayaFlags = {
  'CPPPATH': [
      os.path.join(MAYA_DIR, 'include'),
      os.path.join(MAYA_DIR, 'include', 'Qt')
    ],
  'LIBPATH': [
    os.path.join(MAYA_DIR, 'lib')
  ],
}

mayaFlags['LIBS'] = ['OpenMaya', 'OpenMayaAnim', 'OpenMayaUI', 'Foundation']
if SPLICE_OS == 'Windows':
  mayaFlags['CCFLAGS'] = ['/DNT_PLUGIN']
  mayaFlags['LIBS'].extend(['QtCore4', 'QtGui4'])
elif SPLICE_OS == 'Linux':
  mayaFlags['CCFLAGS'] = ['-DLINUX']
  mayaFlags['LIBS'].extend(['QtCore', 'QtGui'])
else:
  qtCoreLib = File(os.path.join(MAYA_DIR, 'lib', 'QtCore'))
  qtGuiLib = File(os.path.join(MAYA_DIR, 'lib', 'QtGui'))
  mayaFlags['LIBS'].extend([
    qtCoreLib,
    qtGuiLib,
    File(os.path.join(MAYA_DIR, 'lib', 'QtGui'))
  ])

env.MergeFlags(mayaFlags)
env.Append(CPPDEFINES = ["_SPLICE_MAYA_VERSION="+str(MAYA_VERSION[:4])])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)

target = 'FabricSpliceMaya' + MAYA_VERSION

mayaModule = None
if SPLICE_OS == 'Darwin':
  # a loadable module will omit the 'lib' prefix name on Os X
  target += '.bundle'
  mayaModule = env.LoadableModule(target = target, source = Glob('*.cpp'))
else:
  libSuffix = '.so'
  if SPLICE_OS == 'Windows':
    libSuffix = '.mll'
  mayaModule = env.SharedLibrary(target = target, source = Glob('*.cpp'), SHLIBSUFFIX=libSuffix, SHLIBPREFIX='')

sedCmd = 'sed'
if SPLICE_OS == 'Windows':  
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

# also install the FabricCore dynamic library
mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.so'))))
mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.dylib'))))
mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), env.Glob(os.path.join(FABRIC_CAPI_DIR, 'lib', '*.dll'))))

# mayaPythonVersion = '2.7'
# if int(MAYA_VERSION[0:4]) < 2014:
#   mayaPythonVersion = '2.6'

# pythonLib = 'python'+mayaPythonVersion
# if SPLICE_OS == 'Windows':
#   pythonLib = 'python'+mayaPythonVersion.replace('.', '')

# # [andrew 20140323] use Core lib from plug-ins folder
# linkFlags = []
# if SPLICE_OS == 'Linux':
#   linkFlags.append(Literal('-Wl,-rpath,$ORIGIN/../../../plug-ins'))

# mayaFiles.append(installPythonClient(
#   os.path.join(STAGE_DIR.abspath, 'python'), env, installedModule, 
#   {
#     'CPPPATH': [os.path.join(mayaFlags['CPPPATH'][0], 'python'+mayaPythonVersion)],
#     'LIBPATH': [mayaFlags['LIBPATH'][0]],
#     'LIBS': [pythonLib],
#     'LINKFLAGS': linkFlags
#   }))
# mayaFiles.append(installedModule)

if GetOption('debug_build') and SPLICE_OS == 'Windows':
  env['CCPDBFLAGS']  = ['${(PDB and "/Fd%s_incremental.pdb /Zi" % File(PDB)) or ""}']
  pdbSource = mayaModule[0].get_abspath().rpartition('.')[0]+".pdb"
  pdbTarget = os.path.join(STAGE_DIR.abspath, os.path.split(pdbSource)[1])
  copyPdb = env.Command( 'copy', None, 'copy "%s" "%s" /Y' % (pdbSource, pdbTarget) )
  env.Depends( copyPdb, installedModule )
  env.AlwaysBuild(copyPdb)

# # install extensions
# mayaFiles.extend(installExtensions(os.path.join(STAGE_DIR.abspath, 'Exts'), env, installedModule))

alias = env.Alias('maya', mayaFiles)
spliceData = (alias, mayaFiles)
Return('spliceData')
