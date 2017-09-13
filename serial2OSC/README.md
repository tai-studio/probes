# serial2OSC

*2014 -- 2017, Till Bovermann, http://tai-studio.org*

command-line ruby tool to read data from a serial interface and forward it via OSC

### Preparation

This ruby script depends on the following gems:

     trollop serialport ruby-osc json

you can see if they're there by executing

     $ gem list --local

The easiest way to install them (not system-wide) is to use [rvm](http://rvm.io/rvm/install). An `.rvmrc` was added for easy setup.


Wether or not you use rvm, you have to install the gems via

	$ gem install trollop serialport ruby-osc json


### Usage

the command

     $ ruby serial2OSC --help
	 
shows all available options.

### Changes

+ made for DEIND project
+ 2014: added generic interface to transmit comma-separated Integers and floats prefixed with single chars, JSON file parsing
+ 2017: "forked" (copied) from 3DMIN
+ 2017: added `c_setn` interface

Enjoy!
