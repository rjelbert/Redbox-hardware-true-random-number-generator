# Redbox-true-random-number-generator

INTRO

My true random number generator project using an Arduino and 4 reversed biased transistors to generate noise...
I started this project thinking how hard can it be to create a hardware based true random number generator. That was three 
months back. It turns out that getting reasonable volumes of true random numbers is actually quite difficult. I'm pleased
to report though that I think I have done it and here is code and design of what I am called The Redbox Random Number Generator.

OBJECTIVE

When I started out I wanted to have something generate raw 8 byte entropy at 115200 baud over a serial / USB port which is around 10K BYTES a second. This designs meets this criteria. I decided to use 4 entropy sources to generate 4 bits (a nibble) at a time. Two reads make a byte. Most designs on the internet generate a single bit at a time which can then be aggregated to a byte but this is quite slow. The circuit diagram in this repo shows just one white noise source, you will need to make 4 of these to recreate this design. I prototyped on breadboard and then moved to solder board - see the pic.
I also wanted to have a self test mechanism in the design because if a hardware random number generator goes wrong, it can 
often go undetected which is bad. The code does a PSU and noise source check on power up and then every hour. You can change the frequency. With more time, I think it would be better to have a second Arduino nano running the self test code checking PSU and noise source stats continiously but that's for another day.

RESULTS

It works. If I generate 4.2GB of random data, taking around 4 days, the resulting file passes all of the Dieharder tests for randomness. I have included these test result files because you get a real sense of achievment when it finally happens. It also gets excellent results from "ent" and "rngtest" also included in this repo.

LESSONS

Generating random numbers is hard. The hardware design uses 4 reverse biased NPN transistors in 4 individual noise generators. These are fed into a schmitt inverters to digitise the signal and then a divide by 2 flip flop removes any 1 / 0 bias. The problem is if you read the sources too quickly then you get a drop in entropy and increased correleration. I have used a few tricks to improve the randomness and still read the noise sources quite quickly as you can see in the code. The first design outputted the raw entropy from the flipflop inputs to the serial port. This worked OK but even after generating 4GB of data some of the Dieharder tests were failing. I decided to whiten the hardware random numbers by xoring them with two LFSRs (Linear Feedback Shift Registers). First with an 8 bit maximal length LFSR and then again with a maximal length 16 bit LFSR. When the LFSRs finish their sequences they are both re-seeded with hardware randomness. It works very well. I encourage you to switch off the LFSRs to see the outputs and run your own tests on the raw source.

