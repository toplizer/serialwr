# serialwr
Send bytes to serial port from command line.

### Dependencies
*	[cmake](http://cmake.org)
*	[juliencombattelli/Serial](http://github.com/juliencombattelli/Serial) [included by submodule]
*	[wjwwood/args](http://github.com/wjwwood/args) [included by submodule]
  
  
### Build Command
>	mkdir build  
>	cd build  
>	cmake ..  
>	cmake --build . --config Release
  
  
### Platform
*	Passed with Visual Studio 2015
  
  
### Usage   

*	serialwr com1 "hello!"   
	Send `hello!` to com1 with default baudrate(9600).   
	Same as `serialwr --br 9600 -p N -s 1 --fc none -t com1 "hello!"`.
   
*	serialwr --br 115200 -h com1 "01 02 03 04"   
	Send `0x01 0x02 0x03 0x04` to com1 with baudrate 115200.
   
*	serialwr -h com1 "0102 0304"   
	Send `0x02 0x01 0x04 0x03` to com1 with default baudrate.
   
*	serialwr -h --be com1 "0102 0304"   
	Send `0x01 0x02 0x03 0x04` to com1 with default baudrate.   

*	serialwr -b -f abc.txt com1  
	contents of abc.txt (on Windows):   
	`hello`  
	`world`  
	Send binary bytes from abc.txt(`0x68 0x65 0x6C 0x6C 0x6F 0x0D 0x0A 0x77 0x6F 0x72 0x6C 0x64`) to com1.

