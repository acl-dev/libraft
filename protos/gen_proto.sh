#!/bin/bash

for file in ./*.proto
do
	if test -f $file
	then
		protoc --cpp_out=./ --proto_path=./ $file
	fi
done

if [ ! -d "../src/proto_gen/" ]; then
	cd ..
	mkdir -p src/proto_gen
	cd ./../protos
fi

if [ ! -d "../include/proto_gen" ]; then
	cd .. 
	mkdir -p include/proto_gen
	cd ./../protos
fi

for file in ./*
do
	ext="${file##*.}"
	if [ $ext = "cc" ]
	then
		cp $file ../src/proto_gen/
	fi
	if [ $ext = "h" ]
	then
		cp $file ../include/proto_gen/
	fi
done
