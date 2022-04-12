#!/usr/bin/perl

use strict;
use Time::HiRes qw(usleep);
use Fcntl;
use Errno qw ( EAGAIN );
use IO::Socket;
use IO::Socket::UNIX;

my $scaleCorrection = 1.0;
my $rtbox3 = 0;
my $hwVer;
my $dacConfig = 0x3f;
my $pullupEnable = 0;
my $display = IO::Socket::UNIX->new(                  
	Type => SOCK_STREAM,                              
   Peer => "/tmp/display_log_socket",                         
);                                                   

sub setDacConfig($)
{
   my ($newConfig) = @_;
   $dacConfig = $newConfig;
   my $sock = IO::Socket::INET->new(
      Proto    => 'udp',
      PeerPort => 1000,
      PeerAddr => 'localhost',
   ) or die "Could not create socket: $!\n";

   $sock->send(pack('C', $newConfig) . pack('C', $newConfig)) or die "Send error: $!\n";
   usleep(100);
}

sub enablePullups()
{
   $pullupEnable = 0xffffffff;
}

sub disablePullups()
{
   $pullupEnable = 0x0;
}

sub setPullups($$)
{
   my ($board, $val) = @_;
   my $sock = IO::Socket::INET->new(
      Proto    => 'udp',
      PeerPort => 1001,
      PeerAddr => 'localhost',
   ) or die "Could not create socket: $!\n";

   $sock->send(pack('L', $board) . pack('L', $pullupEnable) . pack('L', $val)) or die "Send error: $!\n";
   usleep(1000);
}

sub initScaleCorrection()
{
   $scaleCorrection = 1.024;
   my $boardhw_filename = '/run/media/mmcblk1p1/testHwVersion';
   if (-e $boardhw_filename)
   {
      open(my $fh, '<', $boardhw_filename)
         or die "Could not open file '$boardhw_filename' $!";
      my $val = readline($fh);
      my ($testHwVersionMajor) = ($val =~ /^(\d+)$/);
      $val = readline($fh);
      my ($testHwVersionMinor) = ($val =~ /^(\d+)$/);
      close($fh);
      $hwVer = ($testHwVersionMajor << 16) | $testHwVersionMinor;
   }
   else
   {
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
         if ($hwMajor == 0xffff && $hwMinor == 0xffff)
         {
            print "Cannot read hardware revision from EEPROM.\n";
            die "Cannot read hardware revision from EEPROM.\n";
         }
         $hwVer = ($hwMajor << 16) | $hwMinor;
      }
      else
      {
         print "EEPROM not programmed.\n";
         die "EEPROM not programmed.\n";
      }
   }
   my $rtbox_id_file = '/sys/class/gpio/gpio497/value';
   open(my $fh, '<', $rtbox_id_file) or die "Cannot open '$rtbox_id_file': $!";
   $rtbox3 = <$fh>;
   chomp($rtbox3);
   if ($rtbox3)
   {
      print "Testing RT Box 3\n";
   }
   else
   {
      print "Testing RT Box 2\n";
   }
}

sub pipe_from_fork ()
{
   pipe my $parent, my $child or die;
   $child->autoflush(1);
   my $pid = fork();
   die "fork() failed: $!" unless defined $pid;
   if ($pid)
   {
      close $child;
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

sub i2c_to_base($)
{
   my ($i2c_addr) = @_;
   opendir(my $dh, "/sys/bus/i2c/devices/0-00$i2c_addr/gpio");
   my @entries = grep { /^gpiochip/ } readdir($dh);
   closedir($dh);
   my @baseaddr = ($entries[0] =~ m/gpiochip(\d+)/);
   return $baseaddr[0];
}

sub setDigitalOutput($$)
{
   my ($idx, $value) = @_;
   open(my $fh, '>', "/sys/class/gpio/gpio$idx/value");
   print $fh "$value\n";
   close($fh);
   usleep(100);
}

sub exportPin($)
{
   my ($idx) = @_;
   open(my $fh, '>', "/sys/class/gpio/export");
   print $fh "$idx\n";
   close($fh);
}

sub unexportPin($)
{
   my ($idx) = @_;
   open(my $fh, '>', "/sys/class/gpio/unexport");
   print $fh "$idx\n";
   close($fh);
}

sub makeOutput($)
{
   my ($idx) = @_;
   open(my $fh, '>', "/sys/class/gpio/gpio$idx/direction");
   print $fh "out\n";
   close($fh);
}

sub makeInput($)
{
   my ($idx) = @_;
   open(my $fh, '>', "/sys/class/gpio/gpio$idx/direction");
   print $fh "in\n";
   close($fh);
}

sub getDigitalPinInput($)
{
   my ($idx) = @_;
   open(my $fh, '<', "/sys/class/gpio/gpio$idx/value");
   my $ret = <$fh>;
   close($fh);
   return $ret;
}

sub readLineFile($)
{
   my ($file) = @_;
   open(my $fh, '<', $file);
   my $ret = <$fh>;
   close($fh);
   return $ret;
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

sub getDigitalInput($)
{
   my ($idx) = @_;
   my $val;
   if ($idx > 31)
   {
      $val = peek(hex("0xfffc10a4"));
      $idx -= 32;
   }
   else
   {
      $val = peek(hex("0xfffc10a0"));
   }
   return ($val >> $idx) & 1;
}

sub enableDigitalOuts()
{
   setDacConfig($dacConfig & 0xbf)
}

sub disableDigitalOuts()
{
   setDacConfig($dacConfig | 0x40)
}

sub setDigitalOuts3V()
{
   setDacConfig($dacConfig & 0xdf)
}

sub setDigitalOuts5V()
{
   setDacConfig($dacConfig | 0x20)
}

sub enableAnalogOuts()
{
   #`poke 0x43c00004 1`;
}

sub disableAnalogOuts()
{
   #`poke 0x43c00004 0`;
}

sub setAnalogInputScale($$)
{
   my ($idx, $val) = @_;
   my $scaledVal = $val * $scaleCorrection * 10/32767;
   my $intVal = unpack("L", pack("f", $scaledVal));
   my $addr = 0x43C90000 + $idx * 4;
   `poke $addr, $intVal`;
}

sub setAnalogInputOffset($$)
{
   my ($idx, $val) = @_;
   my $intVal = unpack("L", pack("f", $val));
   my $addr = 0x43CA0000 + $idx * 4;
   `poke $addr, $intVal`;
}

sub setAnalogOutput($$)
{
   my @idxLookup = (17, 0, 20, 1, 21, 4, 24, 5, 25, 8, 28, 9, 29, 12, 32, 13, 19, 2, 22, 3, 23, 6, 26, 7, 27, 10, 30, 11, 31, 14, 34, 15, 33, 16, 35, 18);
   my ($idx, $val) = @_;
   my $intVal = unpack("L", pack("f", $val));
   my $addr = 0xfffc0000 + $idxLookup[$idx] * 4;
   `poke $addr, $intVal`;
   $addr = 0xfffc2000 + $idxLookup[$idx] * 4;
   `poke $addr, $intVal`;
}

sub getAnalogInput($)
{
   my ($idx) = @_;
   my $addr = 0xfffc1170 + $idx * 4;
   my $intVal = peek($addr);
   return unpack("f", pack("L", $intVal));
}

sub setSFPOut($$)
{
   my ($idx, $val) = @_;
   my $intVal = unpack("L", pack("f", $val));
   my $offset;
   if ($idx < 4)
   {
      $offset = $idx * 16;
   }
   else
   {
      $offset = ($idx - 4) * 16 + 8;
   }
   my $addr = 0xfffc0230 + $offset;
   `poke $addr, $intVal`;
   $addr = 0xfffc2230 + $offset;
   `poke $addr, $intVal`;
}

sub getSFPIn($)
{
   my ($idx) = @_;
   my $addr;
   if ($idx < 4)
   {
      $addr = 0xfffc1000 + $idx * 32 + 16;
   }
   else
   {
      $addr = 0xfffc1000 + ($idx - 4) * 32 + 24;
   }
   my $intVal = peek($addr);
   return unpack("f", pack("L", $intVal));
}

sub getAveragedAnalogInput($$)
{
   my ($idx, $n) = @_;
   my @i = (1..$n);
   my $i;
   my $ret = 0;
   for $i (@i)
   {
      $ret += getAnalogInput($idx);
      usleep(10);
   }
   return $ret / $n;
}

sub testDigitalIOs
{
   setDigitalOuts3V();
   enableDigitalOuts();
   my @idx = (0..31);
   if ($rtbox3)
   {
      @idx = (0..63);
   }
   my $i;
   my $errorFlag = 0;
   print "Testing digital I/Os with 3.3V";
   for $i (@idx)
   {
      setDigitalOutput($i+416, 0);
   }
   for $i (@idx)
   {
      setDigitalOutput($i+416, 1);
      for my $j (@idx)
      {
         if (($i != $j) && (getDigitalInput($j) == 1))
         {
            print "\nERROR: Input $j is high when output $i is set high";
            $errorFlag = 1;
         }
         elsif (($i == $j) && (getDigitalInput($j) == 0))
         {
            print "\nERROR: Input $j is low when output $i is set high";
            $errorFlag = 1;
         }

      }
      setDigitalOutput($i+416, 0);
   }
   setDigitalOuts5V();
   print " 5V ...";
   for $i (@idx)
   {
      setDigitalOutput($i+416, 1);
      for my $j (@idx)
      {
         if (($i != $j) && (getDigitalInput($j) == 1))
         {
            print "\nERROR: Input $j is high when output $i is set high";
            $errorFlag = 1;
         }
         elsif (($i == $j) && (getDigitalInput($j) == 0))
         {
            print "\nERROR: Input $j is low when output $i is set high";
            $errorFlag = 1;
         }

      }
      setDigitalOutput($i+416, 0);
   }

   if ($errorFlag)
   {
      print " FAIL\n";
      $display->print("Test of digital IOs failed!");
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag
}

sub testPullUps($)
{
   my ($board) = @_;
   my @idx = (0..31);
   my $i;
   disableDigitalOuts();
   my $errorFlag = 0;
   print "Testing pull-ups ...";
   enablePullups();
   for $i (@idx)
   {
      my $pullupVal = 1 << $i;
      setPullups($board, $pullupVal);
      for my $j (@idx)
      {
         if (($i != $j) && (getDigitalInput($j + 32 * $board) == 1))
         {
            print "\nERROR: Input ", $j+32*$board, " is high when pull-up ", $i+32*$board, " is set high";
            $errorFlag = 1;
         }
         elsif (($i == $j) && (getDigitalInput($j+32*$board) == 0))
         {
            print "\nERROR: Input ", $j+32*$board, " is low when pull-up ", $i+32*$board, " is set high";
            $errorFlag = 1;
         }
      }
   }
   disablePullups();
   enableDigitalOuts();
   if ($errorFlag)
   {
      print " FAIL\n";
      $display->print("Test of pull-ups failed!");
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag
}

sub testAnalogIOs()
{
   my $errorFlag = 0;
   my @idx = (0..15);
   if ($rtbox3)
   {
      @idx = (0..31);
   }
   my $i;

   #exportPin(993); # Analog input voltage
   #makeOutput(993);
   #setDigitalOutput(993, 0); # +-10V
   print "Testing analog I/Os ...";
   enableAnalogOuts();
   for $i (@idx)
   {
      setAnalogOutput($i, $i/$idx[-1]*10);
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/$idx[-1]*10;
      my $input = getAveragedAnalogInput($i, 3);
      if (abs($input - $setValue) > 0.03)
      {
         print "\nERROR: Input $i is $input Volt, expected $setValue Volt";
         $errorFlag = 1;
      }
   }   
   for $i (@idx)
   {
      setAnalogOutput($i, $i/$idx[-1]*10-10);
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/$idx[-1]*10-10;
      my $input = getAveragedAnalogInput($i, 3);
      if (abs($input - $setValue) > 0.03)
      {
         print "\nERROR: Input $i is $input Volt, expected $setValue Volt";
         $errorFlag = 1;
      }
   }
   for $i (@idx)                                                             
   {                                                                         
      setAnalogOutput($i, -$i/$idx[-1]*10);                                          
   }                                                                         
   usleep(10);                                                               
   for $i (@idx)                                                             
   {                                                                         
      my $setValue = -$i/$idx[-1]*10;                                                
      my $input = getAveragedAnalogInput($i, 3);                             
      if (abs($input - $setValue) > 0.03)                                   
      {                                                                      
         print "\nERROR: Input $i is $input Volt, expected $setValue Volt";  
         $errorFlag = 1;                                                     
      }                                                                      
   }                                                                         
   if ($errorFlag)
   {
      print " FAIL\n";
   }
   else
   {
      print(" ok\n");
   }

   print("Testing +-5 Volt input range");
   setDacConfig(0x37);
   setDacConfig(0x25);
   setDacConfig(0x2d);
   usleep(100000);
   for $i (@idx)
   {
      setAnalogOutput($i, $i/$idx[-1]*10);
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/$idx[-1]*5;
      my $input = getAveragedAnalogInput($i, 3)/2;
      if (abs($input - $setValue) > 0.03)
      {
         print "\nERROR: Input $i is $input Volt, expected $setValue Volt";
         $errorFlag = 1;
      }
   }   

   disableAnalogOuts();
   #unexportPin(993);
   if ($errorFlag)
   {
      print " FAIL\n";
      $display->print("Test of analog IOs failed!");
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag
}

sub testAnalogInputScaleOffset()
{
   my $errorFlag = 0;
   my @idx = (0..15);
   my $i;

   exportPin(993); # Analog input voltage
   makeOutput(993);
   setDigitalOutput(993, 0); # +-10V
   print "Testing analog input scale ...";
   enableAnalogOuts();
   for $i (@idx)
   {
      setAnalogOutput($i, 1);
      setAnalogInputScale($i, $i/3);
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/$idx[-1]*5;
      my $input = getAveragedAnalogInput($i, 3);
      if (abs($input - $setValue) > 0.03)
      {
         print "\nERROR: Input $i is $input Volt, expected $setValue Volt";
         $errorFlag = 1;
      }
   }   
   if ($errorFlag)
   {
      print " FAIL\n";
   }
   else
   {
      print(" ok\n");
   }

   print "Testing analog input offset ...";
   my $errorFlagOffset = 0;
   for $i (@idx)
   {
      setAnalogOutput($i, 0);
      setAnalogInputScale($i, 1);
      setAnalogInputOffset($i, $i/$idx[-1]*10);
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/1.5;
      my $input = getAveragedAnalogInput($i, 3);
      if (abs($input - $setValue) > 0.03)
      {
         print "\nERROR: Input $i is $input Volt, expected $setValue Volt";
         $errorFlagOffset = 1;
      }
   }
   for $i (@idx)
   {
      setAnalogInputScale($i, 1);
      setAnalogInputOffset($i, 0);
   }
   disableAnalogOuts();
   unexportPin(993);
   if ($errorFlagOffset)
   {
      print " FAIL\n";
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag || $errorFlagOffset;
}


sub testSFP
{
   my $errorFlag = 0;
   my @peerIdx = (1, 0, 3, 2, 5, 4, 7, 6);
   my $testA = 5.13457489013672;
   my $testB = 8.56934523341124e-09;
   my $i;
   print "Testing SFP ...";
   for $i (0..7)
   {
      setSFPOut($i, $testA);
      usleep(10);
      my $resA = getSFPIn($peerIdx[$i]);
      if (abs($resA - $testA) > 1e-6)
      {
         print "\nERROR: while reading SFP $i: got $resA, expected $testA";
         $errorFlag = 1;
      }
      setSFPOut($i, $testB);
      usleep(10);
      my $resB = getSFPIn($peerIdx[$i]);
      if (abs($resB != $testB) > 1e-6)
      {
         print "\nERROR: while reading SFP $i: got $resB, expected $testB";
         $errorFlag = 1;
      }
   }
   if ($errorFlag)
   {
      print " FAIL\n";
      $display->print("SFP test failed!");
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag
}

sub testEthernet
{
   my $errorFlag = 0;
   print "Testing Ethernet...";
   `/sbin/ip netns add ns_server`;
   `/sbin/ip link set eth1 netns ns_server`;
   `/sbin/ip netns exec ns_server /sbin/ip addr add dev eth1 192.168.101.1/24`;
   `/sbin/ip netns exec ns_server /sbin/ip link set dev eth1 up`;
   if ($? == 512)
   {
      `/sbin/ip netns exec ns_server /sbin/ip link set dev eth1 up`;
   }
   `/sbin/ip netns add ns_client`;
   `/sbin/ip link set eth2 netns ns_client`;
   `/sbin/ip netns exec ns_client /sbin/ip addr add dev eth2 192.168.101.2/24`;
   `/sbin/ip netns exec ns_client /sbin/ip link set dev eth2 up`;
   if ($? == 512)
   {
      `/sbin/ip netns exec ns_client /sbin/ip link set dev eth2 up`;
   }
   sleep(1);
   `/sbin/ip netns exec ns_client /bin/ping -c 1 192.168.101.1`;
   if ($?)
   {
      $errorFlag = 1;
   }
   `/sbin/ip netns del ns_server`;
   `/sbin/ip netns del ns_client`;
   if ($errorFlag)
   {
      print " FAIL\n";
      $display->print("Ethernet test failed!");
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag;
}

sub resolverRead($)
{
   my ($devnum) = @_;
   my $addr = 0xfffc11f0 + $devnum * 4;
   my $intVal = peek($addr);
   return ($intVal & 0xffff, ($intVal >> 16) & 0xff);
}

sub testResolver($)
{
   my ($devnum) = @_;
   my $errorFlag = 0;
   print "Tesing Resolver $devnum ...";
   enableAnalogOuts();
   setAnalogOutput(32+2*$devnum, 0);
   setAnalogOutput(32+2*$devnum+1, 1);
   usleep 100;
   my ($pos, $err) = resolverRead($devnum);
   my $i;
   for $i (1..32) 
   {
      setAnalogOutput(32+2*$devnum, sin($i/64*3.14));
      setAnalogOutput(32+2*$devnum+1, cos($i/64*3.14));
      usleep 10;
   }
   setAnalogOutput(32+2*$devnum, 1);
   setAnalogOutput(32+2*$devnum+1, 0);
   usleep 100;
   ($pos, $err) = resolverRead($devnum);
   my $diff = abs($pos - 16384);
   if ($diff > 128)
   {
      print("\nResolver position error 1: $diff\n");
      $errorFlag = 1;
   }
   if ($err)
   {
      print("\nResolver error 1: $err\n");
      $errorFlag = 1;
   }
   for $i (1..32) 
   {
      setAnalogOutput(32+2*$devnum, cos($i/64*3.14));
      setAnalogOutput(32+2*$devnum+1, -sin($i/64*3.14));
      usleep 10;
   }
   setAnalogOutput(32+2*$devnum, 0);
   setAnalogOutput(32+2*$devnum+1, -1);
   usleep 100;
   ($pos, $err) = resolverRead($devnum);
   $diff = abs($pos - 32768);
   if ($diff > 128)
   {
      print("\nResolver position error 2: $diff\n");
      $errorFlag = 1;
   }
   if ($err)
   {
      print("\nResolver error 2: $err\n");
      $errorFlag = 1;
   }
   disableAnalogOuts();
   if ($errorFlag)
   {
      print " FAIL\n";
      $display->print("Resolver test failed!");
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag;
}

sub testCAN
{
   print "Testing CAN ...";
   `/sbin/ip link set down can0`;
   `/sbin/ip link set down can1`;
   # enable internal CAN interface
   `/sbin/ip link set can0 type can bitrate 1000000`;
   `/sbin/ip link set up can0`;
   `/sbin/ip link set can1 type can bitrate 1000000`;
   `/sbin/ip link set up can1`;
   my %dumpProcessInfo = pipe_from_fork();
   if (!$dumpProcessInfo{'pid'})
   {
      # child process
      exec('/usr/bin/candump', 'can1');
      exit;
   }
   usleep(100000);
   my $errorFlag = 1;
   system ('cansend', 'can0', '-i', '0x61f', '0xff', '0xff', '0x02', '0x02', '0x00', '0x68', '0x00', '0x00', '0x00');
   if ($?)
   {
      print "cansend FAILED: $?\n";
      return $errorFlag;
   }
   my $rin = '';
   my $fh = $dumpProcessInfo{'pipe'};
   vec($rin, fileno($fh), 1) =1;

   my $nfound = select(my $rout = $rin, undef, my $eout = $rin, 1);
   if ($nfound)             
   {
      readline($fh);
      my $newline = readline($fh);                                                           
      if (defined($newline))
      {
         if ($newline eq "<0x61f> [8] ff ff 02 02 00 68 00 00 \n")
         {                      
            $errorFlag = 0;
         }                                             
      }                                  
   }
   `/sbin/ip link set down can1`;
   close($dumpProcessInfo{'pipe'});
   waitpid($dumpProcessInfo{'pid'}, 0);
   delete $dumpProcessInfo{'pid'};
   if ($errorFlag)
   {
      print " FAIL\n";
      $display->print("CAN test failed!");
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag;  
}

sub testVoltages()
{
   opendir(my $dh, "/sys/bus/i2c/devices/0-0040/hwmon");
   my @entries = grep { /^hwmon/ } readdir($dh);
   closedir($dh);
   my $hwmon= "/sys/bus/i2c/devices/0-0040/hwmon/$entries[0]/";
   print "Testing Voltages: 13.0V ";
   my $errorFlag = 0;
   my $volt12 = readLineFile($hwmon . "in3_input");
   if ($volt12 < 12800 || $volt12 > 13100)
   {
      print "Error: " . $volt12/1000 . "V\n";
      $errorFlag = 1;
   }
   print "3.3V ";
   my $volt33 = readLineFile($hwmon . "in1_input");
   if ($volt33 < 3200 || $volt33 > 3400)
   {
      print "Error: " . $volt33/1000 . "V\n";
      $errorFlag = 1;
   }
   print "5.0V ";
   my $volt50 = readLineFile($hwmon . "in2_input");
   if ($volt50 < 4900 || $volt50 > 5100)
   {
      print "ERROR: " . $volt50/1000 . "V\n";
      $errorFlag = 1;
   }
   if ($errorFlag == 0)
   {
      print "Ok.\n";
   }
   else
   {
   	print "FAIL\n";
      $display->print("Voltage test failed!");
   }
   return $errorFlag;
}

my $display = IO::Socket::UNIX->new(                  
	Type => SOCK_STREAM,                              
   Peer => "/tmp/display_log_socket",                         
);                                                   
$display->print("Starting test...");

`poke 0x8000100c 0x0`; # turn off caching for DMA
`poke 0x8000300c 0x0`; # turn off caching for DMA
initScaleCorrection();
setDacConfig(0x37);
setDacConfig(0x3f);
my $errorFlag = testVoltages(); 
$errorFlag |= testDigitalIOs();
$errorFlag |= testPullUps(0);
if ($rtbox3)
{
   $errorFlag |= testPullUps(1);
}
$errorFlag |= testAnalogIOs();
#$errorFlag |= testAnalogInputScaleOffset();
$errorFlag |= testSFP();
$errorFlag |= testCAN();
$errorFlag |= testResolver(0);
if ($rtbox3)
{
   $errorFlag |= testResolver(1);
}
$errorFlag |= testEthernet();


if ($errorFlag)
{
   print "\nTEST FAILED.\n";
   $display->print("TEST FAILED!");
   #setLedOutput(0x1);
}
else
{
   print "\nTest passed.\n";
   $display->print("All tests passed.");
   #setLedOutput(0x4);
}


