import socket
import sys
import time

TEST="ABCDEFGHIJKLMNOPRSTUVWXYZ"

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

# Bind the socket to the address given on the command line
server_name = sys.argv[1]
try:
  test = sys.argv[2] == "test"
except:
  test = False
f = None
if not test:
  f = open('/tmp/out.bin','w')

server_address = (server_name, 6677)
print >>sys.stderr, 'starting up on %s port %s' % server_address
sock.bind(server_address)
sock.listen(1)
sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

start = None
while True:
  print >>sys.stderr, 'waiting for a connection'
  connection, client_address = sock.accept()
  print >>sys.stderr, 'client connected:', client_address
  
  try:
    if test:
      index = 0
      cycles = 0
      errors = 0
      while True:
        err = False
        data = connection.recv(9999)
        if data:
          if start is None:
            start = time.time();
          for c in data:
            if c == 'A':
              cycles += 1
            needed = TEST[index%(len(TEST))]
            #print "%s %s"%(c,needed)
            if c != needed:
              err = True
              break
            index += 1
          if err:
            #print "%d cycles"%cycles
            #print "<%s> != <%s>"%(c,needed)
            #print "<%s> != <%s>"%(c.encode('hex'),needed.encode('hex'))
            connection.sendall("-")
            #print data
            errors += 1         
          else:
            connection.sendall("+")
          end = time.time()
          #if (index % 10) == 0:
          t = end-start
          print "%s byte %s seconds => %fB/s (%s %% errors)"%(index, t, (float(index) / float(t)), float(errors)/float(index))
    else:
      while True:
        data = connection.recv(9999)
        f.write(data)
        connection.sendall("+")

  finally:
      if f is not None:
        f.close()
      connection.close()