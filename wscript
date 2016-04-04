#! /usr/bin/env python
# encoding: utf-8

def options(opt):
	opt.load('compiler_c')

def configure(conf):

	conf.recurse('ptask')

	conf.load('ar')
	conf.load('compiler_c')

	if 'LIB_PTHREAD' not in conf.env:
		conf.check_cc(lib = 'pthread')

	conf.env.append_value('CFLAGS', '-O3')
	conf.env.append_value('CFLAGS', '-std=c99')
	conf.env.append_value('CFLAGS', '-march=native')


def build(bld):

	bld.recurse('ptask')

	bld.stlib(
		source = ['psort.c'],
		target = 'psort',
		lib = ['pthread'],
		use = ['ptask'])

	bld.program(
		source = ['psort.c'],
		target = 'unittest',
		lib = ['pthread'],
		use = ['ptask'],
		defines = ['TEST'])

