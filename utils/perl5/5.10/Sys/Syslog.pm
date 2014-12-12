package Sys::Syslog;
use strict;
use warnings::register;
use Carp;
use Fcntl qw(O_WRONLY);
use File::Basename;
use POSIX qw(strftime setlocale LC_TIME);
use Socket ':all';
require 5.005;
require Exporter;

{   no strict 'vars';
    $VERSION = '0.22';
    @ISA = qw(Exporter);

    %EXPORT_TAGS = (
        standard => [qw(openlog syslog closelog setlogmask)],
        extended => [qw(setlogsock)],
        macros => [
            # levels
            qw(
                LOG_ALERT LOG_CRIT LOG_DEBUG LOG_EMERG LOG_ERR 
                LOG_INFO LOG_NOTICE LOG_WARNING
            ), 

            # standard facilities
            qw(
                LOG_AUTH LOG_AUTHPRIV LOG_CRON LOG_DAEMON LOG_FTP LOG_KERN
                LOG_LOCAL0 LOG_LOCAL1 LOG_LOCAL2 LOG_LOCAL3 LOG_LOCAL4
                LOG_LOCAL5 LOG_LOCAL6 LOG_LOCAL7 LOG_LPR LOG_MAIL LOG_NEWS
                LOG_SYSLOG LOG_USER LOG_UUCP
            ),
            # Mac OS X specific facilities
            qw( LOG_INSTALL LOG_LAUNCHD LOG_NETINFO LOG_RAS LOG_REMOTEAUTH ),
            # modern BSD specific facilities
            qw( LOG_CONSOLE LOG_NTP LOG_SECURITY ),
            # IRIX specific facilities
            qw( LOG_AUDIT LOG_LFMT ),

            # options
            qw(
                LOG_CONS LOG_PID LOG_NDELAY LOG_NOWAIT LOG_ODELAY LOG_PERROR 
            ), 

            # others macros
            qw(
                LOG_FACMASK LOG_NFACILITIES LOG_PRIMASK 
                LOG_MASK LOG_UPTO
            ), 
        ],
    );

    @EXPORT = (
        @{$EXPORT_TAGS{standard}}, 
    );

    @EXPORT_OK = (
        @{$EXPORT_TAGS{extended}}, 
        @{$EXPORT_TAGS{macros}}, 
    );

    eval {
        require XSLoader;
        XSLoader::load('Sys::Syslog', $VERSION);
        1
    } or do {
        require DynaLoader;
        push @ISA, 'DynaLoader';
        bootstrap Sys::Syslog $VERSION;
    };
}


use vars qw($host);             # host to send syslog messages to (see notes at end)

use vars qw($facility);
my $connected = 0;              # flag to indicate if we're connected or not
my $syslog_send;                # coderef of the function used to send messages
my $syslog_path = undef;        # syslog path for "stream" and "unix" mechanisms
my $syslog_xobj = undef;        # if defined, holds the external object used to send messages
my $transmit_ok = 0;            # flag to indicate if the last message was transmited
my $current_proto = undef;      # current mechanism used to transmit messages
my $ident = '';                 # identifiant prepended to each message
$facility = '';                 # current facility
my $maskpri = LOG_UPTO(&LOG_DEBUG);     # current log mask

my %options = (
    ndelay  => 0, 
    nofatal => 0, 
    nowait  => 0, 
    perror  => 0, 
    pid     => 0, 
);

my @connectMethods = qw(native tcp udp unix pipe stream console);
if ($^O =~ /^(freebsd|linux)$/) {
    @connectMethods = grep { $_ ne 'udp' } @connectMethods;
}

EVENTLOG: {
    # use EventLog on Win32
    my $is_Win32 = $^O =~ /Win32/i;

    # some applications are trying to be too smart
    # yes I'm speaking of YOU, SpamAssassin, grr..
    local($SIG{__DIE__}, $SIG{__WARN__}, $@);

    if (eval "use Sys::Syslog::Win32; 1") {
        unshift @connectMethods, 'eventlog';
    }
    elsif ($is_Win32) {
        warn $@;
    }
}

my @defaultMethods = @connectMethods;
my @fallbackMethods = ();

my $err_sub = $options{nofatal} ? \&warnings::warnif : \&croak;


sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.
    no strict 'vars';
    my $constname;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "Sys::Syslog::constant() not defined" if $constname eq 'constant';
    my ($error, $val) = constant($constname);
    croak $error if $error;
    no strict 'refs';
    *$AUTOLOAD = sub { $val };
    goto &$AUTOLOAD;
}


sub openlog {
    ($ident, my $logopt, $facility) = @_;

    # default values
    $ident    ||= basename($0) || getlogin() || getpwuid($<) || 'syslog';
    $logopt   ||= '';
    $facility ||= LOG_USER();

    for my $opt (split /\b/, $logopt) {
        $options{$opt} = 1 if exists $options{$opt}
    }

    $err_sub = $options{nofatal} ? \&warnings::warnif : \&croak;
    return 1 unless $options{ndelay};
    connect_log();
} 

sub closelog {
    $facility = $ident = '';
    disconnect_log();
} 

sub setlogmask {
    my $oldmask = $maskpri;
    $maskpri = shift unless $_[0] == 0;
    $oldmask;
}
 
sub setlogsock {
    my $setsock = shift;
    $syslog_path = shift;
    disconnect_log() if $connected;
    $transmit_ok = 0;
    @fallbackMethods = ();
    @connectMethods = @defaultMethods;

    if (ref $setsock eq 'ARRAY') {
	@connectMethods = @$setsock;

    } elsif (lc $setsock eq 'stream') {
	if (not defined $syslog_path) {
	    my @try = qw(/dev/log /dev/conslog);

            if (length &_PATH_LOG) {        # Undefined _PATH_LOG is "".
		unshift @try, &_PATH_LOG;
            }

	    for my $try (@try) {
		if (-w $try) {
		    $syslog_path = $try;
		    last;
		}
	    }

            if (not defined $syslog_path) {
                warnings::warnif "stream passed to setlogsock, but could not find any device";
                return undef
            }
        }

	if (not -w $syslog_path) {
            warnings::warnif "stream passed to setlogsock, but $syslog_path is not writable";
	    return undef;
	} else {
            @connectMethods = qw(stream);
	}

    } elsif (lc $setsock eq 'unix') {
        if (length _PATH_LOG() || (defined $syslog_path && -w $syslog_path)) {
	    $syslog_path = _PATH_LOG() unless defined $syslog_path;
            @connectMethods = qw(unix);
        } else {
            warnings::warnif 'unix passed to setlogsock, but path not available';
	    return undef;
        }

    } elsif (lc $setsock eq 'pipe') {
        for my $path ($syslog_path, &_PATH_LOG, "/dev/log") {
            next unless defined $path and length $path and -w $path;
            $syslog_path = $path;
            last
        }

        if (not $syslog_path) {
            warnings::warnif "pipe passed to setlogsock, but path not available";
            return undef
        }

        @connectMethods = qw(pipe);

    } elsif (lc $setsock eq 'native') {
        @connectMethods = qw(native);

    } elsif (lc $setsock eq 'eventlog') {
        if (eval "use Win32::EventLog; 1") {
            @connectMethods = qw(eventlog);
        } else {
            warnings::warnif "eventlog passed to setlogsock, but no Win32 API available";
            $@ = "";
            return undef;
        }

    } elsif (lc $setsock eq 'tcp') {
	if (getservbyname('syslog', 'tcp') || getservbyname('syslogng', 'tcp')) {
            @connectMethods = qw(tcp);
	} else {
            warnings::warnif "tcp passed to setlogsock, but tcp service unavailable";
	    return undef;
	}

    } elsif (lc $setsock eq 'udp') {
	if (getservbyname('syslog', 'udp')) {
            @connectMethods = qw(udp);
	} else {
            warnings::warnif "udp passed to setlogsock, but udp service unavailable";
	    return undef;
	}

    } elsif (lc $setsock eq 'inet') {
	@connectMethods = ( 'tcp', 'udp' );

    } elsif (lc $setsock eq 'console') {
	@connectMethods = qw(console);

    } else {
        croak "Invalid argument passed to setlogsock; must be 'stream', 'pipe', ",
              "'unix', 'native', 'eventlog', 'tcp', 'udp' or 'inet'"
    }

    return 1;
}

sub syslog {
    my $priority = shift;
    my $mask = shift;
    my ($message, $buf);
    my (@words, $num, $numpri, $numfac, $sum);
    my $failed = undef;
    my $fail_time = undef;
    my $error = $!;

    # if $ident is undefined, it means openlog() wasn't previously called
    # so do it now in order to have sensible defaults
    openlog() unless $ident;

    local $facility = $facility;    # may need to change temporarily.

    croak "syslog: expecting argument \$priority" unless defined $priority;
    croak "syslog: expecting argument \$format"   unless defined $mask;

    @words = split(/\W+/, $priority, 2);    # Allow "level" or "level|facility".
    undef $numpri;
    undef $numfac;

    foreach (@words) {
	$num = xlate($_);		    # Translate word to number.
	if ($num < 0) {
	    croak "syslog: invalid level/facility: $_"
	}
	elsif ($num <= &LOG_PRIMASK) {
	    croak "syslog: too many levels given: $_" if defined $numpri;
	    $numpri = $num;
	    return 0 unless LOG_MASK($numpri) & $maskpri;
	}
	else {
	    croak "syslog: too many facilities given: $_" if defined $numfac;
	    $facility = $_;
	    $numfac = $num;
	}
    }

    croak "syslog: level must be given" unless defined $numpri;

    if (not defined $numfac) {  # Facility not specified in this call.
	$facility = 'user' unless $facility;
	$numfac = xlate($facility);
    }

    connect_log() unless $connected;

    if ($mask =~ /%m/) {
        # escape percent signs for sprintf()
        $error =~ s/%/%%/g if @_;
        # replace %m with $error, if preceded by an even number of percent signs
        $mask =~ s/(?<!%)((?:%%)*)%m/$1$error/g;
    }

    $mask .= "\n" unless $mask =~ /\n$/;
    $message = @_ ? sprintf($mask, @_) : $mask;

    # See CPAN-RT#24431. Opened on Apple Radar as bug #4944407 on 2007.01.21
    # Supposedly resolved on Leopard.
    chomp $message if $^O =~ /darwin/;

    if ($current_proto eq 'native') {
        $buf = $message;
    }
    elsif ($current_proto eq 'eventlog') {
        $buf = $message;
    }
    else {
        my $whoami = $ident;
        $whoami .= "[$$]" if $options{pid};

        $sum = $numpri + $numfac;
        my $oldlocale = setlocale(LC_TIME);
        setlocale(LC_TIME, 'C');
        my $timestamp = strftime "%b %e %T", localtime;
        setlocale(LC_TIME, $oldlocale);
        $buf = "<$sum>$timestamp $whoami: $message\0";
    }

    # handle PERROR option
    # "native" mechanism already handles it by itself
    if ($options{perror} and $current_proto ne 'native') {
        chomp $message;
        my $whoami = $ident;
        $whoami .= "[$$]" if $options{pid};
        print STDERR "$whoami: $message\n";
    }

    # it's possible that we'll get an error from sending
    # (e.g. if method is UDP and there is no UDP listener,
    # then we'll get ECONNREFUSED on the send). So what we
    # want to do at this point is to fallback onto a different
    # connection method.
    while (scalar @fallbackMethods || $syslog_send) {
	if ($failed && (time - $fail_time) > 60) {
	    # it's been a while... maybe things have been fixed
	    @fallbackMethods = ();
	    disconnect_log();
	    $transmit_ok = 0; # make it look like a fresh attempt
	    connect_log();
        }

	if ($connected && !connection_ok()) {
	    # Something was OK, but has now broken. Remember coz we'll
	    # want to go back to what used to be OK.
	    $failed = $current_proto unless $failed;
	    $fail_time = time;
	    disconnect_log();
	}

	connect_log() unless $connected;
	$failed = undef if ($current_proto && $failed && $current_proto eq $failed);

	if ($syslog_send) {
            if ($syslog_send->($buf, $numpri, $numfac)) {
		$transmit_ok++;
		return 1;
	    }
	    # typically doesn't happen, since errors are rare from write().
	    disconnect_log();
	}
    }
    # could not send, could not fallback onto a working
    # connection method. Lose.
    return 0;
}

sub _syslog_send_console {
    my ($buf) = @_;
    chop($buf); # delete the NUL from the end
    # The console print is a method which could block
    # so we do it in a child process and always return success
    # to the caller.
    if (my $pid = fork) {

	if ($options{nowait}) {
	    return 1;
	} else {
	    if (waitpid($pid, 0) >= 0) {
	    	return ($? >> 8);
	    } else {
		# it's possible that the caller has other
		# plans for SIGCHLD, so let's not interfere
		return 1;
	    }
	}
    } else {
        if (open(CONS, ">/dev/console")) {
	    my $ret = print CONS $buf . "\r";  # XXX: should this be \x0A ?
	    exit $ret if defined $pid;
	    close CONS;
	}
	exit if defined $pid;
    }
}

sub _syslog_send_stream {
    my ($buf) = @_;
    # XXX: this only works if the OS stream implementation makes a write 
    # look like a putmsg() with simple header. For instance it works on 
    # Solaris 8 but not Solaris 7.
    # To be correct, it should use a STREAMS API, but perl doesn't have one.
    return syswrite(SYSLOG, $buf, length($buf));
}

sub _syslog_send_pipe {
    my ($buf) = @_;
    return print SYSLOG $buf;
}

sub _syslog_send_socket {
    my ($buf) = @_;
    return syswrite(SYSLOG, $buf, length($buf));
    #return send(SYSLOG, $buf, 0);
}

sub _syslog_send_native {
    my ($buf, $numpri) = @_;
    syslog_xs($numpri, $buf);
    return 1;
}


sub xlate {
    my($name) = @_;
    return $name+0 if $name =~ /^\s*\d+\s*$/;
    $name = uc $name;
    $name = "LOG_$name" unless $name =~ /^LOG_/;
    $name = "Sys::Syslog::$name";
    # Can't have just eval { &$name } || -1 because some LOG_XXX may be zero.
    my $value = eval { no strict 'refs'; &$name };
    $@ = "";
    return defined $value ? $value : -1;
}


sub connect_log {
    @fallbackMethods = @connectMethods unless scalar @fallbackMethods;

    if ($transmit_ok && $current_proto) {
        # Retry what we were on, because it has worked in the past.
	unshift(@fallbackMethods, $current_proto);
    }

    $connected = 0;
    my @errs = ();
    my $proto = undef;

    while ($proto = shift @fallbackMethods) {
	no strict 'refs';
	my $fn = "connect_$proto";
	$connected = &$fn(\@errs) if defined &$fn;
	last if $connected;
    }

    $transmit_ok = 0;
    if ($connected) {
	$current_proto = $proto;
        my ($old) = select(SYSLOG); $| = 1; select($old);
    } else {
	@fallbackMethods = ();
        $err_sub->(join "\n\t- ", "no connection to syslog available", @errs);
        return undef;
    }
}

sub connect_tcp {
    my ($errs) = @_;

    my $tcp = getprotobyname('tcp');
    if (!defined $tcp) {
	push @$errs, "getprotobyname failed for tcp";
	return 0;
    }

    my $syslog = getservbyname('syslog', 'tcp');
    $syslog = getservbyname('syslogng', 'tcp') unless defined $syslog;
    if (!defined $syslog) {
	push @$errs, "getservbyname failed for syslog/tcp and syslogng/tcp";
	return 0;
    }

    my $addr;
    if (defined $host) {
        $addr = inet_aton($host);
        if (!$addr) {
	    push @$errs, "can't lookup $host";
	    return 0;
	}
    } else {
        $addr = INADDR_LOOPBACK;
    }
    $addr = sockaddr_in($syslog, $addr);

    if (!socket(SYSLOG, AF_INET, SOCK_STREAM, $tcp)) {
	push @$errs, "tcp socket: $!";
	return 0;
    }

    setsockopt(SYSLOG, SOL_SOCKET, SO_KEEPALIVE, 1);
    if (eval { IPPROTO_TCP() }) {
        # These constants don't exist in 5.005. They were added in 1999
        setsockopt(SYSLOG, IPPROTO_TCP(), TCP_NODELAY(), 1);
    }
    $@ = "";
    if (!connect(SYSLOG, $addr)) {
	push @$errs, "tcp connect: $!";
	return 0;
    }

    $syslog_send = \&_syslog_send_socket;

    return 1;
}

sub connect_udp {
    my ($errs) = @_;

    my $udp = getprotobyname('udp');
    if (!defined $udp) {
	push @$errs, "getprotobyname failed for udp";
	return 0;
    }

    my $syslog = getservbyname('syslog', 'udp');
    if (!defined $syslog) {
	push @$errs, "getservbyname failed for syslog/udp";
	return 0;
    }

    my $addr;
    if (defined $host) {
        $addr = inet_aton($host);
        if (!$addr) {
	    push @$errs, "can't lookup $host";
	    return 0;
	}
    } else {
        $addr = INADDR_LOOPBACK;
    }
    $addr = sockaddr_in($syslog, $addr);

    if (!socket(SYSLOG, AF_INET, SOCK_DGRAM, $udp)) {
	push @$errs, "udp socket: $!";
	return 0;
    }
    if (!connect(SYSLOG, $addr)) {
	push @$errs, "udp connect: $!";
	return 0;
    }

    # We want to check that the UDP connect worked. However the only
    # way to do that is to send a message and see if an ICMP is returned
    _syslog_send_socket("");
    if (!connection_ok()) {
	push @$errs, "udp connect: nobody listening";
	return 0;
    }

    $syslog_send = \&_syslog_send_socket;

    return 1;
}

sub connect_stream {
    my ($errs) = @_;
    # might want syslog_path to be variable based on syslog.h (if only
    # it were in there!)
    $syslog_path = '/dev/conslog' unless defined $syslog_path; 
    if (!-w $syslog_path) {
	push @$errs, "stream $syslog_path is not writable";
	return 0;
    }
    if (!sysopen(SYSLOG, $syslog_path, 0400, O_WRONLY)) {
	push @$errs, "stream can't open $syslog_path: $!";
	return 0;
    }
    $syslog_send = \&_syslog_send_stream;
    return 1;
}

sub connect_pipe {
    my ($errs) = @_;

    $syslog_path ||= &_PATH_LOG || "/dev/log";

    if (not -w $syslog_path) {
        push @$errs, "$syslog_path is not writable";
        return 0;
    }

    if (not open(SYSLOG, ">$syslog_path")) {
        push @$errs, "can't write to $syslog_path: $!";
        return 0;
    }

    $syslog_send = \&_syslog_send_pipe;

    return 1;
}

sub connect_unix {
    my ($errs) = @_;

    $syslog_path ||= _PATH_LOG() if length _PATH_LOG();

    if (not defined $syslog_path) {
        push @$errs, "_PATH_LOG not available in syslog.h and no user-supplied socket path";
	return 0;
    }

    if (not (-S $syslog_path or -c _)) {
        push @$errs, "$syslog_path is not a socket";
	return 0;
    }

    my $addr = sockaddr_un($syslog_path);
    if (!$addr) {
	push @$errs, "can't locate $syslog_path";
	return 0;
    }
    if (!socket(SYSLOG, AF_UNIX, SOCK_STREAM, 0)) {
        push @$errs, "unix stream socket: $!";
	return 0;
    }

    if (!connect(SYSLOG, $addr)) {
        if (!socket(SYSLOG, AF_UNIX, SOCK_DGRAM, 0)) {
	    push @$errs, "unix dgram socket: $!";
	    return 0;
	}
        if (!connect(SYSLOG, $addr)) {
	    push @$errs, "unix dgram connect: $!";
	    return 0;
	}
    }

    $syslog_send = \&_syslog_send_socket;

    return 1;
}

sub connect_native {
    my ($errs) = @_;
    my $logopt = 0;

    # reconstruct the numeric equivalent of the options
    for my $opt (keys %options) {
        $logopt += xlate($opt) if $options{$opt}
    }

    eval { openlog_xs($ident, $logopt, xlate($facility)) };
    if ($@) {
        push @$errs, $@;
        return 0;
    }

    $syslog_send = \&_syslog_send_native;

    return 1;
}

sub connect_eventlog {
    my ($errs) = @_;

    $syslog_xobj = Sys::Syslog::Win32::_install();
    $syslog_send = \&Sys::Syslog::Win32::_syslog_send;

    return 1;
}

sub connect_console {
    my ($errs) = @_;
    if (!-w '/dev/console') {
	push @$errs, "console is not writable";
	return 0;
    }
    $syslog_send = \&_syslog_send_console;
    return 1;
}

sub connection_ok {
    return 1 if defined $current_proto and (
        $current_proto eq 'native' or $current_proto eq 'console'
        or $current_proto eq 'eventlog'
    );

    my $rin = '';
    vec($rin, fileno(SYSLOG), 1) = 1;
    my $ret = select $rin, undef, $rin, 0.25;
    return ($ret ? 0 : 1);
}

sub disconnect_log {
    $connected = 0;
    $syslog_send = undef;

    if (defined $current_proto and $current_proto eq 'native') {
        closelog_xs();
        return 1;
    }
    elsif (defined $current_proto and $current_proto eq 'eventlog') {
        $syslog_xobj->Close();
        return 1;
    }

    return close SYSLOG;
}

1;

__END__


=begin comment

Notes for the future maintainer (even if it's still me..)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Using Google Code Search, I search who on Earth was relying on $host being 
public. It found 5 hits: 

* First was inside Indigo Star Perl2exe documentation. Just an old version 
of Sys::Syslog. 


* One real hit was inside DalWeathDB, a weather related program. It simply 
does a 

    $Sys::Syslog::host = '127.0.0.1';

- L<http://www.gallistel.net/nparker/weather/code/>


* Two hits were in TPC, a fax server thingy. It does a 

    $Sys::Syslog::host = $TPC::LOGHOST;

but also has this strange piece of code:

    # work around perl5.003 bug
    sub Sys::Syslog::hostname {}

I don't know what bug the author referred to.

- L<http://www.tpc.int/>
- L<ftp://ftp.tpc.int/tpc/server/UNIX/>
- L<ftp://ftp-usa.tpc.int/pub/tpc/server/UNIX/>


* Last hit was in Filefix, which seems to be a FIDOnet mail program (!).
This one does not use $host, but has the following piece of code:

    sub Sys::Syslog::hostname
    {
        use Sys::Hostname;
        return hostname;
    }

I guess this was a more elaborate form of the previous bit, maybe because 
of a bug in Sys::Syslog back then?

- L<ftp://ftp.kiae.su/pub/unix/fido/>


Links
-----
II12021: SYSLOGD HOWTO TCPIPINFO (z/OS, OS/390, MVS)
- L<http://www-1.ibm.com/support/docview.wss?uid=isg1II12021>

Getting the most out of the Event Viewer
- L<http://www.codeproject.com/dotnet/evtvwr.asp?print=true>

Log events to the Windows NT Event Log with JNI
- L<http://www.javaworld.com/javaworld/jw-09-2001/jw-0928-ntmessages.html>

=end comment

