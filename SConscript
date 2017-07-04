#
# Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
#

import os

Import(
  'parentEnv',
  'FABRIC_DIR',
  'FABRIC_SPLICE_VERSION',
  'STAGE_DIR',
  'FABRIC_BUILD_OS',
  'FABRIC_BUILD_TYPE',
  'FABRIC_BUILD_ARCH',
  'MAYA_BIN_DIR',
  'MAYA_INCLUDE_DIR',
  'MAYA_LIB_DIR',
  'MAYA_VERSION',
  'sharedCapiFlags',
  'spliceFlags',
  'ADDITIONAL_FLAGS',
  'commandsFlags',
  'astWrapperFlags',
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

uses_qt5 = int(float(str(MAYA_VERSION[:4]))) >= 2017

mayaFlags = {
  'CPPPATH': [
      env.Dir('lib').srcnode(),
      env.Dir('lib').Dir('Application').srcnode(),
      env.Dir('lib').Dir('Conversion').srcnode(),
      env.Dir('lib').Dir('Commands').srcnode(),
      env.Dir('lib').Dir('Viewports').srcnode(),
      env.Dir('plugin').srcnode(),
    ],
  'LIBPATH': [
    MAYA_LIB_DIR
  ],
}

maya_include_paths = [
  MAYA_INCLUDE_DIR
  ]

if FABRIC_BUILD_OS == 'Windows' and int(float(str(MAYA_VERSION[:4]))) < 2016 :
  maya_includes = os.path.join(MAYA_INCLUDE_DIR, 'Qt')
  maya_include_paths += [ maya_includes ]
else:
  maya_includes = MAYA_INCLUDE_DIR

maya_include_paths += [
  MAYA_INCLUDE_DIR,
  os.path.join(maya_includes, 'QtCore'),
  os.path.join(maya_includes, 'QtGui'),
  os.path.join(maya_includes, 'QtOpenGL'),
  ]
if uses_qt5:
  maya_include_paths += [
    os.path.join(maya_includes, 'QtWidgets'),
    ]

if FABRIC_BUILD_OS == 'Windows':
  mayaFlags['CPPPATH'] += maya_include_paths
else:
  # [andrew 2016-07-28] this is required because there are warnings generated
  # in the maya Qt headers themselves and we're compiling with -Werror
  maya_include_flags = []
  for path in maya_include_paths:
      maya_include_flags += ['-isystem'+ path]
  mayaFlags['CCFLAGS'] = maya_include_flags
 
mayaFlags['LIBS'] = ['OpenMaya', 'OpenMayaAnim', 'OpenMayaUI', 'OpenMayaRender', 'Foundation']
if FABRIC_BUILD_OS == 'Windows':
  mayaFlags['CPPDEFINES'] = ['NT_PLUGIN']
  if uses_qt5:
    mayaFlags['LIBS'].extend(['Qt5Core', 'Qt5Gui', 'Qt5OpenGL', 'Qt5Widgets'])
  else:
    mayaFlags['LIBS'].extend(['QtCore4', 'QtGui4', 'QtOpenGL4'])
  # FE-4590
  mayaFlags['CCFLAGS'] = ['/wd4190']
elif FABRIC_BUILD_OS == 'Linux':
  mayaFlags['CPPDEFINES'] = ['LINUX']
  if uses_qt5:
    mayaFlags['LIBS'].extend(['Qt5Core', 'Qt5Gui', 'Qt5OpenGL', 'Qt5Widgets'])
  else:
    mayaFlags['LIBS'].extend(['QtCore', 'QtGui', 'QtOpenGL'])
elif FABRIC_BUILD_OS == 'Darwin':
  mayaFlags['CPPDEFINES'] = ['OSMac_']
  if uses_qt5:
    qtCoreLib = File(os.path.join(MAYA_LIB_DIR, 'libQt5Core.dylib'))
    qtGuiLib = File(os.path.join(MAYA_LIB_DIR, 'libQt5Gui.dylib'))
    qtOpenGLLib = File(os.path.join(MAYA_LIB_DIR, 'libQt5OpenGL.dylib'))
    mayaFlags['LIBS'].extend([File(os.path.join(MAYA_LIB_DIR,  'libQt5Widgets.dylib'))])
  else:
    qtCoreLib = File(os.path.join(MAYA_LIB_DIR, 'QtCore'))
    qtGuiLib = File(os.path.join(MAYA_LIB_DIR, 'QtGui'))
    qtOpenGLLib = File(os.path.join(MAYA_LIB_DIR, 'QtOpenGL'))
  mayaFlags['LIBS'].extend([qtCoreLib, qtGuiLib, qtOpenGLLib])
  mayaFlags['CCFLAGS'].extend(['-Wno-#warnings', '-Wno-return-type-c-linkage'])

env.MergeFlags(mayaFlags)

# services flags
if FABRIC_BUILD_OS == 'Windows':
  if FABRIC_BUILD_TYPE == 'Release':
    env.Append(LIBS = ['FabricServices-MSVC-'+env['MSVC_VERSION']+'-mt'])
  else:
    env.Append(LIBS = ['FabricServices-MSVC-'+env['MSVC_VERSION']+'-mtd'])
else:
  env.Append(LIBS = ['FabricServices'])
env.Append(LIBS = ['FabricSplitSearch'])


# build the ui libraries for splice
uiLibPrefix = 'uiMaya'+str(MAYA_VERSION)

uiDir = env.Dir('#').Dir('Native').Dir('FabricUI')
if os.environ.has_key('FABRIC_UI_DIR'):
  uiDir = env.Dir(os.environ['FABRIC_UI_DIR'])
uiSconscript = uiDir.File('SConscript')
if not os.path.exists(uiSconscript.abspath):
  print "Error: You need to have FabricUI checked out to "+uiSconscript.dir.abspath

env.Append(CPPPATH = [os.path.join(os.environ['FABRIC_DIR'], 'include')])
env.Append(LIBPATH = [os.path.join(os.environ['FABRIC_DIR'], 'lib')])
env.Append(LIBPATH = [os.path.join(os.environ['FABRIC_DIR'], 'lib', 'Application')])
env.Append(LIBPATH = [os.path.join(os.environ['FABRIC_DIR'], 'lib', 'Conversion')])
env.Append(LIBPATH = [os.path.join(os.environ['FABRIC_DIR'], 'lib', 'Commands')])
 
if int(float(str(MAYA_VERSION[:4]))) >= 2016:
  env.Append(LIBPATH = [os.path.join(os.environ['FABRIC_DIR'], 'lib', 'Viewports')])

env.Append(CPPPATH = [os.path.join(os.environ['FABRIC_DIR'], 'include', 'FabricServices')])
env.Append(CPPPATH = [uiSconscript.dir])

if FABRIC_BUILD_TYPE == 'Debug':
  env.Append(CPPDEFINES = ['_DEBUG'])
  env.Append(CPPDEFINES = ['_ITERATOR_DEBUG_LEVEL=2'])

uiLibs = SConscript(uiSconscript, exports = {
  'parentEnv': env,
  'uiLibPrefix': uiLibPrefix,
  'qtDir': os.path.join(MAYA_INCLUDE_DIR, 'Qt'),
  'qtMOC': os.path.join(MAYA_BIN_DIR, 'moc'),
  'qtFlags': {

  },
  'fabricFlags': sharedCapiFlags,
  'buildType': FABRIC_BUILD_TYPE,
  'buildOS': FABRIC_BUILD_OS,
  'buildArch': FABRIC_BUILD_ARCH,
  'stageDir': env.Dir('#').Dir('stage').Dir('lib'),
  },
  duplicate=0,
  variant_dir = env.Dir('FabricUI')
  )

# import the maya specific libraries
Import(uiLibPrefix+'Flags')

# ui flags
env.MergeFlags(locals()[uiLibPrefix + 'Flags'])

env.MergeFlags(sharedCapiFlags)
env.MergeFlags(spliceFlags)
env.MergeFlags(ADDITIONAL_FLAGS)

target = 'FabricMaya'

mayaModule = None
libSources = env.Glob('lib/*.cpp')
libSources += env.Glob('lib/Commands/*.cpp')
libSources += env.Glob('lib/Conversion/*.cpp')
libSources += env.Glob('lib/Application/*.cpp')
if int(float(str(MAYA_VERSION[:4]))) >= 2016:
  libSources += env.Glob('lib/Viewports/*.cpp')
libSources += env.QTMOC(env.File('lib/FabricDFGWidget.h'))
libSources += env.QTMOC(env.File('lib/FabricImportPatternDialog.h'))
libSources += env.QTMOC(env.File('lib/FabricExportPatternDialog.h'))
libSources += env.QTMOC(env.File('lib/Commands/FabricCommandManagerCallback.h'))
libSources += env.QTMOC(env.File('lib/FabricProgressbarDialog.h'))

pluginSources = env.Glob('plugin/*.cpp')

libEnv = env.Clone()
# [andrew 20151110] see FE-5693
if FABRIC_BUILD_OS == 'Linux':
  libEnv.Append(LINKFLAGS = [Literal('-Wl,-rpath,$ORIGIN/../../lib/')])
  libFabricMaya = libEnv.SharedLibrary('libFabricMaya'+MAYA_VERSION, libSources)
else:
  libFabricMaya = libEnv.StaticLibrary('libFabricMaya'+MAYA_VERSION, libSources)

if FABRIC_BUILD_OS == 'Darwin':
  # a loadable module will omit the 'lib' prefix name on Os X
  spliceAppName = 'FabricMaya'+MAYA_VERSION
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
  mayaModule = env.LoadableModule(target = target, source = pluginSources)
else:
  libSuffix = '.so'
  if FABRIC_BUILD_OS == 'Windows':
    libSuffix = '.mll'
  if FABRIC_BUILD_OS == 'Linux':
    exportsFile = env.File('Linux.exports').srcnode()
    env.Append(SHLINKFLAGS = ['-Wl,--version-script='+str(exportsFile)])
  elif FABRIC_BUILD_OS == 'Darwin':
    exportsFile = env.File('Darwin.exports').srcnode()
    env.Append(SHLINKFLAGS = ['-Wl,-exported_symbols_list', str(exportsFile)])
  mayaModule = env.SharedLibrary(target = target, source = pluginSources, SHLIBSUFFIX=libSuffix, SHLIBPREFIX='')

sedCmd = 'sed'
if FABRIC_BUILD_OS == 'Windows':
  envPaths = os.environ['PATH'].split(os.pathsep)
  for envPath in envPaths:
    sedPath = os.path.join(envPath, 'sed.exe')
    if os.path.exists(sedPath):
      sedCmd = sedPath

moduleFileMayaVersion = MAYA_VERSION

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
  env.File('FabricMaya.mod'),
  env.Dir('Module').File('FabricMaya.mod.template')
)[0]

mayaFiles = []
mayaFiles.append(env.Install(STAGE_DIR, mayaModuleFile))
installedLibFabricMaya = env.Install(Dir(FABRIC_DIR).Dir('lib'), libFabricMaya)
mayaFiles.append(installedLibFabricMaya)

for png in ['canvasNode', 'out_canvasNode', 'canvasDeformer', 'out_canvasDeformer', 'canvasFuncNode', 'out_canvasFuncNode', 'canvasFuncDeformer', 'out_canvasFuncDeformer', 'canvasRigNode', 'out_canvasRigNode', 'fabricConstraint', 'out_fabricConstraint']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'icons'), os.path.join('Module', 'icons', png+'.png')))
for script in ['FabricSpliceMenu', 'FabricSpliceUI', 'FabricSpliceTool', 'FabricSpliceToolValues', 'FabricSpliceToolProperties', 'FabricDFGUI']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'scripts'), os.path.join('Module', 'scripts', script+'.mel')))
for script in ['AEfabricConstraintTemplate']:
  mayaFiles.append(env.Install(os.path.join(STAGE_DIR.abspath, 'scripts'), os.path.join('Module', 'scripts', script+'.mel')))
for script in ['AEspliceMayaNodeTemplate', 'AEcanvasNodeTemplate']:
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
# todo: install the python client

env.Append(LIBPATH = [env.Dir('.')])
if FABRIC_BUILD_OS == 'Linux':
  # [andrew 20151110] want to find only symbols in libFabricMaya.so
  env['LIBS'] = ['FabricMaya'+MAYA_VERSION]
else:
  env.Append(LIBS = [libFabricMaya])
env.Depends(mayaModule, installedLibFabricMaya)

alias = env.Alias('splicemaya', mayaFiles)
spliceData = (alias, mayaFiles)
Return('spliceData')
