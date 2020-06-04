#!/usr/bin/env python
from waflib.extras.test_base import summary


def depends(ctx):
    pass


def options(opt):
    opt.load("test_base")


def configure(cfg):
    cfg.load("test_base")


def build(bld):
    bld.add_post_fun(summary)
