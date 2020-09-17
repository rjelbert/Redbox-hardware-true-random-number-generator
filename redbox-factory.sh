#!/bin/bash
# todo:
# upload good entropy files to long term storage (s3 / external ssd?)
# split data capture and testing so testing is not taking away from capture time

echo "Setting up the Redbox TRNG serial port..."
# doing this twice because sometimes the speed is not set 1st time
stty raw -echo -ixoff -F /dev/ttyUSB0 speed 115200
sleep 5
stty raw -echo -ixoff -F /dev/ttyUSB0 speed 115200
sleep 5

# check the audit file - if not present then create and write the csv headers
if [ ! -f "./redbox.audit" ]
then
    echo "filename,hash,start_capture,end_capture,end_tests,passed_count,weak_count,failed_count,status" > ./redbox.audit
fi

# make the datasave directories
mkdir -p ./passed
mkdir -p ./failed

# just check we can pull 10 bytes off the TRNG on ttyUSB0 - exit on error, or stall if quiet
while  cat /dev/ttyUSB0 | head -c 10 | wc -c ;
do
	echo "Getting entropy data from Redbox TRNG device..."
	start_capture=$( date '+%F_%H-%M-%S' )
	echo $start_capture
	cat /dev/ttyUSB0 | rngtest -p | head -c 4200000000 > ./$start_capture'.random'
	end_capture=$( date '+%F_%H-%M-%S' )
	echo $end_capture
	echo "Got data, checking entropy quality..."
	# 114 tests in total on default dieharder settings
	cat ./$start_capture'.random' | ent > ./$start_capture'.tests'
	cat ./$start_capture'.random' | rngtest >> ./$start_capture'.tests' 2>> ./$start_capture'.tests'
	dieharder -g 201 -f ./$start_capture'.random' -a -c ' ' >> ./$start_capture'.tests' 2> /dev/null
	end_tests=$( date '+%F_%H-%M-%S' )
	echo $end_tests
	echo "Tests complete, checking results..."
	failed_count=$(grep -o FAILED ./$start_capture'.tests' | wc -l)
	passed_count=$(grep -o PASSED ./$start_capture'.tests' | wc -l)
	weak_count=$(grep -o WEAK ./$start_capture'.tests' | wc -l)

	if [ $failed_count -eq 0 ] && [ $passed_count -gt 110 ]
	then
		echo "Tests passed, saving file!"
		check_sum=$( sha256sum ./$start_capture'.random' )
        echo sha256: $check_sum >> $start_capture'.tests'
        IFS=' '
        read -a strarr <<< "$check_sum"
        just_checksum_code=${strarr[0]}

		echo $start_capture'.random',$just_checksum_code,$start_capture,$end_capture,$end_tests,$passed_count,$weak_count,$failed_count,'PASSED' >> ./redbox.audit

		mv ./$start_capture'.random' ./passed
		mv ./$start_capture'.tests' ./passed
	else
		echo "Tests failed, starting again!"
		# don't bother generating a hash on failed files
		#check_sum=$( sha256sum ./$start_capture'.random' )
		#echo sha256: $check_sum >> $start_capture'.tests'
		#IFS=' '
		#read -a strarr <<< "$check_sum"
		#just_checksum_code=${strarr[0]}
		just_checksum_code=''

		echo $start_capture'.random',$just_checksum_code,$start_capture,$end_capture,$end_tests,$passed_count,$weak_count,$failed_count,'FAILED' >> ./redbox.audit

		mv ./$start_capture'.random' ./failed
		mv ./$start_capture'.tests' ./failed
	fi
done
