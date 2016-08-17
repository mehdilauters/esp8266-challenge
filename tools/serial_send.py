import serial, sys



#while True:
    #port.write("\r\nSay something:")
    #rcv = port.read(10)
    #port.write("\r\nYou sent:" + repr(rcv))
    
BUFFER_SIZE = 1024

serial_port = sys.argv[1]
filename = sys.argv[2]
to_send = ''
total_size = 0
sent_size = 0


port = serial.Serial(serial_port, baudrate=115200, timeout=3.0)

with open(filename, 'r') as content_file:
  to_send = content_file.read()
  print len(to_send)
  total_size = len(to_send)
    
  while sent_size < total_size:
    sending = min(BUFFER_SIZE, total_size - sent_size)
    port.write(to_send[sent_size:sent_size+sending])
    sent_size += sending
    read = port.read(1)
    if len(read) > 0:
      if read[0] != '+':
        print "Error, received %s"%read
        break
      port.write('+')