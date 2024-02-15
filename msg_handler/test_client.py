import argparse
import cmd
import socket
import threading
import generated.message_pb2 as pb


class App(cmd.Cmd):
	intro = 'Type help or ? to list commands.\n'
	prompt = '> '

	def __init__(self):
		cmd.Cmd.__init__(self)

		self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self._sock.settimeout(1)

		self._args = self._init_args()

		self._work = 1
		self._thr = threading.Thread(target = self._thread_method)
		self._thr.start()

	def _init_args(self):
		parser = argparse.ArgumentParser()
		parser.add_argument('-a', '--addr', default='127.0.0.1', help='Address to connect')
		parser.add_argument('-p', '--port', type=int, default=10123, help='Port to connect')
		parser.add_argument('-ar', '--auto_pong_reply', action='store_true', help='Automatically reply pong on ping message')
		return parser.parse_args()

	def cleanup(self):
		self._work = 0
		self._thr.join()

	def _thread_method(self):
		while self._work :
			try:
				data = self._sock.recvfrom(1024)
			except TimeoutError:
				continue
			msg = pb.Message()
			try:
				msg.ParseFromString(data[0])
			except Exception as e:
				print(f'[ERROR] wrong message {data}: {e}')
				continue
			print(f'Got message {msg} from {data[1]}')
			self._try_reply(msg)

	def _try_reply(self, msg):
		if msg.type == pb.PING and self._args.auto_pong_reply:
			self._send(pb.PONG)
			print(f'Sent autopong')

	def _send(self, type_):
		msg = pb.Message()
		msg.type = type_
		self._sock.sendto(msg.SerializeToString(), (self._args.addr, self._args.port))

	def do_q(self, arg):
		'Exit'
		print('bye!')
		return True

	def do_c(self, arg):
		'Send connect'
		self._send(pb.CONNECT)

	def do_d(self, arg):
		'Send disconnect'
		self._send(pb.DISCONNECT)

	def do_p(self, arg):
		'Send pong'
		self._send(pb.PONG)

	def do_g(self, arg):
		'Send get dev info'
		self._send(pb.GET_DEV_INFO)


if __name__ == '__main__':
	app = App()
	try:
		app.cmdloop()
	except KeyboardInterrupt:
		pass
	finally:
		app.cleanup()
