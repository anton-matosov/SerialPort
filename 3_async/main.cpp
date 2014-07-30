
#include <iostream>
#include <boost/thread.hpp>

#include "BufferedAsyncSerial.h"

using namespace std;
using namespace boost;

int main(int argc, char* argv[])
{
	try {
		cout << "Start\n";

		BufferedAsyncSerial serial("/dev/tty.SLAB_USBtoUART", 9600);

		//Return immediately. String is written *after* the function returns,
		//in a separate thread.
		//serial.writeString("Hello world\r\n");

		//Simulate doing something else while the serial device replies.
		//When the serial device replies, the second thread stores the received
		//data in a buffer.
		std::this_thread::sleep_for(std::chrono::seconds(2));

		//Always returns immediately. If the terminator \r\n has not yet
		//arrived, returns an empty string.
		cout << serial.readStringUntil("\r")<<endl;
		cout << serial.readStringUntil("\r")<<endl;
		cout << serial.readStringUntil("\r")<<endl;
		cout << serial.readStringUntil("\r")<<endl;

		serial.close();

		cout << "End\n";
	}
	catch(boost::system::system_error& e)
	{
		cout<<"Error: "<<e.what()<<endl;
		return 1;
	}
}
