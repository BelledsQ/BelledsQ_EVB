package Crypt::OpenSSL::RSA;

use strict;
use Carp;

use vars qw ($VERSION @ISA @EXPORT @EXPORT_OK $AUTOLOAD);

require DynaLoader;
require AutoLoader;

@ISA = qw(DynaLoader);

$VERSION = '0.26';

bootstrap Crypt::OpenSSL::RSA $VERSION;

BEGIN { eval { require Crypt::OpenSSL::Bignum; }; }

1;

__END__


sub new_public_key
{
    my ($proto, $p_key_string) = @_;
    if ($p_key_string =~ /^-----BEGIN RSA PUBLIC KEY-----/)
    {
        return $proto->_new_public_key_pkcs1($p_key_string);
    }
    elsif ($p_key_string =~ /^-----BEGIN PUBLIC KEY-----/)
    {
        return $proto->_new_public_key_x509($p_key_string);
    }
    else
    {
        croak "unrecognized key format";
    }
}


sub new_key_from_parameters
{
    my($proto, $n, $e, $d, $p, $q) = @_;
    return $proto->_new_key_from_parameters
        (map { $_ ? $_->pointer_copy() : 0 } $n, $e, $d, $p, $q);
}


sub import_random_seed
{
    until (_random_status())
    {
        _random_seed(Crypt::OpenSSL::Random::random_bytes(20));
    }
}


sub get_key_parameters
{
    return map { $_ ? Crypt::OpenSSL::Bignum->bless_pointer($_) : undef }
        shift->_get_key_parameters();
}

