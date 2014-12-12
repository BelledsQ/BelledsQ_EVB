package Net::HTTP;

use strict;
use vars qw($VERSION @ISA);

$VERSION = "5.812";
eval { require IO::Socket::INET } || require IO::Socket;
require Net::HTTP::Methods;
require Carp;

@ISA=qw(IO::Socket::INET Net::HTTP::Methods);

sub new {
    my $class = shift;
    Carp::croak("No Host option provided") unless @_;
    $class->SUPER::new(@_);
}

sub configure {
    my($self, $cnf) = @_;
    $self->http_configure($cnf);
}

sub http_connect {
    my($self, $cnf) = @_;
    $self->SUPER::configure($cnf);
}

1;

__END__

