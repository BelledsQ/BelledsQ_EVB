
package Errno;
our (@EXPORT_OK,%EXPORT_TAGS,@ISA,$VERSION,%errno,$AUTOLOAD);
use Exporter ();
use Config;
use strict;

"$Config{'archname'}-$Config{'osvers'}" eq
"mips-linux-uclibc-2.4.30" or
	die "Errno architecture (mips-linux-uclibc-2.4.30) does not match executable architecture ($Config{'archname'}-$Config{'osvers'})";

$VERSION = "1.10";
$VERSION = eval $VERSION;
@ISA = qw(Exporter);

@EXPORT_OK = qw(EBADR ENOMSG ENOTSUP ESTRPIPE EADDRINUSE EL3HLT EBADF
	ENOTBLK ENAVAIL ECHRNG ENOTNAM ELNRNG ENOKEY EXDEV EBADE EBADSLT
	ECONNREFUSED ENOSTR ENONET EOVERFLOW EISCONN EFBIG EKEYREVOKED
	ECONNRESET EWOULDBLOCK ELIBMAX EREMOTEIO ERFKILL ENOPKG ELIBSCN
	EDESTADDRREQ ENOTSOCK EIO EMEDIUMTYPE EINPROGRESS ERANGE EAFNOSUPPORT
	EADDRNOTAVAIL EINTR EREMOTE EILSEQ ENOMEM EPIPE ENETUNREACH ENODATA
	EINIT EUSERS EOPNOTSUPP EPROTO EISNAM ESPIPE EALREADY ENAMETOOLONG
	ENOEXEC EISDIR EBADRQC EEXIST EDOTDOT ELIBBAD EOWNERDEAD ESRCH EFAULT
	EXFULL EDEADLOCK EAGAIN ENOPROTOOPT ENETDOWN EPROTOTYPE EL2NSYNC
	ENETRESET EUCLEAN EADV EROFS ESHUTDOWN EMULTIHOP EPROTONOSUPPORT
	ENFILE ENOLCK ECONNABORTED ECANCELED EDEADLK ESRMNT ENOLINK ETIME
	ENOTDIR EINVAL ENOTTY ENOANO ELOOP EPFNOSUPPORT ENOENT EBADMSG
	ENOMEDIUM EL2HLT EDOM EBFONT EKEYEXPIRED EMSGSIZE ENOCSI EL3RST ENOSPC
	EIDRM ENOBUFS ENOSYS EHOSTDOWN EBADFD ENOSR ENOTCONN ESTALE EDQUOT
	EKEYREJECTED EREMDEV EMFILE ENOTRECOVERABLE EACCES EBUSY E2BIG EPERM
	ELIBEXEC ETOOMANYREFS ELIBACC ENOTUNIQ ECOMM ERESTART ESOCKTNOSUPPORT
	EUNATCH ETIMEDOUT ENXIO ENODEV ETXTBSY EHWPOISON EMLINK ECHILD
	EHOSTUNREACH EREMCHG ENOTEMPTY);

%EXPORT_TAGS = (
    POSIX => [qw(
	E2BIG EACCES EADDRINUSE EADDRNOTAVAIL EAFNOSUPPORT EAGAIN EALREADY
	EBADF EBUSY ECHILD ECONNABORTED ECONNREFUSED ECONNRESET EDEADLK
	EDESTADDRREQ EDOM EDQUOT EEXIST EFAULT EFBIG EHOSTDOWN EHOSTUNREACH
	EINPROGRESS EINTR EINVAL EIO EISCONN EISDIR ELOOP EMFILE EMLINK
	EMSGSIZE ENAMETOOLONG ENETDOWN ENETRESET ENETUNREACH ENFILE ENOBUFS
	ENODEV ENOENT ENOEXEC ENOLCK ENOMEM ENOPROTOOPT ENOSPC ENOSYS ENOTBLK
	ENOTCONN ENOTDIR ENOTEMPTY ENOTSOCK ENOTTY ENXIO EOPNOTSUPP EPERM
	EPFNOSUPPORT EPIPE EPROTONOSUPPORT EPROTOTYPE ERANGE EREMOTE ERESTART
	EROFS ESHUTDOWN ESOCKTNOSUPPORT ESPIPE ESRCH ESTALE ETIMEDOUT
	ETOOMANYREFS ETXTBSY EUSERS EWOULDBLOCK EXDEV
    )]
);

sub EPERM () { 1 }
sub ENOENT () { 2 }
sub ESRCH () { 3 }
sub EINTR () { 4 }
sub EIO () { 5 }
sub ENXIO () { 6 }
sub E2BIG () { 7 }
sub ENOEXEC () { 8 }
sub EBADF () { 9 }
sub ECHILD () { 10 }
sub EWOULDBLOCK () { 11 }
sub EAGAIN () { 11 }
sub ENOMEM () { 12 }
sub EACCES () { 13 }
sub EFAULT () { 14 }
sub ENOTBLK () { 15 }
sub EBUSY () { 16 }
sub EEXIST () { 17 }
sub EXDEV () { 18 }
sub ENODEV () { 19 }
sub ENOTDIR () { 20 }
sub EISDIR () { 21 }
sub EINVAL () { 22 }
sub ENFILE () { 23 }
sub EMFILE () { 24 }
sub ENOTTY () { 25 }
sub ETXTBSY () { 26 }
sub EFBIG () { 27 }
sub ENOSPC () { 28 }
sub ESPIPE () { 29 }
sub EROFS () { 30 }
sub EMLINK () { 31 }
sub EPIPE () { 32 }
sub EDOM () { 33 }
sub ERANGE () { 34 }
sub ENOMSG () { 35 }
sub EIDRM () { 36 }
sub ECHRNG () { 37 }
sub EL2NSYNC () { 38 }
sub EL3HLT () { 39 }
sub EL3RST () { 40 }
sub ELNRNG () { 41 }
sub EUNATCH () { 42 }
sub ENOCSI () { 43 }
sub EL2HLT () { 44 }
sub EDEADLK () { 45 }
sub ENOLCK () { 46 }
sub EBADE () { 50 }
sub EBADR () { 51 }
sub EXFULL () { 52 }
sub ENOANO () { 53 }
sub EBADRQC () { 54 }
sub EBADSLT () { 55 }
sub EDEADLOCK () { 56 }
sub EBFONT () { 59 }
sub ENOSTR () { 60 }
sub ENODATA () { 61 }
sub ETIME () { 62 }
sub ENOSR () { 63 }
sub ENONET () { 64 }
sub ENOPKG () { 65 }
sub EREMOTE () { 66 }
sub ENOLINK () { 67 }
sub EADV () { 68 }
sub ESRMNT () { 69 }
sub ECOMM () { 70 }
sub EPROTO () { 71 }
sub EDOTDOT () { 73 }
sub EMULTIHOP () { 74 }
sub EBADMSG () { 77 }
sub ENAMETOOLONG () { 78 }
sub EOVERFLOW () { 79 }
sub ENOTUNIQ () { 80 }
sub EBADFD () { 81 }
sub EREMCHG () { 82 }
sub ELIBACC () { 83 }
sub ELIBBAD () { 84 }
sub ELIBSCN () { 85 }
sub ELIBMAX () { 86 }
sub ELIBEXEC () { 87 }
sub EILSEQ () { 88 }
sub ENOSYS () { 89 }
sub ELOOP () { 90 }
sub ERESTART () { 91 }
sub ESTRPIPE () { 92 }
sub ENOTEMPTY () { 93 }
sub EUSERS () { 94 }
sub ENOTSOCK () { 95 }
sub EDESTADDRREQ () { 96 }
sub EMSGSIZE () { 97 }
sub EPROTOTYPE () { 98 }
sub ENOPROTOOPT () { 99 }
sub EPROTONOSUPPORT () { 120 }
sub ESOCKTNOSUPPORT () { 121 }
sub ENOTSUP () { 122 }
sub EOPNOTSUPP () { 122 }
sub EPFNOSUPPORT () { 123 }
sub EAFNOSUPPORT () { 124 }
sub EADDRINUSE () { 125 }
sub EADDRNOTAVAIL () { 126 }
sub ENETDOWN () { 127 }
sub ENETUNREACH () { 128 }
sub ENETRESET () { 129 }
sub ECONNABORTED () { 130 }
sub ECONNRESET () { 131 }
sub ENOBUFS () { 132 }
sub EISCONN () { 133 }
sub ENOTCONN () { 134 }
sub EUCLEAN () { 135 }
sub ENOTNAM () { 137 }
sub ENAVAIL () { 138 }
sub EISNAM () { 139 }
sub EREMOTEIO () { 140 }
sub EINIT () { 141 }
sub EREMDEV () { 142 }
sub ESHUTDOWN () { 143 }
sub ETOOMANYREFS () { 144 }
sub ETIMEDOUT () { 145 }
sub ECONNREFUSED () { 146 }
sub EHOSTDOWN () { 147 }
sub EHOSTUNREACH () { 148 }
sub EALREADY () { 149 }
sub EINPROGRESS () { 150 }
sub ESTALE () { 151 }
sub ECANCELED () { 158 }
sub ENOMEDIUM () { 159 }
sub EMEDIUMTYPE () { 160 }
sub ENOKEY () { 161 }
sub EKEYEXPIRED () { 162 }
sub EKEYREVOKED () { 163 }
sub EKEYREJECTED () { 164 }
sub EOWNERDEAD () { 165 }
sub ENOTRECOVERABLE () { 166 }
sub ERFKILL () { 167 }
sub EHWPOISON () { 168 }
sub EDQUOT () { 1133 }

sub TIEHASH { bless [] }

sub FETCH {
    my ($self, $errname) = @_;
    my $proto = prototype("Errno::$errname");
    my $errno = "";
    if (defined($proto) && $proto eq "") {
	no strict 'refs';
	$errno = &$errname;
        $errno = 0 unless $! == $errno;
    }
    return $errno;
}

sub STORE {
    require Carp;
    Carp::confess("ERRNO hash is read only!");
}

*CLEAR = \&STORE;
*DELETE = \&STORE;

sub NEXTKEY {
    my($k,$v);
    while(($k,$v) = each %Errno::) {
	my $proto = prototype("Errno::$k");
	last if (defined($proto) && $proto eq "");
    }
    $k
}

sub FIRSTKEY {
    my $s = scalar keys %Errno::;	# initialize iterator
    goto &NEXTKEY;
}

sub EXISTS {
    my ($self, $errname) = @_;
    my $r = ref $errname;
    my $proto = !$r || $r eq 'CODE' ? prototype($errname) : undef;
    defined($proto) && $proto eq "";
}

tie %!, __PACKAGE__;

1;
__END__


