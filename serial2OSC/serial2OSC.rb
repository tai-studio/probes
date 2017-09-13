#simplest ruby program to read from arduino serial,
#using the SerialPort gem
#(http://rubygems.org/gems/serialport)
 
require "serialport"
require "ruby-osc"
include OSC

require "json"    # json config file parsing

require 'trollop' # commandline parsing

opts = Trollop::options do
  banner <<-eos

serial2OSC -- forward formatted UART (serialport) data via OSC.
  (c) 2014 -- Till Bovermann, 3DMIN.org

Description:
  Each line of the incoming serial stream is considered a message.
  It should have the form
      <k><v1>,<v2>,...

  where 
    <k>   --- first character in the processed line, and 
    <vN>  --- value of type Integer ("I") or Float ("F"). 
  
  There is no limitation to the number of <vN>'s sent in one message. 
  A JSON-formatted config file given as an argument to this program specifies 
    + OSC message name to be generated
    + data type identifier of transfered values ("F", "I")
    + number of dimensions (required for c_setn messages) 

Example JSON-formatted config file:

  {
    "a": ["/pot",  "F", 3],
    "t": ["/temp", "I", 1]
  }

Options:
  eos

  opt :serial_port, "serial port",            :default => "/dev/tty.usbmodem641"
  opt :baud_rate,   "serial port baud rate",  :default => 115200
  opt :addr,        "OSC address",            :default => "localhost"
  opt :port,        "OSC port number",        :default => 57120
  # opt :server_port, "OSC server port number", :default => 7000
  opt :id,          "identifier number / start bus",  :default => 0 
  opt :pConfig,     "protocol config file",  :default => "configExample.json"
  opt :toServer,      "format as SuperCollider server message", :default => false
end

Trollop::die :pConfig, "-- file does not exist" if (not File.exists? opts[:pConfig] )

# parse json file if given
json = File.read(opts[:pConfig])
pConf = JSON.parse(json, :symbolize_names => true)

puts "#{opts[:toServer]}"

i=0;
offsets = Hash.new();

pConf.each { |k, v| 
  # convert type identifiers to symbols
  v[1] = v[1].to_sym

  # compute running offsets
  offsets[k] = i
  i = i + v[2]
}


# serial acquisition and forwarding via osc
sp_data_bits = 8
sp_stop_bits = 1
sp_parity = SerialPort::NONE


osc_client = Client.new opts[:port], opts[:addr] 
sp = SerialPort.new(opts[:serial_port], opts[:baud_rate], sp_data_bits, sp_stop_bits, sp_parity)
puts "opened serial port #{sp}"


serialAcquisitionThread = Thread.new() {
  #just read forever
  while true do
    while (raw = sp.gets.chomp) do
      processLine raw, pConf, offsets, osc_client, opts[:id], opts[:toServer]
    end
    # sleep 0.00002;
  end
}

def processLine(line, pConf, offsets, osc_client, clientId, serverMode)
  # line[0] = line[0].to_sym;
  messageId, type, dim = pConf[line[0].to_sym]
  offset = offsets[line[0].to_sym]
  # ignore unknown types
  if messageId.nil?
    return
  end

  if (type == :F)
    vals = line[1..-1].split(',').collect{|item| item.to_f}
  else
    vals = line[1..-1].split(',').collect{|item| item.to_i}
  end

  if (serverMode)
    # puts "/c_setn, #{clientId + offset}, #{dim}, #{vals.take(dim)}"

    osc_client.send(Message.new("/c_setn", clientId + offset, dim, *(vals.take(dim))))
  else
    osc_client.send(Message.new(messageId, clientId, *vals))
  end
end

# OSC.run do
#   osc_server = Server.new opts[:server_port]
#   osc_server.add_pattern "/rgb" do |*args|
#     sp.write("C")
#     # puts args
#     sp.write(args[1,3].join(','))
#     sp.write("\n")
#   end
# end

# serialAcquisitionThread.join


begin
  puts "type Ctrl-C to quit."
  loop do
    sleep 1
    print "."
  end
rescue SystemExit, Interrupt
  puts "\nclosing serial port..."
  sp.close
  puts "Good bye."
  # raise
end


