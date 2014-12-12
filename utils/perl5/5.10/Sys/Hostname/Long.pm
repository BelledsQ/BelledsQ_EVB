package Sys::Hostname::Long;
use strict;
use Carp;

require Exporter;
use Sys::Hostname;

@Sys::Hostname::Long::ISA     = qw/ Exporter Sys::Hostname /;

use vars qw(@EXPORT $VERSION $hostlong %dispatch $lastdispatch);
@EXPORT  = qw/ hostname_long /;
$VERSION = '1.4';

%dispatch = (

	'gethostbyname' => {
		'title' => 'Get Host by Name',
		'description' => '',
		'exec' => sub {
			return gethostbyname('localhost');
		},
	},

	'exec_hostname' => {
		'title' => 'Execute "hostname"',
		'description' => '',
		'exec' => sub {
			my $tmp = `hostname`;
			$tmp =~ tr/\0\r\n//d;
			return $tmp;
		},
	},

	'win32_registry1' => {
		'title' => 'WIN32 Registry',
		'description' => 'LMachine/System/CurrentControlSet/Service/VxD/MSTCP/Domain',
		'exec' => sub {
			return eval q{
				use Win32::TieRegistry ( TiedHash => '%RegistryHash' );
				$RegistryHash{'LMachine'}{'System'}{'CurrentControlSet'}{'Services'}{'VxD'}{'MSTCP'}{'Domain'}; 
			};
		},
	},

	'uname' => {
		'title' => 'POSIX::unae',
		'description' => '',
		'exec' => sub {
			return eval {
				local $SIG{__DIE__};
				require POSIX;
				(POSIX::uname())[1];
			};
		},
	},

	# XXX This is the same as above - what happened to the other one !!!
	'win32_registry2' => {
		'title' => 'WIN32 Registry',
		'description' => 'LMachine/System/CurrentControlSet/Services/VxD/MSTCP/Domain',
		'exec' => sub {
			return eval q{
				use Win32::TieRegistry ( TiedHash => '%RegistryHash' );
				$RegistryHash{'LMachine'}{'System'}{'CurrentControlSet'}{'Services'}{'VxD'}{'MSTCP'}{'Domain'}; 
			};
		},
	},

	'exec_hostname_fqdn' => {
		'title' => 'Execute "hostname --fqdn"',
		'description' => '',
		'exec' => sub {
			# Skip for Solaris, and only run as non-root
			my $tmp;
			if ($< == 0) {
				$tmp = `su nobody -c "hostname --fqdn"`;
			} else {
				$tmp = `hostname --fqdn`;
			}
			$tmp =~ tr/\0\r\n//d;
			return $tmp;
		},
	},

	'exec_hostname_domainname' => {
		'title' => 'Execute "hostname" and "domainname"',
		'description' => '',
		'exec' => sub {
			my $tmp = `hostname` . '.' . `domainname`;
			$tmp =~ tr/\0\r\n//d;
			return $tmp;
		},
	},


	'network' => {
		'title' => 'Network Socket hostname (not DNS)',
		'description' => '',
		'exec' => sub {
			return eval q{
				use IO::Socket;
				my $s = IO::Socket::INET->new(
					# m.root-servers.net (a remote IP number)
					PeerAddr => '202.12.27.33',
					# random safe port
					PeerPort => 2000,
					# We don't actually want to connect
					Proto => 'udp',
				) or die "Faile socket - $!";
				gethostbyaddr($s->sockaddr(), AF_INET);
			};
		},
	},

	'ip' => {
		'title' => 'Network Socket IP then Hostname via DNS',
		'description' => '',
		'exec' => sub {
			return eval q{
				use IO::Socket;
				my $s = IO::Socket::INET->new(
					# m.root-servers.net (a remote IP number)
					PeerAddr => '202.12.27.33',
					# random safe port
					PeerPort => 2000,
					# We don't actually want to connect
					Proto => 'udp',
				) or die "Faile socket - $!";
				$s->sockhost;
			};
		},
	},

);

sub dispatcher {
	my ($method, @rest) = @_;
	$lastdispatch = $method;
	return $dispatch{$method}{exec}(@rest);
}

sub dispatch_keys {
	return sort keys %dispatch;
}

sub dispatch_title {
	return $dispatch{$_[0]}{title};
}

sub dispatch_description {
	return $dispatch{$_[0]}{description};
}

sub hostname_long {
	return $hostlong if defined $hostlong; 	# Cached copy (takes a while to lookup sometimes)
	my ($ip, $debug) = @_;

	$hostlong = dispatcher('uname');

	unless ($hostlong =~ m|.*\..*|) {
		if ($^O eq 'MacOS') {
			# http://bumppo.net/lists/macperl/1999/03/msg00282.html 
			#	suggests that it will work (checking localhost) on both
			#	Mac and Windows. 
			#	Personally this makes no sense what so ever as 
			$hostlong = dispatcher('gethostbyname');

		} elsif ($^O eq 'IRIX') {	# XXX Patter match string !
			$hostlong = dispatcher('exec_hostname');

		} elsif ($^O eq 'cygwin') {
			$hostlong = dispatcher('win32_registry1');

		} elsif ($^O eq 'MSWin32') {
			$hostlong = dispatcher('win32_registry2');

		} elsif ($^O =~ m/(bsd|nto)/i) {
			$hostlong = dispatcher('exec_hostname');

		# (covered above) } elsif ($^O eq "darwin") {
		#	$hostlong = dispatcher('uname');

		} elsif ($^O eq 'solaris') {
			$hostlong = dispatcher('exec_hostname_domainname');
 
		} else {
			$hostlong = dispatcher('exec_hostname_fqdn');
		}

		if (!defined($hostlong) || $hostlong eq "") {
			# FALL BACK - Requires working internet and DNS and reverse
			# lookups of your IP number.
			$hostlong = dispatcher('network');
		}

		if ($ip && !defined($hostlong) || $hostlong eq "") {
			$hostlong = dispatcher('ip');
		}
	}
	warn "Sys::Hostname::Long - Last Dispatch method = $lastdispatch" if ($debug);
	return $hostlong;
}

1;

__END__

