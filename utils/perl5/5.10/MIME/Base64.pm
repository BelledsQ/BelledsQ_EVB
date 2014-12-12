package MIME::Base64;


use strict;
use vars qw(@ISA @EXPORT $VERSION);

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(encode_base64 decode_base64);

$VERSION = '3.07_01';

require XSLoader;
XSLoader::load('MIME::Base64', $VERSION);

*encode = \&encode_base64;
*decode = \&decode_base64;

1;

__END__

