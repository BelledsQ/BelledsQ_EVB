package URI::rsync;  # http://rsync.samba.org/


require URI::_server;
require URI::_userpass;

@ISA=qw(URI::_server URI::_userpass);

sub default_port { 873 }

1;
