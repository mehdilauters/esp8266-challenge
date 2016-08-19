import serial, sys



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


port = serial.Serial(serial_port, baudrate=115200, timeout=3.0)

def wait():
  status = -1
  while status != '+':
    read = port.read(1)
    if len(read) > 0:
      sys.stdout.write(read)
      for c in read:
        if c == '+':
          return

with open(filename, 'r') as content_file:
  to_send = content_file.read()
  print "total len %s"%len(to_send)
  total_size = len(to_send)
    
  while sent_size < total_size:
    sending = min(BUFFER_SIZE, total_size - sent_size)
    print "sending %s"%sending
    port.write(to_send[sent_size:sent_size+sending])
    sent_size += sending
    wait()
    print "wait done"
  print "Done"
print "exit"