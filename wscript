#! /usr/bin/env python
# encoding: utf-8

def options(opt):
	opt.recurse('ptask')
	opt.load('compiler_c')

def configure(conf):

	conf.recurse('ptask')

	conf.load('ar')
	conf.load('compiler_c')

	if 'LIB_PTHREAD' not in conf.env:
		conf.check_cc(lib = 'pthread')

	conf.env.append_value('LIB_PSORT', conf.env.LIB_PTHREAD)
	conf.env.append_value('CFLAGS', '-g')
	conf.env.append_value('CFLAGS', '-std=c99')
	conf.env.append_value('CFLAGS', '-march=native')


def build(bld):

	bld.recurse('ptask')

	bld.stlib(
		source = ['psort.c'],
		target = 'psort',
		lib = bld.env.LIB_PSORT,
		use = ['ptask'])

	bld.program(
		source = ['unittest.c'],
		target = 'unittest',
		linkflags = ['-all_load'],
		use = ['ptask', 'psort'],
		lib = bld.env.LIB_PSORT,
		defines = ['TEST'])

