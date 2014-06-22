#SerialPort
==========

Class extensions for boost::asio::serial_port

Based on [Serial ports and C++](http://www.webalice.it/fede.tft/serial_port/serial_port.html) article by fede.tft on Seven segment website

Forked from [serial-port repository](https://gitorious.org/serial-port).

## License

The wrapper classes are licensed under the boost license. After all, they're just thin wrappers around the asio library.

## Build

Makefiles are deliberately basic to focus on what libraries need to be linked to produce an executable. For Linux Makefiles have been replaced with CMake build files.
Getting the boost libraries

On Linux: With Ubuntu/Kubuntu/Xubuntu Jaunty (9.04)  or newer, open a shell and type
'''
sudo apt-get install libboost-dev libboost-thread-dev
'''

On Windows: Download MinGW-distro [here](http://nuwen.net/mingw.html) that comes with boost precompiled. Don't forget to set the PATH variable as explained in the link.
    
# Original Article

## Introduction

The serial port protocol is one of the most long lived protocols currently in use. According to wikipedia, it has been standadized in 1969. First, a note: here we're talking about the RS232 serial protocol. This note is necessary because there are many other serial protocols, like SPI, I2C, CAN, and even USB and SATA.

Some time ago, when the Internet connections were done using a 56k modem, the serial port was the most common way of connecting a modem to a computer. Now that we have ADSL modems, the serial ports have disappeared from newer computers, but the protocol is still widely used.

In fact, most microcontrollers, even the newer ones have one or more peripherals capable of communicating using this protocol, and from the PC side, all operating systems provide a way of interfacing with serial devices. The problem of the lack of serial ports on computers was solved with USB to serial converters, often embedded into the device itself.

One of such devices is the Arduino. While the first models had a serial port connector, newer models has an USB port. However, nothing has changed on the microcontroller side, nor on the PC side. Simply the newer Arduinos have a chip that performs the serial to USB conversion.

This article explains how to interface with serial ports from C++. Instead of presenting an API specific to a single operating system, the Boost.asio library was used. This library provides portable, high performance implementation of sockets and serial ports.
The chosen presentation is to provide code with a growing level of scalability, and a growing level of complexity. It starts with a simple class to wrap Asio's serial ports to provide string write and read, and expands to topics like binary data transfer, timeouts, and asynchrounous operation. All the example presented here are available for download at the end of the article. This article presents some classes that wrap the asio library. They can be used as-is, or its implementation can be studied to understand how to perform these tasks with asio.

###What can I do with serial ports?
The answer is simple: communicate with devices that have a serial port. These include the already mentioned Arduino, the Fonera, and many different microcontrollers.

#Part 1: A simple example
This is the simplest possible way to use the Boost.asio library. A class SimpleSerial that provides a way to write strings to the serial device, and to read lines from it. The code is enough short to be printed entirely. Here it is:

```
#include <boost/asio.hpp>
class SimpleSerial
{
public:
    /**
     * Constructor.
     * \param port device name, example "/dev/ttyUSB0" or "COM4"
     * \param baud_rate communication speed, example 9600 or 115200
     * \throws boost::system::system_error if cannot open the
     * serial device
     */
    SimpleSerial(std::string port, unsigned int baud_rate)
    : io(), serial(io,port)
    {
        serial.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
    }

    /**
     * Write a string to the serial device.
     * \param s string to write
     * \throws boost::system::system_error on failure
     */
    void writeString(std::string s)
    {
        boost::asio::write(serial,boost::asio::buffer(s.c_str(),s.size()));
    }

    /**
     * Blocks until a line is received from the serial device.
     * Eventual '\n' or '\r\n' characters at the end of the string are removed.
     * \return a string containing the received line
     * \throws boost::system::system_error on failure
     */
    std::string readLine()
    {
        //Reading data char by char, code is optimized for simplicity, not speed
        using namespace boost;
        char c;
        std::string result;
        for(;;)
        {
            asio::read(serial,asio::buffer(&c,1));
            switch(c)
            {
                case '\r':
                    break;
                case '\n':
                    return result;
                default:
                    result+=c;
            }
        }
    }

private:
    boost::asio::io_service io;
    boost::asio::serial_port serial;
};
```

This class has a constructor that accepts two parameters: a string to identify the device name of the serial port, which on Linux is /dev/ttyS0, /dev/ttyS1,... for real serial ports, and /dev/ttyUSB0, /dev/ttyUSB1,... for USB to serial converters. On Windows it is either COM1, COM2, for real serial ports, while it usually starts from COM4 for USB to serial converters. The second parameter is the baud rate, which is the speed in bits per second of the communication. It usually assumes values like 9600, 19200 or 115200. Of course to establish a communication both the serial device and the PC must use the same baud rate.

The class has two member functions: writeString() and readLine(). The writeString() function simply writes the string passed as argument to the serial device, by using the write() function of the asio library. The readLine() function reads a line of text from the serial device, removing the line terminator character. Since lines are terminated either with \n or \r\n, it reads one character at a time using the read() function of the asio library, if it is \r it is discarded, if it is \n it returns with the string read so far, and in all other cases it appends the last read char to the string.  As the comment suggests, reading one character a time is not the most efficient way, but this is the first example, so it is better to keep it simple.
How to use this class? Here is an example main() code:

```
#include <iostream>
#include "SimpleSerial.h"

using namespace std;
using namespace boost;

int main(int argc, char* argv[])
{
    try {

        SimpleSerial serial("/dev/ttyUSB0",115200);

        serial.writeString("Hello world\n");

        cout<<serial.readLine()<<endl;

    } catch(boost::system::system_error& e)
    {
        cout<<"Error: "<<e.what()<<endl;
        return 1;
    }
}
```

The code assumes that the SimpleSerial class presented above is written in the SimpleSerial.h header file.

This code simply creates an instance of the class, writes "Hello world\n" to the serial device, and waits for a reply. When the reply arrives, it is printed to screen. The code is enclosed in a try/catch block since all functions might throw exceptions. This happens for example trying to open a nonexistent device name, or if the serial port cable is disconnected during a write or read operation.
While this code is simple and easy to use, it has a lot of drawbacks:


 - There is no way to read and write binary data. Only text strings are allowed.
 - Reading from the serial port is problematic: if for some reason the serial device does not send any data, the program remains blocked. There is no way to set a timeout to forcefully return if the reply does not arrive in a reasonable time.
 - Reading data one character a time is inefficient.

This does not mean that the class is useless, for simple applications it is perfectly reasonable to use it.

To see how to fix these problems, go on reading.

## Part 2: Timeouts and binary I/O
This part presents a class named TimeoutSerial. It performs all what the previous class does, and much more. However, the code is a bit longer. So, instead of printing it entirely, only the class public interface will be reported. The full code of the class is available for download at the end of this article.

```
class TimeoutSerial: private boost::noncopyable
{
public:
    TimeoutSerial();

    TimeoutSerial(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

    void open(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

    bool isOpen() const;

    void close();

    void setTimeout(const boost::posix_time::time_duration& t);

    void write(const char *data, size_t size);

    void write(const std::vector<char>& data);

    void writeString(const std::string& s);

    void read(char *data, size_t size);

    std::vector<char> read(size_t size);

    std::string readString(size_t size);

    std::string readStringUntil(const std::string& delim="\n");

    ~TimeoutSerial();
};
```

First, don't be scared by how the constructor and the open() member functions are declared. Serial ports can have more options than simply the device name and baud rate. There is the parity option, which is used for error detection, and can be either disabled, odd or even. There is the character size, which is the size of one "packet". There is flow control, that is used to limit write speed with slow devices to avoid buffer overruns, and there is the stop bit option, which specifies if the last bit of one packet should be longer than all the other. All these additional options are used infrequently, and their default values are: no parity, character size of 8bit, no flow control, and 1 stop bit. So these options are used as default parameters for the function declarations. In this way it is possible to specify only the device name and character size, but unfortunately this requires the long and complicated declaration of the constructor and open() functions.

The class also provides a close() function, to stop the communication, and isOpen(), to test if the serial communication is open.

And now we come to the interesting things: the setTimeout() member function. When constructing an instance of this class, the timeout is set to zero. This means that it is disabled. By calling this member function it is possible to set a timeout on read operations. When a timeout is set the operation completes within the specifed timeout, or an exception of type timeout_exception is thrown.
The writeString() member function remained the same as in the previous class. After all, if it works well, why changing it?

The readString() however, has been replaced by two member functions, readString() and readStringUntil().

The first one is good when there is a need to read an exact number of characters, instead of reading up to the end of a line. The second acnowledges that \n is not universally accepted as line terminator. If a different terminator character, or terminator string, like "\r\n" is required, it can be passed as parameter. Again for convenience, if no parameter is specified, it defaults to "\n".

Then there is a set of new member functions, read() and write(), each one with two overloads. These are designed to let you read and write binary data. The need for the two overloads is to allow using either a plain array, or a vector.
Let's see this class in action:

```
#include <iostream>
#include "TimeoutSerial.h"

using namespace std;
using namespace boost;

int main(int argc, char* argv[])
{
    try {
 
        TimeoutSerial serial("/dev/ttyUSB0",115200);
        serial.setTimeout(posix_time::seconds(5));

        //Text test
        serial.writeString("Hello world\n");
        cout<<serial.readStringUntil("\r\n")<<endl;
   
        //Binary test
        char values[]={0xde,0xad,0xbe,0xef};
        serial.write(values,sizeof(values));
        serial.read(values,sizeof(values));
        for(unsigned int i=0;i<sizeof(values);i++)
        {
            cout<<static_cast<int>(values[i])<<endl;
        }

        serial.close();
 
    } catch(boost::system::system_error& e)
    {
        cout<<"Error: "<<e.what()<<endl;
        return 1;
    }
}
```

This shows both text and binary operations. Since the timeout was set to 5 seconds at the beginning of the program, all read operation are performed with the timeout.

This class is, in my opinion al least, already usable in a wide range of applications.
However, it still has one drawback. Write and read operation are synchronous. This means that if you pass a large array of data to write(), this function won't return until all data have been written, which could possibly take a long time. And the issue is even more evident with the read operations. Ok, they have a timeout so that they can't block forever, but they still block until all data has been received (or the timeout expires).
If this is unacceptable in your application, move to the next level with asynchronous I/O.
## Part 3: Asynchronous read and write
Performing asynchronous I/O is more complicated than synchronous I/O; this is both because callbacks come into play, and for the need of multithreading, but it was clear that at the end we'll need to present it, since after all asynchronous I/O is all what Boost.asio is about. The way I've implemented this is more complex too, since it involves an abstract base class, named AsyncSerial, from which concrete classes are derived. The presented concrete class is named BufferedAsyncSerial. Since inheritance is involved, the full list of member functions of BufferdAsyncSerial is split between itself and the base class. So first the base class public interface will be presented, and then the derived class. Here is AsyncSerial:

```
class AsyncSerial: private boost::noncopyable
{
public:
    AsyncSerial();

    AsyncSerial(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

    void open(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

    bool isOpen() const;

    bool errorStatus() const;

    void close();

    void write(const char *data, size_t size);

    void write(const std::vector<char>& data);

    void writeString(const std::string& s);

    virtual ~AsyncSerial()=0;
};
```

And here is the BufferedAsyncSerial class:

```
class BufferedAsyncSerial: public AsyncSerial
{
public:
    BufferedAsyncSerial();

    BufferedAsyncSerial(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

    size_t read(char *data, size_t size);

    std::vector<char> read();

    std::string readString();

    std::string readStringUntil(const std::string delim="\n");

    virtual ~BufferedAsyncSerial();
};
```

At a first glance it does not seem so much different from the TimeoutAsyncSerial class. Apart from being split into two classes, one can find nearly all the member functions of the previous class. But there is a big difference: both read and write operations are asynchronous. When you call a function to write to the serial device, it stores the data passed to it in a buffer, and returns immediately. The read operation is then performed in a separate thread owned by the class itself.

Read operatons act likewise. The always return immediately. To be able to do so the function's behaviour changed: the read(char *data, size_t size) member function no longer guarantees to read all the charactes specified by size. If size is 512, but only 3 charactes have arrived so far, it returns only the 3 received characters. It can even return zero if no character have arrived. If instead more than 512 characters have arrived, it returns only the first 512. The read() that returns a vector or string are even simpler. They return all characters arrived so far, be it zero or any other number.
And lastly, the readStringUntil() member function returns either a whole line or an empty string if the terminator string has not yer arrived.

Other differnces are the lack of setTimeout(), who needs a timeout if all functions return immediately?
and the addition of the errorStatus() member functions. Since the write operations return before data has been sent, there is no way to know in advance if they'll fail, so it is no longer possible to throw an exception from write(). Hence, this function was added to check if some error occurred.
This is an exmple code that uses the BufferedAsyncSerial class:

```
#include <iostream>
#include <boost/thread.hpp>

#include "AsyncSerial.h"

using namespace std;
using namespace boost;

int main(int argc, char* argv[])
{
    try {
        BufferedAsyncSerial serial("/dev/ttyUSB0",115200);

        //Return immediately. String is written *after* the function returns,
        //in a separate thread.
        serial.writeString("Hello world\n");

        //Simulate doing something else while the serial device replies.
        //When the serial device replies, the second thread stores the received
        //data in a buffer.
        this_thread::sleep(posix_time::seconds(2));

        //Always returns immediately. If the terminator \r\n has not yet
        //arrived, returns an empty string.
        cout<<serial.readStringUntil("\r\n")<<endl;

        serial.close();
 
    } catch(boost::system::system_error& e)
    {
        cout<<"Error: "<<e.what()<<endl;
        return 1;
    }
}
```

I think the code requires no further comment.

What, you say you like the asynchronous read and write, but you want more?

Well yes, this class can be further improved. Where's its weak point? It's in the read operations. Before, when reading your program was blocked until the data arrived, but now it has to constantly poll to see if the data arrived. Wouldn't it be great to be notified when data arrives, instead of polling?

Here's the purpose of the last class of this article: the CallbackAsyncSerial.

## Part 4: Callbacks
This is the last class presented in this article, CallbackAsyncSerial. Contrary to all classes presented until now, this exposes its multithreaded implementation, and might require using mutexes in the code that uses it. This class, just like BufferedAsyncSerial derives from AsyncSerial, so all the member functions that were in AsyncSerial work the same way in this class. This includes the code to open/close and write data to the serial port. What changes is the reading part.

Here is the public inteface of the CallbackAsyncSerial class:

```
class CallbackAsyncSerial: public AsyncSerial
{
public:
    CallbackAsyncSerial();

    CallbackAsyncSerial(const std::string& devname, unsigned int baud_rate,
        boost::asio::serial_port_base::parity opt_parity=
            boost::asio::serial_port_base::parity(
                boost::asio::serial_port_base::parity::none),
        boost::asio::serial_port_base::character_size opt_csize=
            boost::asio::serial_port_base::character_size(8),
        boost::asio::serial_port_base::flow_control opt_flow=
            boost::asio::serial_port_base::flow_control(
                boost::asio::serial_port_base::flow_control::none),
        boost::asio::serial_port_base::stop_bits opt_stop=
            boost::asio::serial_port_base::stop_bits(
                boost::asio::serial_port_base::stop_bits::one));

    void setCallback(const
            boost::function<void (const char*, size_t)>& callback);

    void clearCallback();

    virtual ~CallbackAsyncSerial();
};
```

This class has the setCallback() member function that allows to register a function to be called when data arrives from the serial port. The function must take an array of characters and a size_t parameter that are the buffer with the data arrived, and the number of characters arrived. Note the use of boost::function instead of a function pointer, that allows for example to bind a member function of another class as callback.
The callback is called as soon as data arrives from the serial port, and is called from a thread owned by the class itself. This is why proper mutexing for synchronization might be required.

Instead of providing a simple example for this class, there is an entire program. It's a simplified version of screen or minicom. These programs open a terminal over a serial port. What you type is sent to the serial device, and what the serial device sends is printed on screen. The use of asynchronous callbacks is perfect for this purpose, since the main thread could be left waiting for characters from the standard input, while the callback from the other thread prints arrived data on screen.

While the CallbackAsyncSerial class is portable, the example program is not, since it includes termios.h which is available only on POSIX platforms, therefore not windows. This dependency is used both to disable automatic program termination when typing Ctrl-C, since one might need to send a Ctrl-C to the serial device, to disable echo of characters typed on the keyboard, and to allow to get characters from the keyboard one at a time, instead of one line at a time, which is the default. The source code of the program is available for download.
## Part 5: GUI (Qt)
All the above code is mainly designed for programs without a graphical user interface, even if classes like CallbackAsyncSerial work well even with GUIs. In general, GUI programs impose some restrictions on the programming style, requiring the program flow to be "event driven". The last class presented here is QAsyncSerial. It is a wrapper of CallbackAsyncSerial that takes advantage of Qt's signal and slot mechanism to make safe callbacks even across different threads. With this class you can register a Qt slot to be called when data arrives from the serial port in the same way you do it for pushbuttons on a GUI.

For more information about this class, read this post on my blog: http://fedetft.wordpress.com/2010/04/23/qt-and-serial-ports

## Part 6: Iostreams
This is a new class that was added long after the others. By leveraging on Boost.iostreams I made a serial port class that looks like an iostream. Therefore it is possible to read and write data osing operator >> and <<, getline() etc...
Other than that, it's equivalent to a TimeoutSerial class.

This is an example code:

```
#include <iostream>
#include "serialstream.h"

using namespace std;
using namespace boost::posix_time;

int main(int argc, char* argv[])
{
    SerialOptions options;
    options.setDevice("/dev/ttyUSB0");
    options.setBaudrate(115200);
    options.setTimeout(seconds(3));
    SerialStream serial(options);
    serial.exceptions(ios::badbit | ios::failbit); //Important!
    serial<<"Hello world"<<endl;
    string s;
           cout<<s<<endl;
    serial>>s;
    getline(serial,s);
    cout<<s<<endl;
}
```

## License

The wrapper classes are licensed under the boost license. After all, they're just thin wrappers around the asio library.


## References
    The Boost.asio documentation, available here.
    This example code, that was used as a base to write the AsyncSerial class, and for the minicom example.

Developed by TFT

====
### Notes

Written: Sep 14, 2009; last updated Jun 22, 2011. 

Exproted to github: Jun 21, 2014.
