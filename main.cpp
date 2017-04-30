#include <args.hxx>
#include <serial/serial.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

using namespace serial;


unsigned char hex2n(char hex)
{
	if (hex >= '0' && hex <= '9') {
		return hex - '0';
	}
	else if (hex >= 'A' && hex <= 'F') {
		return hex - 'A' + 10;
	}
	else if (hex >= 'a' && hex <= 'f') {
		return hex - 'a' + 10;
	}
	return 0;
}

unsigned char *put(unsigned char *ptr, unsigned int data, int c, unsigned char *end) {
	if (c == 1 || c == 2) {
		if( ptr == end ) return NULL;
		*ptr++ = data & 0xFF;
	}
	else if (c == 3 || c == 4) {
		if( ptr == end ) return NULL;
		*ptr++ = data & 0xFF;
		if( ptr == end ) return NULL;
		*ptr++ = (data >> 8) & 0xFF;
	}
	else if (c == 5 || c == 6) {
		if( ptr == end ) return NULL;
		*ptr++ = data & 0xFF;
		if( ptr == end ) return NULL;
		*ptr++ = (data >> 8) & 0xFF;
		if( ptr == end ) return NULL;
		*ptr++ = (data >> 16) & 0xFF;
	}
	else if (c == 7 || c == 8) {
		if( ptr == end ) return NULL;
		*ptr++ = data & 0xFF;
		if( ptr == end ) return NULL;
		*ptr++ = (data >> 8) & 0xFF;
		if( ptr == end ) return NULL;
		*ptr++ = (data >> 16) & 0xFF;
		if( ptr == end ) return NULL;
		*ptr++ = (data >> 24) & 0xFF;
	}
	return ptr;
}


unsigned int hex2data(const char *hex, unsigned char *buf, unsigned int len)
{
	int c = 0;
	unsigned int data = 0;
	unsigned char *ptr = buf;

	while (*hex) {
		if (*hex == ' ') {
			if (c == 0) {
				hex++;
				continue;
			}
			ptr = put(ptr, data, c, buf + len);
			if( !ptr ) return 0;
			data = 0;
			c = 0;
			hex++;
			continue;
		}
		unsigned char n = hex2n(*hex);

		data <<= 4;
		data |= n;

		c++;

		if (c == 8) {
			ptr = put(ptr, data, 8, buf + len);
			if( !ptr ) return 0;
			data = 0;
			c = 0;
		}

		hex++;
	}
	if (c) {
		ptr = put(ptr, data, c, buf + len);
		if( !ptr ) return 0;
	}

return ptr - buf;
}

int main(int argc, char **argv)
{
	const std::vector<std::string> args(argv + 1, argv + argc);

	args::ArgumentParser parser("", "This goes after the options.");

	args::Positional<std::string> port(parser, "Port", "port");

	args::ValueFlag<int> baudrate(parser, "BaudRate", "baudrate", { "br" }, 9600);

	std::unordered_map<std::string, parity_t> parity_map{
		{"none", parity_none},
		{"odd", parity_odd},
		{"even", parity_even},
		{"mark", parity_mark},
		{"space", parity_space},
		{"N", parity_none},
		{"O", parity_odd},
		{"E", parity_even},
		{"M", parity_mark},
		{"S", parity_space}
	};
	args::MapFlag<std::string, parity_t> parity(parser, "Parity", "parity [none, odd, even, mark, space, N, O, E, M, S]", { 'p' }, parity_map, parity_none);

	std::unordered_map<std::string, stopbits_t> stopbits_map{
		{"one", stopbits_one},
		{"two", stopbits_two},
		{"one_point_five", stopbits_one_point_five},
		{"1", stopbits_one},
		{"2", stopbits_two},
		{"1.5", stopbits_one_point_five},
	};
	args::MapFlag<std::string, stopbits_t> stopbits(parser, "StopBits", "stopbits [one, two, one_point_five, 1, 2, 1.5]", { 's' }, stopbits_map, stopbits_one);


	std::unordered_map<std::string, flowcontrol_t> flowcontrol_map{
		{"none", flowcontrol_none},
		{"hw", flowcontrol_hardware},
		{"sw", flowcontrol_software},
	};
	args::MapFlag<std::string, flowcontrol_t> flowcontrol(parser, "FlowControl", "flowcontrol [none, hw, sw]", { "fc" }, flowcontrol_map, flowcontrol_none);

	args::Group dataTypeGroup(parser, "Data Mode", args::Group::Validators::AtMostOne);
	args::Flag textType(dataTypeGroup, "t", "text mode", { 't' });
	args::Flag hexType(dataTypeGroup, "h", "hex mode", { 'h' });
	args::Flag binType(dataTypeGroup, "b", "binary mode", { 'b' });

	args::Group dataSourceGroup(parser, "Data Source", args::Group::Validators::Xor);
	args::Positional<std::string> data(dataSourceGroup, "Data", "input data");
	args::ValueFlag<std::string> file(dataSourceGroup, "File", "data file path", { 'f',"file" });

	try {
		parser.ParseArgs(args);

		serial::Serial s(args::get(port), args::get(baudrate), Timeout(), serial::eightbits, args::get(parity), args::get(stopbits));
		if (!s.isOpen()) {
			std::cerr << "open port failed!" << std::endl;
			exit(-1);
		}

		if (!args::get(data).empty()) {
			if (args::get(textType) || args::get(binType) || !args::get(hexType)) {
				s.write(args::get(data));
			}
			else if (args::get(hexType)) {
				unsigned char buf[1024];
				unsigned int n = hex2data(args::get(data).c_str(), buf, sizeof(buf));
				if (!n) {
					std::cerr << "out of memory" << std::endl;
					return -1;
				}
				s.write(buf, n);
			}
		}
		else {
			std::fstream f;
			f.open(args::get(file), std::ios::in);
			if (!f.is_open()) {
				std::cerr << "open file error!" << std::endl;
				exit(-1);
			}
			char line[1024];
			if (args::get(textType) || (!args::get(binType) && !args::get(hexType))) {
				while (!f.eof()) {
					f.getline(line, 1024, '\n');
					s.write(line);
				}
			}
			else if (args::get(binType)) {
				f.close();
				f.open(args::get(file), std::ios::in | std::ios::binary);
				std::basic_filebuf<char> *pbuf = f.rdbuf();
				std::streamsize size = 1024;
				char *buf = new char[1024];
				while (pbuf->sgetc() != std::char_traits<char>::eof()) {
					std::streamsize n = pbuf->in_avail();
					std::cout << "n=" << n << std::endl;
					if( n > size ){
						delete buf;
						buf = new char[(size_t)n];
						size = n;
					}
					pbuf->sgetn(buf, n);
					s.write((unsigned char *)buf, (size_t)n);
					std::this_thread::sleep_for(std::chrono::milliseconds(100)); //not received second buffer
				}
			}
			else if( args::get(hexType) ){
				unsigned char buf[2048];

				while (!f.eof()) {
					f.getline(line, 1024, '\n');
					unsigned int n = hex2data(line, buf, sizeof(buf));
					if( !n ){
						std::cerr << "out of memory" <<std::endl;
						return -1;
					}
					s.write(buf, n);
				}

			}
			f.close();
		}

		s.flush();
		s.close();
	}
	catch (args::Help e) {
		std::cout << parser;
	}
	catch (args::Error e) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return -1;
	}
	catch (serial::PortNotOpenedException e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}
	catch (serial::IOException e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}
	catch (std::exception e){
		std::cerr << e.what() << std::endl;
		return -1;
	}
	catch (...) {
		std::cerr << "unknown error" << std::endl;
		return -1;
	}
	
	return 0;
}