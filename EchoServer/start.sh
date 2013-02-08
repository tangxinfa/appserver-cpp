#!/bin/bash

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/ spawn-fcgi -p 8000 -- ./EchoServer -t 16
