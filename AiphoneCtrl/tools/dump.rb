#!/usr/bin/ruby -Ku
#
# dump aiphone message.
#
require 'time'
STDOUT.sync = true
while !STDIN.eof?
  s = ""
  while IO.select([STDIN],[],[],  0.03)
    s += STDIN.read(1)
  end
  break if s.length == 0
  msg = s.bytes
  typ = "----"
  len = 0
  if msg[0] == 0x50 || msg[0] == 0xd0
    typ = "ACK "
  elsif msg[0] == 0x60 || msg[0] == 0xe0
    typ = "NAK "
  elsif (msg[0] == 0x40 || msg[0] == 0xc0) && msg.length >= 4
    len = msg[2] & 0x03
    if len == 3
      len = msg[3] + 1
    end
    if msg.length > 3 && msg[2] == 0x68
      typ = "PING"
    elsif msg.length > 3 && msg[2] == 0x69
      typ = "PONG"
    elsif msg.length > 3 && msg[2] == 0x05
      typ = "CALL"
    elsif msg.length > 3 && msg[2] == 0x45 && msg[3] == 0x8c
      typ = "-OPN"
    elsif msg.length > 3 && msg[2] == 0x45 && msg[3] == 0x8f
      typ = "-STA"
    elsif msg.length > 3 && msg[2] == 0x45 && msg[3] == 0x0f
      typ = "-END"
    elsif msg.length > 3 && msg[2] == 0x45
      typ = "COMM"
    elsif msg.length > 3 && msg[2] == 0x55
      typ = "--OP"
    elsif msg.length > 3 && msg[2] == 0x1c
      typ = "DOWN"
    elsif msg.length > 3 && msg[2] == 0x87
      typ = "STAT"
    elsif msg.length > 3 && msg[2] == 0x00
      typ = "TERM"
    end
  end
  puts typ + " " + Time.now().strftime('%Y-%m-%d %H:%M:%S.%3N') + ": " + msg.map{|x| '%02x' %  x}.join(" ") + "  (#{len}"
end

