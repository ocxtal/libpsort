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

	conf.env.append_value('CFLAGS', '-O3')
	conf.env.append_value('CFLAGS', '-std=c99')
	conf.env.append_value('CFLAGS', '-march=native')

	conf.env.append_value('LIB_PSORT', conf.env.LIB_PTASK)
	conf.env.append_value('OBJ_PSORT', ['psort.o'] + conf.env.OBJ_PTASK)


def build(bld):

	bld.recurse('ptask')

	bld.objects(source = 'psort.c', target = 'psort.o')

	bld.stlib(
		source = ['unittest.c'],
		target = 'psort',
		use = bld.env.OBJ_PSORT,
		lib = bld.env.LIB_PSORT)

	bld.program(
		source = ['unittest.c'],
		target = 'unittest',
		use = bld.env.OBJ_PSORT,
		lib = bld.env.LIB_PSORT,
		defines = ['TEST'])
