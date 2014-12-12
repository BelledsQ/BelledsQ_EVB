package Digest::MD5;

use strict;
use vars qw($VERSION @ISA @EXPORT_OK);

$VERSION = '2.36_01';  # $Date: 2005/11/30 13:46:47 $

require Exporter;
*import = \&Exporter::import;
@EXPORT_OK = qw(md5 md5_hex md5_base64);

eval {
    require Digest::base;
    push(@ISA, 'Digest::base');
};
if ($@) {
    my $err = $@;
    *add_bits = sub { die $err };
}


eval {
    require XSLoader;
    XSLoader::load('Digest::MD5', $VERSION);
};
if ($@) {
    my $olderr = $@;
    eval {
	# Try to load the pure perl version
	require Digest::Perl::MD5;

	Digest::Perl::MD5->import(qw(md5 md5_hex md5_base64));
	push(@ISA, "Digest::Perl::MD5");  # make OO interface work
    };
    if ($@) {
	# restore the original error
	die $olderr;
    }
}
else {
    *reset = \&new;
}

1;
__END__

