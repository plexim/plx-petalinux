#!/usr/bin/perl -w

#   Copyright (c) 2016 by Plexim GmbH
#   All rights reserved.


use strict;
my $board_filename = '/sys/bus/i2c/devices/0-0052/eeprom'; 
open(my $fh, '<', $board_filename)
   or die "Could not open file '$board_filename' $!";      
binmode $fh;                                             
read($fh, my $buffer, 2);
if (unpack("S", $buffer) == 1)
{
   read($fh, $buffer, 2);
   my $hwMajor = unpack("S", $buffer);
   read($fh, $buffer, 2);
   my $hwMinor = unpack("S", $buffer);
   my $ver = ($hwMajor << 16) | $hwMinor;
   `poke 0xffff4004 $ver`;
}

