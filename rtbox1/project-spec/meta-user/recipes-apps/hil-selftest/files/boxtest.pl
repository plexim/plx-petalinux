#!/usr/bin/perl

use strict;
use Time::HiRes qw(usleep);
use Fcntl;
use Errno qw ( EAGAIN );

my $scaleCorrection = 1.0;
my $hwVer;

sub isRtboxCE()
{
   my $ret = 1;
   if (-e '/sys/bus/i2c/devices/i2c-1')
   {
      $ret = 0;
   }
   return $ret;
}

sub initScaleCorrection()
{
   $scaleCorrection = 1.0;
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
   if ($hwVer > (1 << 16))
   {
      $scaleCorrection = 1.024;
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

sub zynq_base()
{
   opendir(my $dh, "/sys/bus/platform/devices/e000a000.gpio/gpio");
   my @entries = grep { /^gpiochip/ } readdir($dh);
   closedir($dh);
   my @baseaddr = ($entries[0] =~ m/gpiochip(\d+)/);
   return $baseaddr[0];
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

sub getDigitalInput($)
{
   my ($idx) = @_;
   my $val = peek(hex("0xffff1050"));
   return ($val >> $idx) & 1;
}

sub enableDigitalOuts()
{
   my $base = zynq_base();
   setDigitalOutput($base+91, 0);
}

sub disableDigitalOuts()
{
   my $base = zynq_base();
   setDigitalOutput($base+91, 1);
}

sub setDigitalOuts3V()
{
   my $base = zynq_base();
   setDigitalOutput($base+90, 0);
}

sub setDigitalOuts5V()
{
   my $base = zynq_base();
   setDigitalOutput($base+90, 1);
}

sub enableAnalogOuts()
{
   `poke 0x43c00004 1`;
}

sub disableAnalogOuts()
{
   `poke 0x43c00004 0`;
}

sub setAnalogInputScale($$)
{
   my ($idx, $val) = @_;
   my $scaledVal = $val * $scaleCorrection * 10/32767;
   my $intVal = unpack("L", pack("f", $scaledVal));
   `poke 0x43c90000 $idx`;
   `poke 0x43c90004 $intVal`;
}

sub setAnalogInputOffset($$)
{
   my ($idx, $val) = @_;
   my $intVal = unpack("L", pack("f", $val));
   `poke 0x43ca0000 $idx`;
   `poke 0x43ca0004 $intVal`;
}

sub setAnalogOutput($$)
{
   my @idxLookup = (0, 4, 8, 12, 2, 6, 10, 14, 1, 5, 9, 13, 3, 7, 11, 15);
   my ($idx, $val) = @_;
   my $intVal = unpack("L", pack("f", $val));
   my $addr = 0xffff0000 + $idxLookup[$idx] * 4;
   `poke $addr, $intVal`;
   $addr = 0xffff2000 + $idxLookup[$idx] * 4;
   `poke $addr, $intVal`;
}

sub getAnalogInput($)
{
   my ($idx) = @_;
   my $addr = 0xffff10b8 + $idx * 4;
   my $intVal = peek($addr);
   return unpack("f", pack("L", $intVal));
}

sub setSFPOut($$)
{
   my ($idx, $val) = @_;
   my $intVal = unpack("L", pack("f", $val));
   my $addr = 0xffff0110 + $idx * 8;
   `poke $addr, $intVal`;
   $addr = 0xffff2110 + $idx * 8;
   `poke $addr, $intVal`;
}

sub getSFPIn($)
{
   my ($idx) = @_;
   my $addr = 0xffff1000 + $idx * 16 + 8;
   my $intVal = peek($addr);
   return unpack("f", pack("L", $intVal));
}

sub getLedInput()
{
   my $val = peek(0xe000a06c) >> 16;
   return $val & 0x0f;
}

sub setLedOutput($)
{
   my ($val) = @_;
   `poke 0x41200000 $val`;
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
   makeOutput(zynq_base() + 90);
   setDigitalOuts3V();
   enableDigitalOuts();
   my @idx = (0..31);
   my $i;
   my $errorFlag = 0;
   my $base = zynq_base() + 54;
   print "Testing digital I/Os 3.3V";
   for $i (@idx)
   {
      setDigitalOutput($base+$i, 0);
   }
   for $i (@idx)
   {
      setDigitalOutput($base+$i, 1);
      usleep(10);
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
      setDigitalOutput($base + $i, 0);
   }
   setDigitalOuts5V();
   print " 5V ...";
   for $i (@idx)
   {
      setDigitalOutput($base+$i, 1);
      usleep(10);
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
      setDigitalOutput($base + $i, 0);
   }
   if ($errorFlag)
   {
      print " FAIL\n";
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag
}

sub testPullUps
{
   my @idx = (0..31);
   my $zynqbase = zynq_base() + 54;
   my $i;
   for $i (@idx)
   {
      setDigitalOutput($zynqbase+$i, 1); # really put digital outputs into tristate
   }
   disableDigitalOuts();
   my $errorFlag = 0;
   print "Testing pull-ups ...";
   my $base = i2c_to_base(74);
   for $i (@idx)
   {
      makeOutput($base + $i);
      setDigitalOutput($base + $i, 0);
   }
   for $i (@idx)
   {
      setDigitalOutput($base + $i, 1);
      usleep(100);
      for my $j (@idx)
      {
         if (($i != $j) && (getDigitalInput($j) == 1))
         {
            print "\nERROR: Input $j is high when pull-up $i is set high";
            $errorFlag = 1;
         }
         elsif (($i == $j) && (getDigitalInput($j) == 0))
         {
            print "\nERROR: Input $j is low when pull-up $i is set high";
            $errorFlag = 1;
         }
      }
      setDigitalOutput($base + $i, 0);
   }
   for $i (@idx)
   {
      makeInput($base + $i);
   }
   if ($errorFlag)
   {
      print " FAIL\n";
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag
}

sub testAnalogIOs
{
   my $errorFlag = 0;
   my @idx = (0..15);
   my $zynqbase = zynq_base();
   if (isRtboxCE())
   {
      @idx = (0..7);
   }
   my $i;

   exportPin($zynqbase + 87); # Analog input voltage
   makeOutput($zynqbase + 87);
   setDigitalOutput($zynqbase + 87, 0); # +-10V
   print "Testing analog I/Os ...";
   enableAnalogOuts();
   for $i (@idx)                      
   {                                                                     
      setAnalogInputScale($i, 1);
      setAnalogInputOffset($i, 0);
   }                                 
                                                      
   my $scale = 1.5;
   if (isRtboxCE())
   {
      $scale = 0.7;
   }
   for $i (@idx)
   {
      setAnalogOutput($i, $i/$scale);
      if (isRtboxCE()) 
      {
         setAnalogOutput($i+8, 0);
      }
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/$scale;
      my $input = getAveragedAnalogInput($i, 3);
      if (abs($input - $setValue) > 0.05)
      {
         print "\nERROR: Output $i is $input Volt, expected $setValue Volt";
         $errorFlag = 1;
      }
      elsif (abs($input - $setValue) > 0.02)
      {
         print "\nWarning: Output $i is $input Volt, expected $setValue Volt";
      }
   }
   if (isRtboxCE())
   {
      for $i (@idx)
      {
         setAnalogOutput($i, 0);
         setAnalogOutput($i+8, $i/$scale);
      }
      usleep(10);
      for $i (@idx)
      {
         my $setValue = $i/$scale;
         my $input = -getAveragedAnalogInput($i, 3);
         if (abs($input - $setValue) > 0.05)
         {
            print "\nERROR: Output ".($i+8)." is $input Volt, expected $setValue Volt";
            $errorFlag = 1;
         }
         elsif (abs($input - $setValue) > 0.02)
         {
            print "\nWarning: Output ".($i+8)." is $input Volt, expected $setValue Volt";
         }
      }   
   }

   for $i (@idx)
   {
      setAnalogOutput($i, $i/$scale-10);
      if (isRtboxCE()) 
      {
         setAnalogOutput($i+8, 0);
      }
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/$scale-10;
      my $input = getAveragedAnalogInput($i, 3);
      if (abs($input - $setValue) > 0.05)
      {
         print "\nERROR: Output $i is $input Volt, expected $setValue Volt";
         $errorFlag = 1;
      }
      elsif (abs($input - $setValue) > 0.02)
      {
         print "\nWarning: Output $i is $input Volt, expected $setValue Volt";
      }
   }
   if (isRtboxCE())
   {
      for $i (@idx)
      {
         setAnalogOutput($i, 0);
         setAnalogOutput($i+8, $i/$scale-10);
      }
      usleep(10);
      for $i (@idx)
      {
         my $setValue = $i/$scale-10;
         my $input = -getAveragedAnalogInput($i, 3);
         if (abs($input - $setValue) > 0.05)
         {
            print "\nERROR: Output ".($i+8)." is $input Volt, expected $setValue Volt";
            $errorFlag = 1;
         }
         elsif (abs($input - $setValue) > 0.02)
         {
            print "\nWarning: Output ".($i+8)." is $input Volt, expected $setValue Volt";
         }
      }   
   }


   for $i (@idx)                                                             
   {                                                                         
      setAnalogOutput($i, -$i/$scale);                                          
      if (isRtboxCE()) 
      {
         setAnalogOutput($i+8, 0);
      }
   }                                                                         
   usleep(10);                                                               
   for $i (@idx)                                                             
   {                                                                         
      my $setValue = -$i/$scale;                                                
      my $input = getAveragedAnalogInput($i, 3);                             
      if (abs($input - $setValue) > 0.05)                                   
      {                                                                      
         print "\nERROR: Output $i is $input Volt, expected $setValue Volt";  
         $errorFlag = 1;                                                     
      }                                                                      
      elsif (abs($input - $setValue) > 0.02)                                 
      {                                                                      
         print "\nWarning: Output $i is $input Volt, expected $setValue Volt";
      }                                                                      
   }
   if (isRtboxCE())
   {
      for $i (@idx)
      {
         setAnalogOutput($i, 0);
         setAnalogOutput($i+8, -$i/$scale);
      }
      usleep(10);
      for $i (@idx)
      {
         my $setValue = -$i/$scale;
         my $input = -getAveragedAnalogInput($i, 3);
         if (abs($input - $setValue) > 0.05)
         {
            print "\nERROR: Output ".($i+8)." is $input Volt, expected $setValue Volt";
            $errorFlag = 1;
         }
         elsif (abs($input - $setValue) > 0.02)
         {
            print "\nWarning: Output ".($i+8)." is $input Volt, expected $setValue Volt";
         }
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
   setDigitalOutput($zynqbase + 87, 1); # +-5V
   usleep(100000);
   for $i (@idx)
   {
      setAnalogOutput($i, $i/(2*$scale));
      if (isRtboxCE()) 
      {
         setAnalogOutput($i+8, 0);
      }
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/(2*$scale);
      my $input = getAveragedAnalogInput($i, 3)/2;
      if (abs($input - $setValue) > 0.05)
      {
         print "\nERROR: Output $i is $input Volt, expected $setValue Volt";
         $errorFlag = 1;
      }
      elsif (abs($input - $setValue) > 0.02)
      {
         print "\nWarning: Output $i is $input Volt, expected $setValue Volt";
      }
   }   
   if (isRtboxCE())
   {
      for $i (@idx)
      {
         setAnalogOutput($i, 0);
         setAnalogOutput($i+8, $i/(2*$scale));
      }
      usleep(10);
      for $i (@idx)
      {
         my $setValue = $i/(2*$scale);
         my $input = -getAveragedAnalogInput($i, 3)/2;
         if (abs($input - $setValue) > 0.05)
         {
            print "\nERROR: Output ".($i+8)." is $input Volt, expected $setValue Volt";
            $errorFlag = 1;
         }
         elsif (abs($input - $setValue) > 0.02)
         {
            print "\nWarning: Output ".($i+8)." is $input Volt, expected $setValue Volt";
         }
      }   
   }

   disableAnalogOuts();
   unexportPin($zynqbase + 87);
   if ($errorFlag)
   {
      print " FAIL\n";
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
   my $zynqbase = zynq_base();
   my @idx = (0..15);
   my $i;

   exportPin($zynqbase + 87); # Analog input voltage
   makeOutput($zynqbase + 87);
   setDigitalOutput($zynqbase + 87, 0); # +-10V
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
      my $setValue = $i/3;
      my $input = getAveragedAnalogInput($i, 3);
      if (abs($input - $setValue) > 0.05)
      {
         print "\nERROR: Output $i is $input Volt, expected $setValue Volt";
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
      setAnalogInputOffset($i, $i/1.5);
   }
   usleep(10);
   for $i (@idx)
   {
      my $setValue = $i/1.5;
      my $input = getAveragedAnalogInput($i, 3);
      if (abs($input - $setValue) > 0.05)
      {
         print "\nERROR: Output $i is $input Volt, expected $setValue Volt";
         $errorFlagOffset = 1;
      }
   }
   for $i (@idx)
   {
      setAnalogInputScale($i, 1);
      setAnalogInputOffset($i, 0);
   }
   disableAnalogOuts();
   unexportPin($zynqbase + 87);
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

sub testLeds()
{
   my $errorFlag = 0;
   my @idx = (0..3);
   my $i;

   print "Testing LEDs ...";
   my $oldLedVal = getLedInput();
   for $i (@idx)
   {
      my $val = 1 << $i;
      setLedOutput($val);
      usleep(10);
      my $input = getLedInput();
      for my $j (@idx)
      {
         if ($i != $j && ($input & (1 << $j)) != 0) 
         {
            print "\nERROR: LED Feedback $j is high when LED $i is set high";
            $errorFlag = 1;
         }
         elsif ($i == $j && ($input & (1 << $j)) == 0) 
         {
            print "\nERROR: LED Feedback $j is low when LED $i is set high";
            $errorFlag = 1;
         }
      }
   }
   setLedOutput($oldLedVal);
   if ($errorFlag)
   {
      print " FAIL\n";
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag
}

sub testSFP
{
   my $errorFlag = 0;
   my @peerIdx = (1, 0, 3, 2);
   my $testA = 5.13457489013672;
   my $testB = 8.56934523341124e-09;
   my $i;
   print "Testing SFP ...";
   for $i (0..3)
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
   }
   else
   {
      print " ok\n";
   }
   return $errorFlag
}

sub testCAN
{
   print "Testing CAN ...";
   `/sbin/ip link set down can0`;
   `/sbin/ip link set down can1`;
   # enable termination
   my $pin = 850;
   if (isRtboxCE())
   {
      $pin = 850 + 16;
   }
   exportPin($pin);
   exportPin($pin+1);
   makeOutput($pin);
   makeOutput($pin+1);
   setDigitalOutput($pin, 1);
   setDigitalOutput($pin+1, 1);
   # enable internal CAN interface
   `poke 0x43d60000 0x40`;
   `poke 0x43d60004 0x40`;
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
   my $volt12ref = 12800;
   my $volt12disp = 13;
   if (isRtboxCE())
   {
      $volt12ref = 11900;
      $volt12disp = 12;
   }
   print "Testing Voltages: ${volt12disp}V ";
   my $errorFlag = 0;
   my $volt12 = readLineFile($hwmon . "in3_input");
   if ($volt12 < $volt12ref || $volt12 > $volt12ref + 300)
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
   }
   return $errorFlag;
}

initScaleCorrection();
my $errorFlag = testDigitalIOs();
$errorFlag |= testPullUps();
$errorFlag |= testAnalogIOs();
if (!isRtboxCE())
{
   $errorFlag |= testAnalogInputScaleOffset();
   $errorFlag |= testSFP();
}
$errorFlag |= testLeds();
if ($hwVer > 0x00010001 || isRtboxCE())
{
   $errorFlag |= testCAN();
   $errorFlag |= testVoltages();
}

if ($errorFlag)
{
   print "\nTEST FAILED.\n";
   setLedOutput(0x1);
}
else
{
   print "\nTest passed.\n";
   setLedOutput(0x4);
}


