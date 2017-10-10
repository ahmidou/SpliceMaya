import os, platform
import shutil

spliceEnv = Environment()

def RemoveFolderCmd(target, source, env):
  if os.path.exists(source[0].abspath):
    shutil.rmtree(source[0].abspath)

# define the clean target
if 'clean' in COMMAND_LINE_TARGETS:
  cleanBuild = spliceEnv.Command( spliceEnv.File('cleaning build folder'), spliceEnv.Dir('.build'), RemoveFolderCmd )
  cleanStage = spliceEnv.Command( spliceEnv.File('cleaning stage folder'), spliceEnv.Dir('.stage'), RemoveFolderCmd )
  spliceEnv.Alias('clean', [cleanBuild, cleanStage])
  Return()

# check environment variables
for var in [
  'FABRIC_DIR',
  'FABRIC_SPLICE_VERSION',
  'FABRIC_BUILD_OS',
  'FABRIC_BUILD_ARCH',
  'FABRIC_BUILD_TYPE',
  'MAYA_BIN_DIR',
  'MAYA_INCLUDE_DIR',
  'MAYA_LIB_DIR',
  'MAYA_VERSION',
  'FABRIC_UI_DIR',
  'BOOST_DIR',
]:
  if not os.environ.has_key(var):
    print 'The environment variable %s is not defined.' % var
    exit(0)
  if var.lower().endswith('_dir'):
    if not os.path.exists(os.environ[var]):
      print 'The path for environment variable %s does not exist.' % var
      exit(0)


FTL_INCLUDE_DIR = os.path.join(os.environ['FABRIC_DIR'], 'include')
Export('FTL_INCLUDE_DIR')
commandsFlags = {}
astWrapperFlags = {}
astWrapperFlags = {}
codeCompletionFlags = {}
Export('commandsFlags', 'astWrapperFlags', 'codeCompletionFlags')

spliceEnv = Environment(
  MSVC_VERSION = ( os.environ['MSVC_VERSION'] if 'MSVC_VERSION' in os.environ else '12.0' )
)

if not os.path.exists(spliceEnv.Dir('.stage').Dir('lib').abspath):
  os.makedirs(spliceEnv.Dir('.stage').Dir('lib').abspath)

# determine if we have the SpliceAPI two levels up
spliceApiDir = spliceEnv.Dir('..').Dir('..').Dir('SpliceAPI')

# try to get the Splice API from there
if os.path.exists(spliceApiDir.abspath):

  (spliceAPIAlias, spliceAPIFiles) = SConscript(
    dirs = [spliceApiDir],
    exports = {
      'parentEnv': spliceEnv,
      'FABRIC_DIR': os.environ['FABRIC_DIR'],
      'FABRIC_SPLICE_VERSION': os.environ['FABRIC_SPLICE_VERSION'],
      'FABRIC_BUILD_TYPE': os.environ['FABRIC_BUILD_TYPE'],
      'FABRIC_BUILD_OS': os.environ['FABRIC_BUILD_OS'],
      'FABRIC_BUILD_ARCH': os.environ['FABRIC_BUILD_ARCH'],
      'STAGE_DIR': spliceEnv.Dir('.stage'),
      'BOOST_DIR': os.environ['BOOST_DIR'],
    },
    duplicate=0,
    variant_dir = spliceEnv.Dir('.build').Dir('SpliceAPI')
  )
  
  spliceApiDir = spliceEnv.Dir('.build').Dir('SpliceAPI').Dir('.stage').abspath
  
else:

  print 'The folder "'+spliceApiDir.abspath+'" does not exist. Please see the README.md for build instructions.'
  exit(0)

(mayaSpliceAlias, mayaSpliceFiles, returned) = SConscript(
  os.path.join('SConscript'),
  exports = {
    'parentEnv': spliceEnv,
    'FABRIC_DIR': os.environ['FABRIC_DIR'],
    'FABRIC_SPLICE_VERSION': os.environ['FABRIC_SPLICE_VERSION'],
    'FABRIC_BUILD_TYPE': os.environ['FABRIC_BUILD_TYPE'],
    'FABRIC_BUILD_OS': os.environ['FABRIC_BUILD_OS'],
    'FABRIC_BUILD_ARCH': os.environ['FABRIC_BUILD_ARCH'],
    'STAGE_DIR': spliceEnv.Dir('.stage').Dir('DCCIntegrations').Dir('FabricMaya'+os.environ['MAYA_VERSION']),
    'MAYA_BIN_DIR': os.environ['MAYA_BIN_DIR'],
    'MAYA_INCLUDE_DIR': os.environ['MAYA_INCLUDE_DIR'],
    'MAYA_LIB_DIR': os.environ['MAYA_LIB_DIR'],
    'MAYA_VERSION': os.environ['MAYA_VERSION'],
    'ADDITIONAL_FLAGS': {}
  },
  duplicate=0,
  variant_dir = spliceEnv.Dir('.build').Dir(os.environ['MAYA_VERSION'])
)

if os.environ['FABRIC_BUILD_OS'] == 'Windows' :

  msvsEnv = returned['msvs']['env']
  msvsEnv['CPPPATH'] = [ p if type(p) is str else p.srcnode().abspath for p in msvsEnv['CPPPATH'] ]
  msvsProj = msvsEnv.MSVSProject(
    target = 'MSVS/SpliceMaya' + msvsEnv['MSVSPROJECTSUFFIX'],
    srcs = [ os.path.join(spliceEnv.Dir('.').srcnode().abspath, str(s)) for s in returned['msvs']['src'] ],
    variant = 'Release'
  )
  msvsEnv.Alias( "MSVS", msvsProj )


allAliases = [mayaSpliceAlias]
spliceEnv.Alias('all', allAliases)
spliceEnv.Default(allAliases)
