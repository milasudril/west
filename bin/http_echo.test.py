#!/bin/env python3

#@	{
#@	"target":{"name":"http_echo_test_result.txt"},
#@	"dependencies":[{"ref":"./http_echo"}]
#@	}

import sys
import json
import subprocess
import io
import socket

def get_ports(text_src):
	ret = {}
	for line in text_src:
		stripped_line = line.rstrip()
		keyval = stripped_line.split(' ')
		if keyval[0] == 'http' or keyval[0] == 'adm':
			ret[keyval[0]] = int(keyval[1])
		if 'http' in ret and 'adm' in ret:
			return ret

def talk_http(port):
	data = [{'request': '''POST /first_request HTTP/1.1
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8
Accept-Encoding: gzip, deflate, br
Accept-Language: en-US,en;q=0.5
Connection: keep-alive
Content-Length: 604
Content-Type: application/x-www-form-urlencoded
Host: 127.0.0.1:49152
Origin: null
Sec-Fetch-Dest: iframe
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: cross-site
Sec-Fetch-User: ?1
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/113.0

'''.replace('\n', '\r\n') + '''data=Phasellus+leo+velit%2C+volutpat+sit+amet+luctus+ac%2C+porta+ac+libero.+Quisque+auctor+turpis+dolor%2C+at+vulputate+massa+sodales+eu.+Proin+et+nulla+cursus%2C+pulvinar+nisl+sed%2C+vehicula+felis.+Sed+blandit%2C+eros+at+lobortis+malesuada%2C+nibh+neque+bibendum+lacus%2C+mattis+cursus+lacus+mi+vehicula+risus.+Aliquam+erat+volutpat.+Nulla+viverra+volutpat+eros%2C+ut+auctor+leo+feugiat+vel.+Cras+sed+felis+sit+amet+sapien+lacinia+tempor+ut+ac+libero.+Donec+non+ex+dapibus%2C+ultricies+libero+interdum%2C+elementum+nulla.+Proin+urna+felis%2C+fermentum+hendrerit+eros+non%2C+placerat+condimentum+felis.+''',
		'response': '''HTTP/1.1 200 Ok
Content-Length: 1143
Content-Type: text/plain

POST /first_request HTTP/1.1
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8
Accept-Encoding: gzip, deflate, br
Accept-Language: en-US,en;q=0.5
Connection: keep-alive
Content-Length: 604
Content-Type: application/x-www-form-urlencoded
Host: 127.0.0.1:49152
Origin: null
Sec-Fetch-Dest: iframe
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: cross-site
Sec-Fetch-User: ?1
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/113.0

'''.replace('\n', '\r\n') + '''data=Phasellus+leo+velit%2C+volutpat+sit+amet+luctus+ac%2C+porta+ac+libero.+Quisque+auctor+turpis+dolor%2C+at+vulputate+massa+sodales+eu.+Proin+et+nulla+cursus%2C+pulvinar+nisl+sed%2C+vehicula+felis.+Sed+blandit%2C+eros+at+lobortis+malesuada%2C+nibh+neque+bibendum+lacus%2C+mattis+cursus+lacus+mi+vehicula+risus.+Aliquam+erat+volutpat.+Nulla+viverra+volutpat+eros%2C+ut+auctor+leo+feugiat+vel.+Cras+sed+felis+sit+amet+sapien+lacinia+tempor+ut+ac+libero.+Donec+non+ex+dapibus%2C+ultricies+libero+interdum%2C+elementum+nulla.+Proin+urna+felis%2C+fermentum+hendrerit+eros+non%2C+placerat+condimentum+felis.+'''}
	,
		{'request':'''POST /second_request HTTP/1.1
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8
Accept-Encoding: gzip, deflate, br
Accept-Language: en-US,en;q=0.5
Connection: keep-alive
Content-Length: 376
Content-Type: application/x-www-form-urlencoded
Host: 127.0.0.1:49152
Origin: null
Sec-Fetch-Dest: iframe
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: cross-site
Sec-Fetch-User: ?1
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/113.0

'''.replace('\n', '\r\n') + '''data=Aliquam+nec+sem+odio.+Curabitur+scelerisque+a+nisi+quis+maximus.+Cras+nisl+ex%2C+posuere+non+lorem+at%2C+finibus+commodo+purus.+Vestibulum+non+orci+ac+ligula+pellentesque+tincidunt.+Praesent+vel+aliquet+metus.+Donec+elementum%2C+elit+at+finibus+dictum%2C+metus+velit+condimentum+nibh%2C+a+imperdiet+odio+lacus+sit+amet+eros.+Donec+convallis+sit+amet+urna+vitae+lobortis.+''',
		'response':'''HTTP/1.1 200 Ok
Content-Length: 916
Content-Type: text/plain

POST /second_request HTTP/1.1
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8
Accept-Encoding: gzip, deflate, br
Accept-Language: en-US,en;q=0.5
Connection: keep-alive
Content-Length: 376
Content-Type: application/x-www-form-urlencoded
Host: 127.0.0.1:49152
Origin: null
Sec-Fetch-Dest: iframe
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: cross-site
Sec-Fetch-User: ?1
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/113.0

'''.replace('\n', '\r\n') + '''data=Aliquam+nec+sem+odio.+Curabitur+scelerisque+a+nisi+quis+maximus.+Cras+nisl+ex%2C+posuere+non+lorem+at%2C+finibus+commodo+purus.+Vestibulum+non+orci+ac+ligula+pellentesque+tincidunt.+Praesent+vel+aliquet+metus.+Donec+elementum%2C+elit+at+finibus+dictum%2C+metus+velit+condimentum+nibh%2C+a+imperdiet+odio+lacus+sit+amet+eros.+Donec+convallis+sit+amet+urna+vitae+lobortis.+'''}
]	
	with socket.create_connection(('127.0.0.1', port)) as connection:
		with connection.makefile('rwb') as connfile:
			k = 0
			for item in data:
				request = item['request']
				response = item['response']
				connfile.write(str.encode(request))
				connfile.flush()
				result = connfile.read(len(response))
				k = k + 1
				if response != result.decode():
					print('Request %d: {%s}'%(k, request))
					print('Unexpected response: {%s}'%result.decode())
					print('Expected: {%s}'%response)
					return 1
	return 0

def talk_adm(port):
	with socket.create_connection(('127.0.0.1', port)) as connection:
		connection.sendall(str.encode('shutdown'))

def compile(args):
	# TODO: Compute  path to executable based on location of this file
	executable = args['build_info']['target_dir'] + '/bin/http_echo'
	target = args['targets'][0]
	exit_status = 0
	with subprocess.Popen(executable, stdout = subprocess.PIPE) as server_proc:
		ports = get_ports(io.TextIOWrapper(server_proc.stdout, encoding="utf-8"))
		server_proc.stdout.close()
		if talk_http(ports['http']) != 0:
			exit_status = 1
		talk_adm(ports['adm'])
		return exit_status
	with open(target, 'w') as result:
		print('Testcase passed')

if sys.argv[1] == 'compile':
	exit(compile(json.loads(sys.argv[2])))
