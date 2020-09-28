#!/usr/bin/perl -wT

#   Copyright (c) 2016 by Plexim GmbH
#   All rights reserved.


use strict;
use CGI::Fast;
use CGI::Carp qw ( fatalsToBrowser );
use File::Basename;
use IO::Socket::UNIX;
use JSON::PP;
use Fcntl;
use Errno qw ( EAGAIN );

$CGI::POST_MAX = 50 * 1024 * 1024;
my $safe_filename_characters = "a-zA-Z0-9_.-";
my $upload_dir = "/lib/firmware";
my $cpuSerial;
my $boardSerial;
my $macAddress;
my $boardRevision;
my $firmwareVersion;
my $firmwareBuild;
my $fpgaVersion;
my %testProcessInfo;

$ENV{'PATH'} = '/bin:/usr/bin';

sub readLineFile($)
{
   my $filename = shift;
   open(my $fh, '<', $filename)
      or die "Could not open file '$filename' $!";
   my $line = <$fh>;
   chomp $line;
   close $fh;
   return $line;
}

sub pipe_from_fork ()
{
   pipe my $parent, my $child or die;
   my $pid = fork();
   die "fork() failed: $!" unless defined $pid;
   if ($pid)
   {
      close $child;
      my $flags = 0;
      fcntl($parent, F_GETFL, $flags) || die $!; # Get the current flags on the filehandle
      $flags |= O_NONBLOCK; # Add non-blocking to the flags
      fcntl($parent, F_SETFL, $flags) || die $!; # Set the flags on the filehandle
   }
   else
   {
      # child process
      close $parent;
      untie *STDIN; untie *STDOUT; untie *STDERR;
      open(STDOUT, ">&=" . fileno($child)) or die "Cannot open stdout\n";
   }
   my %ret = (
      'pid' => $pid,
      'pipe' => $parent
   );
   return %ret;
}

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

sub stopApplication()
{
   if (-e '/tmp/hilserver')                                
   {                                                       
      my $client = IO::Socket::UNIX->new(                  
         Type => SOCK_STREAM,                              
         Peer => "/tmp/hilserver",                         
      );                                                   
      $client->print(pack('i<', 0xef56a55a));              
      my $errorString = <$client>;
   }                                         
}

sub peek($)                                                   
{                                                             
   my ($addr) = @_;                                           
   my $blksize = 64*1024;                                     
   my $blk = int($addr/$blksize) * $blksize;                  
   my $offset = $addr - $blk;                                 
   sysopen(my $fh, "/dev/mem", O_RDONLY) or die "Cannot open /dev/mem";
   my $ptr = syscall(192, 0, $blksize, 1, 1, fileno($fh), $blk/4096); #mmap2
   close($fh);                                                               
   my $valderef = unpack("P[4]", pack("L", $ptr + $offset));               
   my $val = unpack("L", $valderef);                                         
   syscall(91, $ptr, $blksize); #munmap                                  
   return $val;                                                              
}                                                                            


sub checkRunning()
{
   my $ret = 0;
   my $filename = '/sys/kernel/debug/remoteproc/remoteproc0/state';
   if (-e $filename)
   {
      my $val = readLineFile($filename);
      $ret = ($val =~ m/^running/);
   }
   return $ret;
}

my $query;
while ($query = CGI::Fast->new) 
{
   my $file = basename ($ENV{'SCRIPT_FILENAME'}, ".cgi");

   SWITCH:
   for ($file)
   {
      if (/^stop/)
      {
         stopApplication();
         print $query->header();
         last SWITCH;
      }

      if (/^start/)
      {
         my $errorString = startApplication($query->param("startOnFirstTrigger") && ($query->param("startOnFirstTrigger") eq 'true'));
         if ($errorString) {
            print $query->header(-status => 500);
            print $errorString;
         }
         else {
            print $query->header();
         }
         last SWITCH;
      }

      if (/^front-panel/) {
         my $ledVal = peek(0xe000a06c);
         my $leds = {
            'error' => ($ledVal & 0x00010000) > 0,
            'running' => ($ledVal & 0x00020000) > 0,
            'ready' => ($ledVal & 0x00040000) > 0,
            'power' => ($ledVal & 0x00080000) > 0,
         };
         my $analogIn;
         my $analogOut;
         my $digitalIn = 13;
         my $digitalOut;
         if ($ledVal & 0x00400000) {  # VAD_EN
            $analogIn = ($ledVal & 0x00800000) ? 5 : 4;
            $analogOut = peek(0xffff4000);
         };
         if (not ($ledVal & 0x00200000)) { # DOUT_OE_N
            $digitalOut = ($ledVal & 0x00800000) ? 11 : 12;
         };
         my $voltageRanges = {
            'analogIn' => $analogIn,
            'analogOut' => $analogOut,
            'digitalIn' => $digitalIn,
            'digitalOut' => $digitalOut,
         };
         my $ret = {
            'leds' => $leds,
            'voltageRanges' => $voltageRanges,
         };
         print $query->header();
         print(encode_json($ret));
         last SWITCH;
      }

      if (/^upload/)
      {
         my $safe_filename_characters = "a-zA-Z0-9_.-";
         my $upload_file = "/lib/firmware/firmware";
         my $filename = $query->param("filename");

         if ( !$filename )
         {
            print $query->header(-status => 400);
            print "There was a problem uploading your file.";    
            last SWITCH;
         }

         my $upload_filehandle = $query->upload("filename");
         if (!$upload_filehandle && $query->cgi_error)
         {
            print $query->header(-status=>$query->cgi_error);
            last SWITCH;
         }

         open ( UPLOADFILE, ">$upload_file" ) or die "$!";
         binmode UPLOADFILE;

         while ( <$upload_filehandle> )
         {
            print UPLOADFILE;
         }

         close UPLOADFILE;
         print $query->header();
         last SWITCH;         
      }

      if (/^boxtest/)
      {
         my $boardhw_filename = '/media/mmcblk0p1/testHwVersion';
         if (-e $boardhw_filename)
         {
            open(my $fh, '<', $boardhw_filename)
               or die "Could not open file '$boardhw_filename' $!";
            my $val = readline($fh);
            my ($testHwVersionMajor) = ($val =~ /^(\d+)$/);
            $val = readline($fh);
            my ($testHwVersionMinor) = ($val =~ /^(\d+)$/);
            close($fh);
            my $ver = ($testHwVersionMajor << 16) | $testHwVersionMinor;
            `poke 0xffff4004 $ver`;
            if ($ver > 0x00010001)
            {
               `echo 871 > /sys/class/gpio/export`;
               `echo out > /sys/class/gpio/gpio871/direction`;
               `echo 1 > /sys/class/gpio/gpio871/value`;
            }
         }
         if (!-e '/www/cgi/boxtest.pl')
         {
            print $query->header(-status => 501);
            last SWITCH;
         }
         if (!-e '/media/mmcblk0p1/testApp.elf')
         {
            print $query->header(-status => 501);
            last SWITCH;
         }
         `cp /media/mmcblk0p1/testApp.elf /lib/firmware/firmware`;
         stopApplication();
         startApplication(0);
         %testProcessInfo = pipe_from_fork();
         if (!$testProcessInfo{'pid'})
         {
            # child process
            sleep(1);
            exec('/www/cgi/boxtest.pl');
            exit;
         }
         print $query->header();
         last SWITCH;
      }

      if (/^teststatus/)
      {
         my $logContents = '';
         my $endOfTest = 0;
         if (!exists($testProcessInfo{'pipe'}))
         {
            print $query->header(-status => 503);
            last SWITCH;
         }
         while (1)
         {
            my $newline = readline($testProcessInfo{'pipe'});
            if (defined($newline))
            {
               $logContents .= $newline;
            }
            elsif ($! != EAGAIN)
            {
               close($testProcessInfo{'pipe'});
               waitpid($testProcessInfo{'pid'}, 0);
               delete $testProcessInfo{'pid'};
               $endOfTest = 1;
               last;
            }
            else
            {
               last;
            }
         }
         my $ret = {
            'testLog' => $logContents,
            'endOfTest' => $endOfTest,
         };
         print $query->header();
         print(encode_json($ret));
         last SWITCH;
      }      

      if (/^ipstate/)
      {
         my $ipInfo = `/sbin/ip -o -4 addr show dev eth0`;
         my ($ip) = ($ipInfo =~ m/inet (\d+\.\d+\.\d+\.\d+\/\d+)/);
         my $ret = {
            'ip' => $ip,
         };
         print $query->header();
         print(encode_json($ret));

         last SWITCH;
      }

      if (/^netstate/)
      {
         my $ret = {
            'speed' => readLineFile('/sys/class/net/eth0/speed'),
            'duplex' => readLineFile('/sys/class/net/eth0/duplex'),
            'rxp' => readLineFile('/sys/class/net/eth0/statistics/rx_packets'),
            'txp' => readLineFile('/sys/class/net/eth0/statistics/tx_packets'),
            'collisions' => readLineFile('/sys/class/net/eth0/statistics/collisions'),
            'tx_errors' => readLineFile('/sys/class/net/eth0/statistics/tx_errors'),
            'rx_errors' => readLineFile('/sys/class/net/eth0/statistics/rx_errors'),
         };
         print $query->header();
         print(encode_json($ret));

         last SWITCH;
      }
      else 
      {
         print $query->header(-status => 400);
      }
   }
}
