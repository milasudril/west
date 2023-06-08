#!/bin/env python3

#@	{
#@	"target":{"name":"http_echo_test_result.txt"},
#@	"dependencies":[{"ref":"bin/http_echo"}, {"ref":"./long_text.txt", "origin":"project"}]
#@	}

import sys
import json
import subprocess
import io
import socket
import pytest
import time
import psutil
import os
import signal

http_session_data = 	data = [{'request': '''POST /first_request HTTP/1.1
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

def get_cpu_usage(pid):
	time.sleep(2)
	return psutil.Process(pid).cpu_percent(1.0)/100.0

def get_ports(text_src):
	ret = {}
	for line in text_src:
		stripped_line = line.rstrip()
		keyval = stripped_line.split(' ')
		if keyval[0] == 'http' or keyval[0] == 'adm':
			ret[keyval[0]] = int(keyval[1])
		if 'http' in ret and 'adm' in ret:
			return ret

def talk_adm(port):
	with socket.create_connection(('127.0.0.1', port)) as connection:
		connection.sendall(str.encode('shutdown'))

class run_params:
	def __init__(self, args):
		self.executable = args['build_info']['target_dir'] + '/bin/http_echo'
		self.target = args['targets'][0]
		self.source_dir = args['build_info']['source_dir']

def init_test_suite(args):
	pytest.testsuite_args = run_params(args)

class server_process:
	def __init__(self, http_port, pid):
		self.http_port = http_port
		self.pid = pid

def with_server(server_executable, func):
	with subprocess.Popen([server_executable], stdout = subprocess.PIPE) as server_proc:
		ports = get_ports(io.TextIOWrapper(server_proc.stdout, encoding="utf-8"))
		func(server_process(ports['http'], server_proc.pid))
		talk_adm(ports['adm'])
		wait_res = os.waitpid(server_proc.pid, 0)
		assert wait_res[0] == server_proc.pid
		assert os.waitstatus_to_exitcode(wait_res[1]) == 0

@pytest.fixture
def server():
	with subprocess.Popen(pytest.testsuite_args.executable, stdout = subprocess.PIPE) as server_proc:
		ports = get_ports(io.TextIOWrapper(server_proc.stdout, encoding="utf-8"))
		yield server_process(ports['http'], server_proc.pid)
		talk_adm(ports['adm'])
		wait_res = os.waitpid(server_proc.pid, 0)
		assert wait_res[0] == server_proc.pid
		assert os.waitstatus_to_exitcode(wait_res[1]) == 0

def test_process_two_requests_read_response_inbetween(server):
	with socket.create_connection(('127.0.0.1', server.http_port)) as connection:
		with connection.makefile('rwb') as connfile:
			for item in http_session_data:
				request = item['request']
				response = item['response']
				connfile.write(str.encode(request))
				connfile.flush()
				result = connfile.read(len(response))
				assert response == result.decode()
			cpu_usage = get_cpu_usage(server.pid)
			assert cpu_usage < 0.5

def test_process_two_requests_read_response_after_sending_reqs(server):
	with socket.create_connection(('127.0.0.1', server.http_port)) as connection:
		with connection.makefile('rwb') as connfile:
			for item in http_session_data:
				request = item['request']
				connfile.write(str.encode(request))
				connfile.flush()
			for item in http_session_data:
				response = item['response']
				result = connfile.read(len(response))
				assert response == result.decode()

			cpu_usage = get_cpu_usage(server.pid)
			assert cpu_usage < 0.5

def test_process_two_requests_read_only_one_response_after_sending_reqs(server):
	with socket.create_connection(('127.0.0.1', server.http_port)) as connection:
		with connection.makefile('rwb') as connfile:
			for item in http_session_data:
				request = item['request']
				connfile.write(str.encode(request))
				connfile.flush()

			response = http_session_data[0]['response']
			result = connfile.read(len(response))
			assert response == result.decode()

			cpu_usage = get_cpu_usage(server.pid)
			assert cpu_usage < 0.5

def test_read_response_without_request():
	def handler(server):
		pid = os.fork()
		if pid == 0:
			with socket.create_connection(('127.0.0.1', server.http_port)) as connection:
				with connection.makefile('rwb') as connfile:
					response = http_session_data[0]['response']
					result = connfile.read(len(response))
					exit(1)
		else:
			cpu_usage = get_cpu_usage(server.pid)
			assert cpu_usage < 0.5
			os.kill(pid, signal.SIGKILL)
			os.waitpid(pid, 0)

	with_server(pytest.testsuite_args.executable, handler)

def test_process_large_amount_of_data(server):
	with socket.create_connection(('127.0.0.1', server.http_port)) as connection:
		print(pytest.testsuite_args.source_dir + '/integration_test/long_text.txt')
		with open(pytest.testsuite_args.source_dir + '/integration_test/long_text.txt') as f:
			data = f.read()

		request = (''.join([('''GET / HTTP/1.1
Content-Length: %d

'''%len(data)).replace('\n', '\r\n'), data])).encode()
		bytes_left = len(request)
		bytes_written = 0
		while bytes_left > 0:
			bytes_to_write = min(bytes_left, 256*1024)
			bytes_sent = connection.send(request[bytes_written: bytes_written + bytes_to_write])
			assert bytes_sent != 0
			bytes_left = bytes_left - bytes_sent
			bytes_written = bytes_written + bytes_sent
			time.sleep(2.0)

		response = ''.join([('''HTTP/1.1 200 Ok
Content-Length: %d
Content-Type: text/plain

'''%len(request)).replace('\n', '\r\n'),
		request.decode()])
		print('response %d, request %d'%(len(response), len(request)), file=sys.stderr)

		print('Waiting for data', file=sys.stderr)
		time.sleep(1.0)
		print('Start reading', file=sys.stderr)
		bytes_left = len(response)
		resp_recv = []
		while bytes_left > 0:
			bytes_to_read = min(bytes_left, 4096)
			data = connection.recv(bytes_to_read)
			bytes_left = bytes_left - len(data)
			resp_recv.append(data)
		resp_recv = b''.join(resp_recv)
		assert resp_recv.decode() == response

	print('Client closed connection', file=sys.stderr)
		#	time.sleep(1.0/128.0)


def main(argv):
	if sys.argv[1] == 'compile':
		init_test_suite(json.loads(sys.argv[2]))
		return pytest.main(['-s', '--verbose',  '-k', 'test_process_large_amount_of_data', __file__])

if __name__ == '__main__':
	exit(main(sys.argv))

