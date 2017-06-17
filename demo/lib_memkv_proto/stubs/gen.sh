#!/bin/bash
#generate h,cpp files from *.stub
gson -d .

#cp files
for file in ./*
do
	#get file extension
	ext="${file##*.}"
	if [ $ext = "cpp" ]
	then
		cp $file ../src/
		rm $file
	fi
	if [ $ext = "h" ]
	then
		cp $file ../include/
		rm $file
	fi
done
