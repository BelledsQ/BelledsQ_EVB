package Tie::Hash;

our $VERSION = '1.02';


use Carp;
use warnings::register;

sub new {
    my $pkg = shift;
    $pkg->TIEHASH(@_);
}


sub TIEHASH {
    my $pkg = shift;
    if (defined &{"${pkg}::new"}) {
	warnings::warnif("WARNING: calling ${pkg}->new since ${pkg}->TIEHASH is missing");
	$pkg->new(@_);
    }
    else {
	croak "$pkg doesn't define a TIEHASH method";
    }
}

sub EXISTS {
    my $pkg = ref $_[0];
    croak "$pkg doesn't define an EXISTS method";
}

sub CLEAR {
    my $self = shift;
    my $key = $self->FIRSTKEY(@_);
    my @keys;

    while (defined $key) {
	push @keys, $key;
	$key = $self->NEXTKEY(@_, $key);
    }
    foreach $key (@keys) {
	$self->DELETE(@_, $key);
    }
}


package Tie::StdHash;

sub TIEHASH  { bless {}, $_[0] }
sub STORE    { $_[0]->{$_[1]} = $_[2] }
sub FETCH    { $_[0]->{$_[1]} }
sub FIRSTKEY { my $a = scalar keys %{$_[0]}; each %{$_[0]} }
sub NEXTKEY  { each %{$_[0]} }
sub EXISTS   { exists $_[0]->{$_[1]} }
sub DELETE   { delete $_[0]->{$_[1]} }
sub CLEAR    { %{$_[0]} = () }
sub SCALAR   { scalar %{$_[0]} }

package Tie::ExtraHash;

sub TIEHASH  { my $p = shift; bless [{}, @_], $p }
sub STORE    { $_[0][0]{$_[1]} = $_[2] }
sub FETCH    { $_[0][0]{$_[1]} }
sub FIRSTKEY { my $a = scalar keys %{$_[0][0]}; each %{$_[0][0]} }
sub NEXTKEY  { each %{$_[0][0]} }
sub EXISTS   { exists $_[0][0]->{$_[1]} }
sub DELETE   { delete $_[0][0]->{$_[1]} }
sub CLEAR    { %{$_[0][0]} = () }
sub SCALAR   { scalar %{$_[0][0]} }

1;
