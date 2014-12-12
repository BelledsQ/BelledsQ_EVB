package URI::ssh;
require URI::_login;
@ISA=qw(URI::_login);


sub default_port { 22 }

1;
