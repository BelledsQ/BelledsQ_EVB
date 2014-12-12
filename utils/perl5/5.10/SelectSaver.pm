package SelectSaver;

our $VERSION = '1.01';


require 5.000;
use Carp;
use Symbol;

sub new {
    @_ >= 1 && @_ <= 2 or croak 'usage: new SelectSaver [FILEHANDLE]';
    my $fh = select;
    my $self = bless \$fh, $_[0];
    select qualify($_[1], caller) if @_ > 1;
    $self;
}

sub DESTROY {
    my $self = $_[0];
    select $$self;
}

1;
