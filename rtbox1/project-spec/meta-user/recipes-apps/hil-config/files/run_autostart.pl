#!/usr/bin/perl -wT

#   Copyright (c) 2016 by Plexim GmbH
#   All rights reserved.


use strict;
use IO::Socket::UNIX;
use Errno qw ( EAGAIN );

my $upload_dir = "/lib/firmware";

$ENV{'PATH'} = '/bin:/usr/bin';

sub startApplication($)
{
   my $startOnFirstTrigger = shift;
   my $errorString;
   if (-e '/tmp/hilserver')                                
   {                                                       
      my $client = IO::Socket::UNIX->new(                  
         Type => SOCK_STREAM,                              
         Peer => "/tmp/hilserver",                         
      ) or die "Cannot open socket: $!";                                                   
      $client->print(pack('i<', 0x00000005));
      if ($startOnFirstTrigger) 
      {
         $client->print(pack('i<', 0x00000001));
      }
      else
      {
         $client->print(pack('i<', 0x00000000));
      }
      $errorString = <$client>;
      chomp $errorString;
   }
   else
   {
      $errorString = "Cannot connect to scopeserver process.";
   }
   return $errorString;
}

if (-e '/media/mmcblk0p1/autostart.elf')
{
   `cp /media/mmcblk0p1/autostart.elf $upload_dir/firmware`;
   my $errorString = startApplication(0);
   $errorString and print "$errorString\n";
}
elsif (-e '/media/mmcblk0p1/firmware.bin')
{
   `cp /media/mmcblk0p1/firmware.bin $upload_dir/firmware`;
   my $errorString = startApplication(0);
   $errorString and print "$errorString\n";
}

