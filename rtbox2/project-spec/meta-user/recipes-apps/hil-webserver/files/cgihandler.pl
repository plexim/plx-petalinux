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
   my $blksize = 4*1024;                                     
   my $blk = int($addr/$blksize) * $blksize;
   my $offset = $addr - $blk;
   sysopen(my $fh, "/dev/mem", O_RDONLY) or die "Cannot open /dev/mem";
   my $ptr = syscall(222, 0, $blksize, 1, 1, fileno($fh), $blk); #mmap2
   close($fh);                                                               
   my $valderef = unpack("P[4]", pack("Q", $ptr + $offset));               
   my $val = unpack("L", $valderef);                                         
   syscall(215, $ptr, $blksize); #munmap                                  
   return $val;                                                              
}

sub outputFile($)
{
   my ($filename) = @_; 
   print "<pre>";
   if (open(FH, '<' ,$filename))
   {
      while ( <FH> )
      {
         print $_;
      }
   }
   else
   {
      print "Error: $!";
   }
   print "</pre>";
   close FH;
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
         if (!-e '/www/cgi/boxtest.pl')
         {
            print $query->header(-status => 501);
            last SWITCH;
         }
         if (!-e '/run/media/mmcblk1p1/testApp.elf')
         {
            print $query->header(-status => 501);
            last SWITCH;
         }
         `cp /run/media/mmcblk1p1/testApp.elf /lib/firmware/firmware`;
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

      if (/^ipstate$/)
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
      if (/^ipstate2$/)
      {
         my $ipInfo = `/sbin/ip -o -4 addr show dev eth3`;
         my ($ip) = ($ipInfo =~ m/inet (\d+\.\d+\.\d+\.\d+\/\d+)/);
         my $ret = {
            'ip' => $ip,
         };
         print $query->header();
         print(encode_json($ret));

         last SWITCH;
      }

      if (/^netstate$/)
      {
         my $state = readLineFile('/sys/class/net/eth0/operstate');
         my $ret;
         my $speed = 0;
         my $duplex = "";
         if ($state eq "up")
         {
            $speed = readLineFile('/sys/class/net/eth0/speed');
            $duplex = readLineFile('/sys/class/net/eth0/duplex');
         }
	 $ret = {
            'state' => $state,
	    'speed' => $speed,
            'duplex' => $duplex,
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
      if (/^netstate2$/)
      {
         my $state = readLineFile('/sys/class/net/eth3/operstate');
         my $ret;
         my $speed = 0;
         my $duplex = "";
         if ($state eq "up")
         {
            $speed = readLineFile('/sys/class/net/eth3/speed');
            $duplex = readLineFile('/sys/class/net/eth3/duplex');
         }
	 $ret = {
            'state' => $state,
	    'speed' => $speed,
            'duplex' => $duplex,
            'rxp' => readLineFile('/sys/class/net/eth3/statistics/rx_packets'),
            'txp' => readLineFile('/sys/class/net/eth3/statistics/tx_packets'),
            'collisions' => readLineFile('/sys/class/net/eth3/statistics/collisions'),
            'tx_errors' => readLineFile('/sys/class/net/eth3/statistics/tx_errors'),
            'rx_errors' => readLineFile('/sys/class/net/eth3/statistics/rx_errors'),
	 };
         print $query->header();
         print(encode_json($ret));

         last SWITCH;
      }
      if (/^consolelog$/)
      {
         print $query->header();
         outputFile('/sys/devices/jailhouse/console');
         last SWITCH;
      }
      if (/^syslog$/)
      {
         print $query->header();
         outputFile('/var/log/messages');
         last SWITCH;
      }
      if (/^scopeserverlog$/)
      {
         print $query->header();
         outputFile('/var/log/scopeserver.log');
         last SWITCH;
      }
      else 
      {
         print $query->header(-status => 400);
      }
   }
}
