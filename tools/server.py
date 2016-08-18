import socket
import sys


TEST="ABCDEFGHIJKLMNOPRSTUVWXYZ"

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

# Bind the socket to the address given on the command line
server_name = sys.argv[1]
test = sys.argv[2] == "test"
f = None
if not test:
  f = open('/tmp/out.bin','w')

server_address = (server_name, 6677)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.bind(server_address)
sock.listen(1)
sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

while True:
  print >>sys.stderr, 'waiting for a connection'
  connection, client_address = sock.accept()
  print >>sys.stderr, 'client connected:', client_address
  
  try:
    if test:
      index = 0
      while True:
        err = False
        data = connection.recv(9999)
        for c in data:
          needed = TEST[index%(len(TEST))]
          if c != needed:
            err = True
          if err:
            print "%s != %s"%(c,needed)
            connection.sendall("-")
          else:
            connection.sendall("+")
          index += 1
        print ">%s<"%data
    else:
      while True:
        data = connection.recv(9999)
        f.write(data)
        connection.sendall("+")
        print >>sys.stderr, 'received "%s"' % data

  finally:
      if f is not None:
        f.close()
      connection.close()