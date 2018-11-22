import os
import sys
import subprocess
import setuptools

here = os.path.abspath(os.path.dirname(__file__))

# Since build_py (which copies python sources to the build directory) comes before build_ex (which calls swig to generate patrace.py), we have to manually call swig;
# This should only be called in the distribution side.
if not os.path.exists(os.path.join(here, 'patrace.py')):
    input_file = os.path.join(here, 'patrace.i')

    if not os.path.exists(input_file):
        raise Exception("Unable to find {0}".format(input_file))

    command = 'swig -python -c++ -outdir {outdir} -o patrace_wrap.cpp {input_file}'.format(
        outdir=here,
        input_file=input_file,
    )
    print "Executing {0}".format(command)
    subprocess.call(command.split())

on_64bit_platform = sys.maxsize > 2 ** 32


def generate_version():
    cmakelist = os.path.join(here, '../../project/cmake/CMakeLists.txt')
    patch = None
    minor = None
    major = None
    with open(cmakelist, 'r') as f:
        for line in f:
            if line.strip().startswith('#'):
                continue

            if 'PATRACE_VERSION_PATCH' in line and patch is None:
                patch = line.split()[-1].split(')')[0]
            elif 'PATRACE_VERSION_MINOR' in line and minor is None:
                minor = line.split()[-1].split(')')[0]
            elif 'PATRACE_VERSION_MAJOR' in line and major is None:
                major = line.split()[-1].split(')')[0]

    with open('version.py', 'w') as f:
        f.write("version='{major}.{minor_patch}'".format(major=major, minor_patch=("{}.{}".format(minor, patch) if patch != "0" else minor)))

if not os.path.exists(os.path.join(here, 'PKG-INFO')):
    generate_version()


def version():
    from version import version
    return version

patrace_extension = setuptools.Extension(
    '_patrace',
    extra_compile_args=['-std=gnu++0x'],
    include_dirs=[
        'src',
        'common',
        'thirdparty',
        'thirdparty/snappy',
        'thirdparty/opengl-registry/api',
        'thirdparty/jsoncpp/include',
    ],
    sources=[
        'patrace_wrap.cpp',

        'src/common/call_parser.cpp',
        'src/common/trace_model.cpp',

        'src/common/api_info_auto.cpp',
        'src/common/api_info.cpp',
        'src/common/in_file.cpp',
        'src/common/in_file_ra.cpp',
        'src/common/out_file.cpp',
        'src/common/os_posix.cpp',

        'common/eglstate/common.cpp',

        # jsoncpp
        'thirdparty/jsoncpp/src/lib_json/json_writer.cpp',
        'thirdparty/jsoncpp/src/lib_json/json_reader.cpp',
        'thirdparty/jsoncpp/src/lib_json/json_value.cpp',

        # snappy
        'thirdparty/snappy/snappy.cc',
        'thirdparty/snappy/snappy-sinksource.cc',
        'thirdparty/snappy/snappy-stubs-internal.cc',
        'thirdparty/snappy/snappy-c.cc',
    ],
    define_macros=[('PLATFORM_64BIT', None)] if on_64bit_platform else [],
)

setuptools.setup(
    name='pypatrace',
    description='Python interface of PATrace SDK',
    maintainer='PPA Team',
    maintainer_email='MPG-Eng-PPA@arm.com',
    version=version(),
    ext_modules=[patrace_extension],
    py_modules=['patrace'],
)
