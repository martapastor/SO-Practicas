#!/bin/bash

TMP_DIRECTORY=tmp
OUT_DIRECTORY=out

MYTAR_FILE=mytar.mtar

FILENAME1=file1.txt
FILENAME2=file2.txt
FILENAME3=file3.dat

echo " "
echo " SCRIPT FOR TESTING MYTAR PROGRAM "
echo " Developed by Marcos Calero and Marta Pastor "
echo " "

# First of all, we have to check that the MyTar program is located in the current
# working directory and that we can execute it. If the check fails, print an
# error message and exit.
if [ ! -e ./mytar ]; then
    echo "[ ERROR ] MyTar file has not been found!"
    exit -1
elif [ ! -x ./mytar ]; then
	echo "[ ERROR ] Mytar file is not executable!"
    exit -1
fi

echo " "
echo "[ OK ] MyTar file was found and has execution permisions."
echo " "

# Now, we check if a directory called "tmp" is found in the current working
# directory. If so, we have to remove it and its contents recursively.
if [ -d "$TMP_DIRECTORY" ]; then
	rm -rf -- $TMP_DIRECTORY
	echo "[ OK ] $TMP_DIRECTORY directory has been succesfully deleted."
fi

# We create a temporary directory named "tmp" in the current working directory.
# The script will then switch to that directory with cd.
mkdir $TMP_DIRECTORY
cd $TMP_DIRECTORY

# We create three files inside the tmp folder:
if [ ! -e $FILENAME1 ]; then
    # This file contains "Hello world!" useing the echo command and redirecting
    # its standard output to a file:
	touch $FILENAME1
	echo "Hello World!" > $FILENAME1
  	echo "[ OK ] $FILENAME1 has been succesfully created."
fi

if [ ! -e $FILENAME2 ]; then
    # This file stores a copy of the first 10 lines of the /etc/passwd file
	touch $FILENAME2
	head -10 /etc/passwd > $FILENAME2
  	echo "[ OK ] $FILENAME2 has been succesfully created."
fi

if [ ! -e $FILENAME3 ]; then
	# This file stores 1024 random bytes taken from the special file /dev/urandom
	touch $FILENAME3
	head -c 1024 /dev/urandom > $FILENAME3
  	echo "[ OK ] $FILENAME3 has been succesfully created."
fi

# Now, we invoke the MyTar program to create a fileTar.mtar archive.
./../mytar -c -f $MYTAR_FILE $FILENAME1 $FILENAME2 $FILENAME3

# We also create the output directory within the current working directory and
# we create a copy of the fileTar.mtar file in such directory:
if [ ! -d $OUT_DIRECTORY ]; then
	mkdir $OUT_DIRECTORY
	echo "[ OK ] $OUT_DIRECTORY directory has been succesfully created."
fi

cp ./$MYTAR_FILE ./$OUT_DIRECTORY/$MYTAR_FILE

# We now switch to the out directory and run the MyTar program to extract the
# tarball's contents:
cd $OUT_DIRECTORY
./../../mytar -x -f $MYTAR_FILE

# We use the diff program to compare the contents of both each extracted file
# with the the associated original file found in the parent directory:
if diff ../$FILENAME1 $FILENAME1 >/dev/null ; then
  	echo "[ OK ] $FILENAME1 content are the same in both directories."
else
  	echo "[ ERROR ] $FILENAME1 content are not the same in both directories."
  	exit -1
fi

if diff ../$FILENAME2 $FILENAME2 >/dev/null ; then
  	echo "[ OK ] $FILENAME2 content are the same in both directories."
else
	echo "[ ERROR ] $FILENAME2 content are not the same in both directories."
  	exit -1
fi

if diff ../$FILENAME3 $FILENAME3 >/dev/null ; then
  	echo "[ OK ] $FILENAME3 content are the same in both directories."
else
	echo "[ ERROR ] $FILENAME3 content are not the same in both directories."
	exit -1
fi

# Finally, we switch to the ../.. directory and print "Success" in case the three
# files are the same and terminate the execution (return 0). Otherwise, we also
# switch to the ../.. directory, print a message describing the error and
# terminate the execution (return 1):
echo " "
echo "Test finished: all tests have been succesfully passed."
echo "Success."
echo " "
exit 0
