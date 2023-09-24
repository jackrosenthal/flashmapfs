project('fmapfs', 'c')

libfuse = dependency('fuse3')
add_global_arguments('-DFUSE_USE_VERSION=35', language: 'c')

coverage_args = []
if get_option('b_coverage')
  coverage_args = ['-fprofile-instr-generate', '-fcoverage-mapping']
  add_global_arguments(coverage_args, language: 'c')
endif

sources = [
  '3rdparty/flashmap/fmap.c',
  'arena.c',
  'boolean_flag_file.c',
  'fs.c',
  'gbb.c',
  'main.c',
  'mmap_file.c',
  'raw_file.c',
  'route.c',
  'str_file.c',
  'version_file.c',
]

executable(
  'fmapfs',
  sources,
  dependencies: [libfuse],
  include_directories: include_directories(
    '3rdparty/flashmap',
    'include',
  ),
  link_args: coverage_args,
)
