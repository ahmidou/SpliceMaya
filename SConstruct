import os, platform

AddOption(
  '--debug-build',
  action='store_true',
  help='debug build',
  default=False
)

# check environment variables
for var in ['FABRIC_CAPI_DIR', 'FABRIC_SPLICE_VERSION', 'BOOST_DIR', 'MAYA_DIR', 'MAYA_VERSION']:
  if not os.environ.has_key(var):
    print 'The environment variable %s is not defined.' % var
    exit(0)
  if var.lower().endswith('_dir'):
    if not os.path.exists(os.environ[var]):
      print 'The path for environment variable %s does not exist.' % var
      exit(0)

spliceEnv = Environment()

if not os.path.exists(spliceEnv.Dir('.stage').abspath):
  os.makedirs(spliceEnv.Dir('.stage').abspath)

# determine if we have the SpliceAPI two levels up
spliceApiDir = spliceEnv.Dir('..').Dir('..').Dir('SpliceAPI')

# try to get the Splice API from there
if os.path.exists(spliceApiDir.abspath):

  # print spliceEnv['CPPPATH']

  (spliceAPIAlias, spliceAPIFiles) = SConscript(
    dirs = [spliceApiDir],
    exports = {
      'parentEnv': spliceEnv,
      'FABRIC_CAPI_DIR': os.environ['FABRIC_CAPI_DIR'],
      'CORE_VERSION': os.environ['FABRIC_SPLICE_VERSION'].rpartition('.')[0],
      'SPLICE_VERSION': os.environ['FABRIC_SPLICE_VERSION'],
      'STAGE_DIR': spliceEnv.Dir('.build').Dir('SpliceAPI').Dir('.stage'),
      'SPLICE_DEBUG': GetOption('debug_build'),
      'SPLICE_OS': platform.system(),
      'SPLICE_ARCH': 'x86_64',
      'BOOST_DIR': os.environ['BOOST_DIR']
    },
    variant_dir = spliceEnv.Dir('.build').Dir('SpliceAPI')
  )

  print spliceEnv['CPPPATH']
  
  spliceApiDir = spliceEnv.Dir('.build').Dir('SpliceAPI').Dir('.stage').abspath
  
else:

  print 'The folder "'+spliceApiDir.abspath+'" does not exist. Please see the README.md for build instructions.'
  exit(0)

(mayaSpliceAlias, mayaSpliceFiles) = SConscript(
  dirs = ['.'],
  exports = {
    'parentEnv': spliceEnv,
    'FABRIC_CAPI_DIR': os.environ['FABRIC_CAPI_DIR'],
    'CORE_VERSION': os.environ['FABRIC_SPLICE_VERSION'].rpartition('.')[0],
    'SPLICE_VERSION': os.environ['FABRIC_SPLICE_VERSION'],
    'STAGE_DIR': spliceEnv.Dir('.stage').Dir('Applications').Dir('FabricSpliceMaya'+os.environ['MAYA_VERSION']),
    'SPLICE_DEBUG': GetOption('debug_build'),
    'SPLICE_OS': platform.system(),
    'SPLICE_ARCH': 'x86_64',
    'BOOST_DIR': os.environ['BOOST_DIR'],
    'MAYA_DIR': os.environ['MAYA_DIR'],
    'MAYA_VERSION': os.environ['MAYA_VERSION']
  },
  variant_dir = spliceEnv.Dir('.build').Dir(os.environ['MAYA_VERSION'])
)

allAliases = [mayaSpliceAlias]
spliceEnv.Alias('all', allAliases)
