package Tie::Hash::NamedCapture;

our $VERSION = "0.06";


my ($one, $all) = Tie::Hash::NamedCapture::flags();

sub TIEHASH {
    my ($pkg, %arg) = @_;
    my $flag = $arg{all} ? $all : $one;
    bless \$flag => $pkg;
}

tie %+, __PACKAGE__;
tie %-, __PACKAGE__, all => 1;

1;

__END__

