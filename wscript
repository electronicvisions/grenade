#!/usr/bin/env python
import argparse
import os
from os.path import join
from waflib.extras.symwaf2ic import get_toplevel_path


def depends(ctx):
    ctx('code-format')
    ctx('dapr')
    ctx('logger')
    ctx('halco')
    ctx('haldls')
    ctx('hate')
    ctx('libnux')
    ctx('fisch')
    ctx('hxcomm')

    if getattr(ctx.options, 'with_grenade_python_bindings', True):
        ctx('grenade', 'pygrenade')


def options(opt):
    opt.load('compiler_cxx')
    opt.load('gtest')
    opt.load('doxygen')

    hopts = opt.add_option_group('grenade options')
    hopts.add_withoption('grenade-python-bindings', default=True,
                         help='Toggle the generation and build of grenade python bindings')


def configure(cfg):
    from waflib.Options import options as o
    if cfg.env.have_ppu_toolchain:
        cfg.define('WITH_GRENADE_PPU_SUPPORT', 1)
    cfg.write_config_header('include/grenade/build-config.h')

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
    bld.env.BBS_HARDWARE_AVAILABLE = "SLURM_HWDB_YAML" in os.environ
    bld.env.DLSvx_SIM_AVAILABLE = "FLANGE_SIMULATION_RCF_PORT" in os.environ

    bld(
        target = 'grenade_inc',
        export_includes = ['include']
    )

    bld.install_files(
        dest = '${PREFIX}/',
        files = bld.path.ant_glob('include/grenade/vx/ppu/**/*.(h|tcc)'),
        name = 'grenade_vx_ppu_header',
        relative_trick = True
    )

    bld.install_as(
        '${PREFIX}/share/grenade/base.cpp',
        'src/grenade/vx/ppu/start.cpp',
        name = 'grenade_ppu_base_vx'
    )

    if bld.env.have_ppu_toolchain:
        env = bld.all_envs[f'nux_vx_v3'].derive()
        env.detach()

        bld.stlib(
            target = 'grenade_ppu_vx',
            source = bld.path.ant_glob('src/grenade/vx/ppu/**/*.cpp', excl='src/grenade/vx/ppu/start.cpp'),
            install_path = '${PREFIX}/lib/ppu',
            use = ['grenade_inc', 'nux_vx_v3', 'haldls_ppu_vx_v3'],
            linkflags = '-Wl,-z,defs',
            env = env,
            uselib = 'GRENADE_LIBRARIES',
        )

    bld(
        target = 'grenade_common',
        features = 'cxx cxxshlib',
        source = bld.path.ant_glob('src/grenade/common/**/*.cpp'),
        install_path = '${PREFIX}/lib',
        use = ['grenade_inc', 'halco_common', 'hate', 'dapr'],
        uselib = 'GRENADE_LIBRARIES',
    )

    bld(
        target = 'grenade_vx',
        features = 'cxx cxxshlib',
        source = bld.path.ant_glob('src/grenade/vx/**/*.cpp', excl='src/grenade/vx/ppu/*.cpp'),
        install_path = '${PREFIX}/lib',
        use = ['grenade_inc', 'grenade_common', 'halco_hicann_dls_vx_v3', 'lola_vx_v3', 'haldls_vx_v3', 'stadls_vx_v3', 'TBB'],
        depends_on = ['grenade_ppu_base_vx', 'grenade_vx_ppu_header', 'grenade_ppu_vx', 'nux_vx_v3', 'nux_runtime_vx_v3.o', 'haldls_ppu_vx_v3'] if bld.env.have_ppu_toolchain else [],
        uselib = 'GRENADE_LIBRARIES',
    )

    bld(
        target = 'grenade_vx_serialization',
        features = 'cxx cxxshlib',
        source = bld.path.ant_glob('src/cereal/types/grenade/vx/**/*.cpp'),
        install_path = '${PREFIX}/lib',
        use = ['grenade_vx', 'haldls_vx_v3_serealization', 'lola_vx_v3_serealization', 'stadls_vx_v3_serialization', 'TBB'],
        depends_on = ['grenade_ppu_base_vx', 'grenade_vx_ppu_header', 'nux_vx_v3', 'nux_runtime_vx_v3.o', 'haldls_ppu_vx_v3'] if bld.env.have_ppu_toolchain else [],
        uselib = 'GRENADE_LIBRARIES',
    )

    bld(
        target = 'grenade_swtest_common',
        features = 'gtest cxx cxxprogram',
        source = bld.path.ant_glob('tests/sw/grenade/common/**/test-*.cpp'),
        test_main = 'tests/common/grenade/main.cpp',
        use = ['grenade_common', 'GTEST', 'grenade_test_common_inc', 'logger_obj'],
        linkflags = ['-lboost_program_options-mt'],
        test_timeout=240,
        install_path = '${PREFIX}/bin',
    )

    bld(
        target = 'grenade_swtest_vx',
        features = 'gtest cxx cxxprogram',
        source = bld.path.ant_glob('tests/sw/grenade/vx/**/test-*.cpp'),
        test_main = 'tests/common/grenade/main.cpp',
        use = ['grenade_vx', 'GTEST', 'grenade_test_common_inc', 'grenade_vx_serialization', 'grenade_test_common_inc'],
        linkflags = ['-lboost_program_options-mt'],
        test_timeout=240,
        install_path = '${PREFIX}/bin',
    )

    bld(
        target = 'grenade_hwtest_helper_vx',
        features = 'cxx cxxshlib',
        source = bld.path.ant_glob('tests/hw/grenade/vx/helper.cpp'),
        use = ['grenade_vx', 'stadls_vx_v3', 'haldls_vx_v3', 'lola_vx_v3'],
        uselib = 'GRENADE_LIBRARIES',
        export_includes = 'tests/hw/grenade/vx',
    )

    bld(
        target = 'grenade_hwtest_vx',
        features = 'gtest cxx cxxprogram',
        source = bld.path.ant_glob('tests/hw/grenade/vx/**/test-*.cpp'),
        test_main = 'tests/common/grenade/main.cpp',
        use = ['grenade_vx', 'stadls_vx_v3', 'GTEST', 'haldls_vx_v3', 'lola_vx_v3', 'grenade_hwtest_helper_vx'],
        install_path = '${PREFIX}/bin',
        linkflags = ['-lboost_program_options-mt'],
        test_timeout=480,
        skip_run = not bld.env.BBS_HARDWARE_AVAILABLE
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
                "PREDEFINED": "GENPYBIND(...)= GENPYBIND_MANUAL(...)= SYMBOL_VISIBLE=",
                "WARN_LOGFILE": join(get_toplevel_path(), "build/grenade/grenade_doxygen_warnings.log"),
                "INCLUDE_PATH": join(get_toplevel_path(), "grenade", "include")
            },
        )

    bld(
    target = 'grenade_test_common_inc',
    export_includes = 'tests/common',
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
