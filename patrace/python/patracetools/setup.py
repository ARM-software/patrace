from setuptools import setup
import os

here = os.path.abspath(os.path.dirname(__file__))
content_capturing_dir = os.path.abspath(os.path.join(here, os.pardir, os.pardir, os.pardir))


def generate_version():
    cmakelist = os.path.join(here, '../../project/cmake/CMakeLists.txt')
    patch = None
    minor = None
    major = None
    commitid = os.environ.get("GIT_COMMIT", "unknown_commit_id")
    branch = os.environ.get("GIT_BRANCH", "unknown_branch")
    version_type = os.environ.get("VERSION_TYPE", "dev")

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

    with open('patracetools/version.py', 'w') as f:
        version='{major}.{minor_patch}'.format(major=major, minor_patch=("{}.{}".format(minor, patch) if patch != "0" else minor))

        template = """
version='{version}'
version_full='{version} {branch} {commitid} {version_type}'
commitid='{commitid}'
branch='{branch}'
version_type='{version_type}'
"""

        f.write(template.format(
            version=version,
            commitid=commitid,
            branch=branch,
            version_type=version_type
        ))

if not os.path.exists(os.path.join(here, 'PKG-INFO')):
    generate_version()


def version():
    from patracetools.version import version
    return version

setup(
    name="patracetools",
    version=version(),
    description="Various tools to manipulate PAT trace files",

    # The project URL.
    url='http://wiki.arm.com/MPD/PaTrace',

    # Author details
    author='PPA Team',
    author_email='MPG-Eng-PPA@arm.com',

    # Choose your license
    license='',

    classifiers=[
        # How mature is this project? Common values are
        #   3 - Alpha
        #   4 - Beta
        #   5 - Production/Stable
        'Development Status :: 4 - Beta',

        # Indicate who your project is intended for
        'Intended Audience :: Developers',
        'Topic :: Utilities',

        # Pick your license as you wish (should match "license" above)
        #'License :: OSI Approved :: MIT License',

        # Specify the Python versions you support here. In particular, ensure
        # that you indicate whether you support Python 2, Python 3 or both.
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        #'Programming Language :: Python :: 3',
        #'Programming Language :: Python :: 3.1',
        #'Programming Language :: Python :: 3.2',
        #'Programming Language :: Python :: 3.3',
    ],

    # What does your project relate to?
    keywords='patrace trace pat',

    # You can just specify the packages manually here if your project is
    # simple. Or you can use find_packages.
    #packages=find_packages(exclude=["contrib", "docs", "tests*"]),
    packages=['patracetools'],

    # List run-time dependencies here.  These will be installed by pip when your
    # project is installed.
    install_requires=[
        'pypatrace',
        'PyOpenGL',
    ],

    # To provide executable scripts, use entry points in preference to the
    # "scripts" keyword. Entry points provide cross-platform support and allow
    # pip to create the appropriate form of executable for the target platform.
    entry_points={
        'console_scripts': [
            'pat-remap-egl=patracetools.remap_egl:main',
            'pat-remap-tid=patracetools.remap_tid:main',
            'pat-upgrade-version=patracetools.upgrade_version:main',
            #'pat-change-texture-size=patracetools.ChangeTextureSize:main',
            'pat-set-header-size=patracetools.set_header_size:main',
            'pat-update-md5=patracetools.update_md5:main',
            'pat-minimize-texture-usage=patracetools.MinimizeTextureUsage:main',
            'pat-resize-fbos=patracetools.ResizeFBOs:main',
            'pat-dump-header=patracetools.pat_jsondump:main',
            'pat-version=patracetools.pat_version:main',
            #'pat-set-resolution=patracetools.set_resolution:main',
            #'pat-editor=patracetools.trace_editor:main',
            'pat-trim=patracetools.trace_trim:main',
            'pat-remove-frames=patracetools.remove_frames:main',
            'pat-edit-header=patracetools.edit_header:main',
            'pat-dump-textures=patracetools.dump_textures:main',
            'pat-get-call-numbers=patracetools.get_call_numbers:main',
        ],
    },
)
