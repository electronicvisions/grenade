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

    if getattr(ctx.options, 'with_grenade_python_bindings', True):
        ctx('grenade', 'pygrenade')

    if getattr(ctx.options, 'with_grenade_ppu_support', True):
        ctx('libnux')


def options(opt):
    opt.load('compiler_cxx')
    opt.load('gtest')
    opt.load('doxygen')

    hopts = opt.add_option_group('grenade options')
    hopts.add_withoption('grenade-python-bindings', default=True,
                         help='Toggle the generation and build of grenade python bindings')
    hopts.add_withoption('grenade-ppu-support', default=True,
                       help='Toggle support for the PPU')


def configure(cfg):
    cfg.env.build_with_grenade_ppu_support = cfg.options.with_grenade_ppu_support

    from waflib.Options import options as o
    if cfg.env.build_with_grenade_ppu_support:
        cfg.define('WITH_GRENADE_PPU_SUPPORT', 1)
    cfg.write_config_header('grenade-build-config.h')

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
        source = bld.path.ant_glob('src/grenade/vx/**/*.cpp', excl='src/grenade/vx/ppu/*.cpp'),
        install_path = '${PREFIX}/lib',
        use = ['grenade_inc', 'halco_hicann_dls_vx_v2', 'lola_vx_v2', 'haldls_vx_v2', 'stadls_vx_v2', 'TBB'],
        depends_on = ['grenade_ppu_base_vx'] if bld.env.build_with_grenade_ppu_support else [],
        uselib = 'GRENADE_LIBRARIES',
    )

    if bld.env.build_with_grenade_ppu_support:
        bld.program(
            target = 'grenade_ppu_base_vx',
            features = 'cxx',
            source = bld.path.ant_glob('src/grenade/vx/ppu/*.cpp'),
            use = ['grenade_inc', 'nux_vx_v2', 'nux_runtime_vx_v2'],
            env = bld.all_envs['nux_vx_v2'],
        )

    bld(
        target = 'grenade_swtest_vx',
        features = 'gtest cxx cxxprogram pyembed',
        source = bld.path.ant_glob('tests/sw/grenade/vx/**/test-*.cpp'),
        test_main = 'tests/common/grenade/vx/main.cpp',
        use = ['grenade_vx', 'GTEST', 'grenade_test_common_inc'],
        linkflags = ['-lboost_program_options-mt'],
        install_path = '${PREFIX}/bin',
    )

    bld(
        target = 'grenade_hwtest_vx',
        features = 'gtest cxx cxxprogram pyembed',
        source = bld.path.ant_glob('tests/hw/grenade/vx/**/test-*.cpp'),
        test_main = 'tests/common/grenade/vx/main.cpp',
        use = ['grenade_vx', 'stadls_vx_v2', 'GTEST', 'haldls_vx_v2', 'lola_vx_v2'],
        install_path = '${PREFIX}/bin',
        linkflags = ['-lboost_program_options-mt'],
        skip_run = not bld.env.DLSvx_HARDWARE_AVAILABLE
    )

    if bld.env.DOXYGEN:
        bld(
            target = 'doxygen_grenade',
            features = 'doxygen',
            doxyfile = bld.root.make_node('%s/code-format/doxyfile' % get_toplevel_path()),
            install_path = 'doc/grenade',
            pars = {
                "PROJECT_NAME": "\"GRENADE\"",
                "INPUT": "%s/grenade/include/grenade" % get_toplevel_path(),
                "OUTPUT_DIRECTORY": "%s/build/grenade/doc" % get_toplevel_path(),
                "PREDEFINED": "GENPYBIND()= GENPYBIND_MANUAL()= GENPYBIND_TAG_GRENADE_VX=",
                "WARN_LOGFILE": join(get_toplevel_path(), "build/grenade/grenade_doxygen_warnings.log"),
                "INCLUDE_PATH": join(get_toplevel_path(), "grenade", "include")
            },
        )

# for grenade_vx's runtime dependency on grenade_ppu_base_vx
from waflib.TaskGen import feature, after_method
@feature('*')
@after_method('process_use')
def post_the_other_dependencies(self):
    deps = getattr(self, 'depends_on', [])
    for name in set(self.to_list(deps)):
        other = self.bld.get_tgen_by_name(name)
        other.post()
