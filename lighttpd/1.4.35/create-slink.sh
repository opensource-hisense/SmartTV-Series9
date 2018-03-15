#!/bin/sh

rm -f libfcgi.so
rm -f libfcgi++.so
rm -f libfcgi.so.0
rm -f libfcgi++.so.0
rm -f libjansson.so
rm -f libjansson.so.4

ln -s libfcgi.so.0.0.0 libfcgi.so
ln -s libfcgi.so.0.0.0 libfcgi.so.0
ln -s libfcgi++.so.0.0.0 libfcgi++.so
ln -s libfcgi++.so.0.0.0 libfcgi++.so.0
ln -s libjansson.so.4.6.0 libjansson.so
ln -s libjansson.so.4.6.0 libjansson.so.4
