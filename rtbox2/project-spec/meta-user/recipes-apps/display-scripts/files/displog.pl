#!/usr/bin/perl -wT

use IO::Socket::UNIX;

my $socket_path = "/tmp/display_log_socket";

if (-e $socket_path)
{
    my $client = IO::Socket::UNIX->new(
        Type => SOCK_STREAM(),
        Peer => $socket_path,
    ) or die "\nCannot open socket: $!\n\n";

    my $message = join("", <STDIN>);
    $client->print($message);
}
else
{
    print STDERR "\nCould not find socket '$socket_path'. Is the scopeserver running?\n\n";
}

