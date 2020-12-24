#!/usr/bin/env python
import argparse
import os
import sys
import subprocess

from build_all import (
    build_project,
    build_android,
    all_platforms,
    linux_platforms,
)

script_dir = os.path.dirname(os.path.realpath(__file__))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Build content capturing projects',
        usage="Usage: %(prog)s [options]\nExample: %(prog)s --build-dir /work/mybuild/debug --install-dir /work/mybuild/bin patrace fbdev_arm debug",
        epilog="see build_all.py for building every target including Android"
    )
    parser.add_argument('project',
                        help='the only valid value is \'patrace\'')
    parser.add_argument('--build-dir', default='',
                        help='path to build directory. Defaults to \'builds\' in the top level project directory.')
    parser.add_argument('--install-dir', default='',
                        help='path to install directory. When relative, it is relative to the build directory. Defaults to \'install\' in the top level project directory.')
    parser.add_argument(
        'platform', default='x11_x64',
        help="window system and architecture separated with an underscore. Possible values: {platforms}. These are are defined in patrace/project/cmake/toolchains".format(
            platforms=', '.join(all_platforms)
        )
    )

    parser.add_argument('type', default='debug',
                        help='the build type. Possible values: debug, release, sanitizer')
    args = parser.parse_args()

    if not args.build_dir:
        args.build_dir = os.path.abspath(os.path.join(
            script_dir, '..', 'builds', args.project, args.platform, args.type
        ))

    if not args.install_dir:
        args.install_dir = os.path.abspath(os.path.join(
            script_dir, '..', 'install', args.project, args.platform, args.type
        ))

    if args.platform == 'android':
        # Run CMake first to create generated files
        build_project('x11_x64', variant=args.type, build_dir=args.build_dir, install_dir=args.install_dir,
            project_path=os.path.abspath(os.path.join(script_dir, '..', 'patrace', 'project', 'cmake')),
            cmake_defines=[], stop_early=True
        )
        # Run Android build system
        returncode = build_android(
            src_path=os.path.join(script_dir, '..'),
            product_names=[args.project],
            install_base_dir=args.install_dir
        )
    else:
        if not args.platform in linux_platforms:
            print >> sys.stderr, 'Unsupported platform: %s' % args.platform
            sys.exit(-1)
        returncode = build_project(
            platform=args.platform,
            variant=args.type,
            build_dir=args.build_dir,
            install_dir=args.install_dir,
            project_path=os.path.abspath(os.path.join(script_dir, '..', 'patrace', 'project', 'cmake')),
            cmake_defines=[]
        )

    sys.exit(returncode)
