#!/usr/bin/env python
import argparse
import os
from os.path import join
from waflib.extras.symwaf2ic import get_toplevel_path


def depends(ctx):
    ctx('code-format')
    ctx('logger')
    ctx('halco')
    ctx('haldls')
    ctx('hate')

    ctx('grenade', 'pygrenade')


def options(opt):
    opt.load('compiler_cxx')
    opt.load('gtest')
    opt.load('doxygen')


def configure(cfg):
    cfg.load('compiler_cxx')
    cfg.load('gtest')

    cfg.load('local_rpath')
    cfg.load('doxygen')

    cfg.check_cxx(lib='tbb', uselib_store="TBB")

    cfg.env.CXXFLAGS_GRENADE_LIBRARIES = [
        '-fvisibility=hidden',
        '-fvisibility-inlines-hidden',
    ]
    cfg.env.LINKFLAGS_GRENADE_LIBRARIES = [
        '-fvisibility=hidden',
        '-fvisibility-inlines-hidden',
    ]


def build(bld):
    bld.env.DLSvx_HARDWARE_AVAILABLE = "cube" == os.environ.get("SLURM_JOB_PARTITION")
    bld.env.DLSvx_SIM_AVAILABLE = "FLANGE_SIMULATION_RCF_PORT" in os.environ

    bld(
        target = 'grenade_inc',
        export_includes = 'include',
    )

    bld(
        target = 'grenade_vx',
        features = 'cxx cxxshlib pyembed',
        source = bld.path.ant_glob('src/grenade/vx/*.cpp'),
        install_path = '${PREFIX}/lib',
        use = ['grenade_inc', 'halco_hicann_dls_vx_v2', 'lola_vx_v2', 'haldls_vx_v2', 'stadls_vx_v2', 'TBB'],
        uselib = 'GRENADE_LIBRARIES',
    )

    bld(
        target = 'grenade_swtest_vx',
        features = 'gtest cxx cxxprogram pyembed',
        source = bld.path.ant_glob('tests/sw/grenade/vx/test-*.cpp'),
        use = ['grenade_vx', 'GTEST', 'grenade_test_common_inc'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target = 'grenade_hwtest_vx',
        features = 'gtest cxx cxxprogram pyembed',
        source = bld.path.ant_glob('tests/hw/grenade/vx/test-*.cpp'),
        use = ['grenade_vx', 'stadls_vx_v2', 'GTEST', 'haldls_vx_v2', 'lola_vx_v2'],
        install_path = '${PREFIX}/bin',
        skip_run = not bld.env.DLSvx_HARDWARE_AVAILABLE
    )

    bld(
        features = 'doxygen',
        doxyfile = bld.root.make_node('%s/code-format/doxyfile' % get_toplevel_path()),
        install_path = 'doc/grenade',
        pars = {
            "PROJECT_NAME": "\"GRENADE\"",
            "INPUT": "%s/grenade/include/grenade" % get_toplevel_path(),
            "OUTPUT_DIRECTORY": "%s/build/grenade/grenade/doc" % get_toplevel_path(),
            "PREDEFINED": "GENPYBIND()= GENPYBIND_MANUAL()= GENPYBIND_TAG_GRENADE_VX=",
            "WARN_LOGFILE": join(get_toplevel_path(), "build/grenade/grenade_doxygen_warnings.log"),
            "INCLUDE_PATH": join(get_toplevel_path(), "grenade", "include")
        },
    )
