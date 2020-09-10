# Redbox-hardware-true-random-number-generator

INTRO

This is my true random number generator (TRNG) project using an Arduino Nano and 4 reversed biased transistors to generate noise...
I started this project thinking "how hard can it be to create a hardware based true random number generator?". That was three 
months back. It turns out that getting reasonable volumes of true random numbers is actually quite difficult. I'm pleased
to report though that I think I have done it and here is code and design of what I am called my Redbox Hardware Random Number Generator. Why Redbox? Because its in a Redbox.

OBJECTIVE

When I started out I wanted to have something generate raw 8 byte entropy at 115200 baud over a serial / USB port which is around 12K BYTES a second. This designs meets my criteria. I decided to use 4 entropy sources to generate 4 bits (a nibble) at a time. Two reads make a byte. Most designs on the internet generate a single bit at a time which can then be aggregated to a byte but this is quite slow. The circuit diagram in this repo shows just one white noise source, you will need to make 4 of these to recreate this design. I prototyped on breadboard and then moved to solder board - see the pic.
I also wanted to have a self test mechanism in the design because if a hardware random number generator goes wrong, it can 
often go undetected which is bad. The code does a PSU and noise source check on power up and then every hour. You can change the frequency. With more time, I think it would be better to have a second Arduino nano running the self test code checking PSU and noise source stats continiously but that's for another day.

RESULTS

It works. If I generate 4.2GB of random data, taking around 4 days, the resulting file passes all of the Dieharder tests for randomness. You will occasionally get WEAK results on some tests which is normal. I have included these test result files as proof and because you get a real sense of achievment when it finally happens. It also gets excellent results from "ent" and "rngtest". I have also included a png of the random data using a heat map type colour conversion of the 0 to 255 random values.

LESSONS

Generating true random numbers is hard. This hardware design uses 4 reverse biased NPN transistors as 4 individual noise generators. These are fed into a schmitt inverters to digitise the signal and then a divide by 2 flip flop removes any 1 / 0 bias. The problem is if you read the sources too quickly then you get a drop in entropy and increased correleration. I have used a few tricks to improve the randomness and still read the noise sources quite quickly (as you will see in the code). The first design outputted the raw entropy from the flipflop inputs to the serial port. This worked OK but even after generating 4GB of data some of the Dieharder tests were still WEAK. I decided to whiten the hardware random numbers by xoring them with two LFSRs (Linear Feedback Shift Registers). First with an 8 bit maximal length LFSR and then again with a maximal length 16 bit LFSR. When the LFSRs finish their sequences they are both re-seeded with hardware randomness. It works very well and to clarify, every single byte of output from Redbox starts off as 8 bits of hardware entropy before being xored with the two LFSRs so this is not an LFSR based generator! I encourage you to switch off the LFSRs to see the outputs and run your own tests on the raw source.

SETUP

I built the circult into a red metal box (it's in the name). This is to screen the noise generator side from external noise sources that might cause randomness tests to fail. The Arduino Nano is powered from the USB port and 5v from the Arduino is used to power the 5v 74 logic components. A separate 12v PSU is needed to power the noise generators and this needs to be good quality or the self tests will fail because either the voltage will be too high or low OR there will be too much noise on it. I do recommend using capacitors accross the 12v rail close to the PCB but that might not be enough. I put a voltage divider on the 12v line of 10K and 2K2 and put this into an analogue input to monitor 12v PSU status. Add a 4.7v Zenor accross the 2K2 resistor to protect the analogue input if you like.

Looking inside the box, you'll see that the Arduino is held down by a white 3D printed part. I have included the OpenSCAD and stl files in this repo for that part. I drilled a hole through the side of the box and through this part to secure it with a screw and bolt.

I've written a calibration mode into the code. Power the unit up by plugging in the USB with the 12v disconnected. Monitor the serial port with a serial terminal and when in calibration mode plug the 12v back in. This should trigger calibration mode which shows you the PSU readings and also the 4 noise source "counts" within a 1 second window. The noise sources should tip just over 9000 - adjust the 4 pots in turn to achieve this. Be careful not to overdrive the noise generating transistors by winding the pots too far to +12v. I put the reversed biased transistors into sockets so they can be easily changed in the future. It might be better to have a button for calibration mode but that is something else for a future version.

The self test runs on power up and every hour. You'll see the green light flash error codes these mean:

1 flash - noise channel 1 low or no bit generation. If low, adjust pot or replace transistor.

2 flashes - noise channel 2 low or no bit generation. If low, adjust pot or replace transistor.

3 flashes - noise channel 3 low or no bit generation. If low, adjust pot or replace transistor.

4 flashes - noise channel 4 low or no bit generation. If low, adjust pot or replace transistor.

5 flashes - 12v PSU voltage too high. PSU may be developing a fault.

6 flashes - 12v PSU voltage too low. PSU may be developing a fault (or mains may have dropped).

7 flashes - 12v PSU too noisy. PSU may be developing a fault.

At the moment, the LED flash code stops the generation of any new random data until the Arduino is reset. This allows you to see any errors that might occur while you are away.

CODE

I have tried to include as many comments as possible in the code but the workflow is basically this:
1) set up Arduino pins
2) Do a self test
3) load LFSRs with hardware random data
4) in a loop:

read 64 bytes of hardware random data into a 64 byte memory array / buffer,

xor each buffer entry with the 8bit LFSR clocking it for each byte,

xor each buffer entry with the bottom 8 bits of the 16bit LFSR clocking it for each byte,

write the 64 byte buffer into the 64 byte UART buffer

5) every hour run the self test

The screen shot of the oscilliscope in this repo shows Redbox running. The top trace is the Arduino TX line on serial out showing the random data being sent. The bottom trace shows the Arduino D7 output pin which goes high each time the code enters the dump the 64 byte entropy buffer into the 64 byte UART buffer for loop. You can see this is triggering at 184Hz which means it is transmitting 184 * 64 bytes per second = 12.9 K BYTES per second. Not bad. There is deliberate random jitter added to the timing of the hardware random nibble reads and this shows up as jitter in the time between buffer dumps to the UART - the scope screen shows this jitter too as a ghost trace. 

TESTING

Be patient. This is a reasonably fast generator but you will still have to wait several days to generate 4.2GB which seems to be a good file size to use with Dieharder, ent and rngtest. I write the random data to a file (commands below) and then use the cat command or file options in these tests to check the results. If you have multiple terminal windows open you can generate the file on one terminal and then run the ent tests on the file as it is being written. Linux seems to handle this well and it means you don't have to wait until the end before you can test.

I tend to test by running ent first and most of the time and then rngtest as a backup check to see how many errors are being thrown up. I then run Dieharder after generating about 100MB but you should not expect perfect results until you get nearer 4GB of data.

OPERATION

To use Redbox, you need to plug it into a computer into its USB 2 or 3 port. 

After you plug in the Arduino or upload a new program to it, you must initialise the USB with:

(adjust the port / file references as necessary - assumes Linux)

stty raw -echo -ixoff -F /dev/ttyUSB1 speed 115200

Then start to write the 4.2GB random data to a file:

head -c 4200000000 /dev/ttyUSB1 > /media/pi/usbdisk/redbox.bin

You might also want to use rngtest in pipe mode to reject poor random data like this:

cat /dev/ttyUSB0 | rngtest -p > /home/pi/redbox-random-rngtestp.bin

After a short while, run "ent" to test the data file:

cat /media/pi/usbdisk/redbox.bin | ent

You can also run rngtest:

cat /media/pi/usbdisk/redbox.bin | rngtest

When you have at least 100MB of random data (preferrably >4GB) you can start the Dieharder tests:

dieharder -g 201 -f /media/pi/usbdisk/redbox.bin -a

If you want to see the first 2MB or so of the random file as a png use my col viz script:

python3 col-randvis.py /media/pi/usbdisk/redbox.bin

when finished:

eog /media/pi/usbdisk/redbox-col.png



