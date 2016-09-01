import serial, sys
import time



#while True:
    #port.write("\r\nSay something:")
    #rcv = port.read(10)
    #port.write("\r\nYou sent:" + repr(rcv))
    
BUFFER_SIZE = 256

serial_port = sys.argv[1]
filename = sys.argv[2]
to_send = ''
total_size = 0
sent_size = 0

total_wait_time = 0
total_time = 0

port = serial.Serial(serial_port, baudrate=921600, timeout=3.0)

def wait():
  global total_wait_time
  start = time.time();
  status = -1
  while status != '+':
    read = port.read(1)
    if len(read) > 0:
      sys.stdout.write(read)
      for c in read:
        if c == '+':
          end = time.time()
          t = end-start
          total_wait_time += t
          return

with open(filename, 'r') as content_file:
  to_send = content_file.read()
  print "total len %s"%len(to_send)
  total_size = len(to_send)

  start = time.time();
  while sent_size < total_size:
    sending = min(BUFFER_SIZE, total_size - sent_size)
    print "sending %s"%sending
    port.write(to_send[sent_size:sent_size+sending])
    sent_size += sending
    wait()
    print "wait done"
  end = time.time()
  total_time = end-start
  print "Done in %s s, waiting %s s"%(total_time, total_wait_time)
print "exit"