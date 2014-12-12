
package URI::ldap;

use strict;

use vars qw(@ISA $VERSION);
$VERSION = "1.11";

require URI::_server;
require URI::_ldap;
@ISA=qw(URI::_ldap URI::_server);

sub default_port { 389 }

sub _nonldap_canonical {
    my $self = shift;
    $self->URI::_server::canonical(@_);
}

1;

__END__

