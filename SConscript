#
# Copyright 2010-2013 Fabric Engine Inc. All rights reserved.
#

import os, sys, platform, copy

Import(
  'parentEnv',
  'FABRIC_DIR',
  'FABRIC_SPLICE_VERSION',
  'STAGE_DIR',
  'FABRIC_BUILD_OS',
  'FABRIC_BUILD_TYPE',
  'MAYA_BIN_DIR',
  'MAYA_INCLUDE_DIR',
  'MAYA_LIB_DIR',
  'MAYA_VERSION',
  'sharedCapiFlags',
  'spliceFlags',
  'ADDITIONAL_FLAGS',
  'commandsFlags',
  'dfgWrapperFlags',
  'astWrapperFlags',
  'legacyBoostFlags',
  'codeCompletionFlags'
  )

env = parentEnv.Clone()

qtMOCBuilder = Builder(
  action = [[os.path.join(MAYA_BIN_DIR, 'moc'), '-o', '$TARGET', '$SOURCE']],
  prefix = 'moc_',
  suffix = '.cc',
  src_suffix = '.h',
)
env.Append(BUILDERS = {'QTMOC': qtMOCBuilder})

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
  mayaFlags['CPPDEFINES'] = ['NT_PLUGIN']
  mayaFlags['LIBS'].extend(['QtCore4', 'QtGui4'])
if FABRIC_BUILD_OS == 'Linux':
  mayaFlags['CPPDEFINES'] = ['LINUX']
  mayaFlags['LIBS'].extend(['QtCore', 'QtGui'])
if FABRIC_BUILD_OS == 'Darwin':
  mayaFlags['CPPDEFINES'] = ['OSMac_']
  qtCoreLib = File(os.path.join(MAYA_LIB_DIR, 'QtCore'))
  qtGuiLib = File(os.path.join(MAYA_LIB_DIR, 'QtGui'))
  mayaFlags['LIBS'].extend([
    qtCoreLib,
    qtGuiLib,
    File(os.path.join(MAYA_LIB_DIR, 'QtGui'))
    ])

env.MergeFlags(mayaFlags)

# services flags
env.MergeFlags(commandsFlags)
env.MergeFlags(dfgWrapperFlags)
env.MergeFlags(astWrapperFlags)
env.MergeFlags(legacyBoostFlags)
env.MergeFlags(codeCompletionFlags)

# build the ui libraries for splice
uiLibPrefix = 'uiMaya'+str(MAYA_VERSION)
uiLibs = SConscript('#/Native/FabricUI/SConscript', exports = {
  'parentEnv': env, 
  'uiLibPrefix': uiLibPrefix, 
  'qtDir': os.path.join(MAYA_INCLUDE_DIR, 'Qt'),
  'qtMOC': os.path.join(MAYA_BIN_DIR, 'moc'),
  'fabricFlags': sharedCapiFlags
  }, 
  variant_dir = env.Dir('FabricUI')
  )

# import the maya specific libraries
Import(uiLibPrefix+'Flags')

# ui flags
env.MergeFlags(locals()[uiLibPrefix + 'Flags'])

env.Append(CPPDEFINES = ["_SPLICE_MAYA_VERSION="+str(MAYA_VERSION[:4])])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)
env.MergeFlags(ADDITIONAL_FLAGS)

if FABRIC_BUILD_OS == 'Linux':
  env.Append(LIBS=['boost_filesystem', 'boost_system'])

target = 'FabricSpliceMaya'

mayaModule = None
sources = env.Glob('*.cpp')
sources += env.QTMOC(env.File('FabricDFGWidget.h'))

if FABRIC_BUILD_OS == 'Darwin':
  # a loadable module will omit the 'lib' prefix name on Os X
  spliceAppName = 'FabricSpliceMaya'+MAYA_VERSION
  target += '.bundle'
  env.Append(SHLINKFLAGS = ','.join([
    '-Wl',
    '-current_version',
    '.'.join([env['FABRIC_VERSION_MAJ'],env['FABRIC_VERSION_MIN'],env['FABRIC_VERSION_REV']]),
    '-compatibility_version',
    '.'.join([env['FABRIC_VERSION_MAJ'],env['FABRIC_VERSION_MIN'],'0']),
    '-install_name',
    '@rpath/Splice/Applications/'+spliceAppName+'/plugins/'+spliceAppName+".bundle"
    ]))
  mayaModule = env.LoadableModule(target = target, source = sources)
else:
  libSuffix = '.so'
  if FABRIC_BUILD_OS == 'Windows':
    libSuffix = '.mll'
  if FABRIC_BUILD_OS == 'Linux':
    exportsFile = env.File('Linux.exports').srcnode()
    env.Append(SHLINKFLAGS = ['-Wl,--version-script='+str(exportsFile)])
    env[ '_LIBFLAGS' ] = '-Wl,--start-group ' + env['_LIBFLAGS'] + ' -Wl,--end-group'
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

if FABRIC_BUILD_OS == 'Darwin':
  pythonVersion = '2.7'
else:
  pythonVersion = Glob(os.path.join(MAYA_INCLUDE_DIR, 'python*'))[0].abspath[-3:]

env.Append(BUILDERS = {
  'SubstMayaModuleFile': Builder(action = [
    [
      sedCmd,
      '-e', 's/{{MAYA_VERSION}}/'+moduleFileMayaVersion+'/g ',
      '-e', 's/{{PYTHON_VERSION}}/'+pythonVersion+'/g ',
      '<$SOURCE', '>$TARGET',
    ]
  ])
})

mayaModuleFile = env.SubstMayaModuleFile(
  env.File('FabricSpliceMaya.mod'),
  env.Dir('Module').File('FabricSpliceMaya.mod.template')
)[0]

mayaFiles = []
mayaFiles.append(env.Install(STAGE_DIR, mayaModuleFile))

for script in ['FabricSpliceMenu', 'FabricSpliceUI', 'FabricSpliceTool', 'FabricSpliceToolValues', 'FabricSpliceToolProperties', 'FabricDFGUI']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'scripts'), os.path.join('Module', 'scripts', script+'.mel')))
for script in ['AEspliceMayaNodeTemplate', 'AEdfgMayaNodeTemplate']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'scripts'), os.path.join('Module', 'scripts', script+'.py')))
for ui in ['FabricSpliceEditor', 'FabricDFGWidget']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'ui'), os.path.join('Module', 'ui', ui+'.ui')))
for png in ['FE_logo']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'ui'), os.path.join('Module', 'ui', png+'.png')))
for xpm in ['FE_tool']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'ui'), os.path.join('Module', 'ui', xpm+'.xpm')))
installedModule = env.Install(os.path.join(STAGE_DIR.abspath, 'plug-ins'), mayaModule)
mayaFiles.append(installedModule)

# also install the FabricCore dynamic library
if FABRIC_BUILD_OS == 'Linux':
  env.Append(LINKFLAGS = [Literal('-Wl,-rpath,$ORIGIN/../../../lib/')])
if FABRIC_BUILD_OS == 'Darwin':
  env.Append(LINKFLAGS = [Literal('-Wl,-rpath,@loader_path/../../..')])
# if FABRIC_BUILD_OS == 'Windows':
#   FABRIC_CORE_VERSION = FABRIC_SPLICE_VERSION.rpartition('.')[0]
#   mayaFiles.append(
#     env.Install(
#       os.path.join(STAGE_DIR.abspath, 'plug-ins'),
#       os.path.join(FABRIC_DIR, 'lib', 'FabricCore-' + FABRIC_CORE_VERSION + '.dll')
#       )
#     )
#   mayaFiles.append(
#     env.Install(
#       os.path.join(STAGE_DIR.abspath, 'plug-ins'),
#       os.path.join(FABRIC_DIR, 'lib', 'FabricCore-' + FABRIC_CORE_VERSION + '.pdb')
#       )
#     )

# todo: install the python client

alias = env.Alias('splicemaya', mayaFiles)
spliceData = (alias, mayaFiles)
Return('spliceData')
