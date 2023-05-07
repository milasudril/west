#!/usr/bin/env python3

import sys
import json
import subprocess
import os
import shutil

def compile_file(args):
	input_file = args['source_file']
	output_dir = os.path.dirname(args['targets'][0].rstrip('/'))
	tempdir = '/tmp/west_%s_%d'%(args['build_info']['build_id'], args['task_id'])
	os.mkdir(tempdir)
	prog_opts = ['plantuml', '-tsvg', '-o%s'%tempdir, input_file]
	res = subprocess.run(prog_opts)
	if res.returncode != 0:
		shutil.rmtree(tempdir)
		return res.returncode
	else:
		for file in os.listdir(tempdir):
			shutil.move('%s/%s'%(tempdir, file), '%s/%s'%(output_dir, file))
		shutil.rmtree(tempdir)
		return res.returncode

if __name__ == '__main__':
	if sys.argv[1] == 'configure':
		print('{}');
	elif sys.argv[1] == 'compile':
		exit(compile_file(json.loads(sys.argv[2])))
	else:
		eprint('Unhandled action: %s'%sys.argv[1])
		exit(-1)