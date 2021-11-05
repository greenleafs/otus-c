#!/bin/bash
SKEL_DIR=hwskel
HOMEWORK_DIR=homework$1

if [ -z "$1" ]; then
    echo "Need to specify homework number... exit."
    exit 1
fi

if [ -e $HOMEWORK_DIR ]; then
    echo "$HOMEWORK_DIR already exists... exit."
    exit 1
fi

echo "Making directory of $HOMEWORK_DIR ..."
mkdir homework$1
if [ ! $? -eq 0 ]; then
    echo "Can't create directory $HOMEWORK_DIR"
    exit 1
fi

echo "Move files to $HOMEWORK_DIR..."
cp $SKEL_DIR/homework.c $SKEL_DIR/Makefile $SKEL_DIR/test.py $HOMEWORK_DIR/
if [ ! $? -eq 0 ]; then
    echo "Can't move files to $HOMEWORK_DIR"
    exit 1
fi

echo "Finished OK."
