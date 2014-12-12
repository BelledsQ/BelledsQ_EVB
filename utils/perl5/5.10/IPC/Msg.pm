
package IPC::Msg;

use IPC::SysV qw(IPC_STAT IPC_SET IPC_RMID);
use strict;
use vars qw($VERSION);
use Carp;

$VERSION = "1.02";
$VERSION = eval $VERSION;

{
    package IPC::Msg::stat;

    use Class::Struct qw(struct);

    struct 'IPC::Msg::stat' => [
	uid	=> '$',
	gid	=> '$',
	cuid	=> '$',
	cgid	=> '$',
	mode	=> '$',
	qnum	=> '$',
	qbytes	=> '$',
	lspid	=> '$',
	lrpid	=> '$',
	stime	=> '$',
	rtime	=> '$',
	ctime	=> '$',
    ];
}

sub new {
    @_ == 3 || croak 'new IPC::Msg ( KEY , FLAGS )';
    my $class = shift;

    my $id = msgget($_[0],$_[1]);

    defined($id)
	? bless \$id, $class
	: undef;
}

sub id {
    my $self = shift;
    $$self;
}

sub stat {
    my $self = shift;
    my $data = "";
    msgctl($$self,IPC_STAT,$data) or
	return undef;
    IPC::Msg::stat->new->unpack($data);
}

sub set {
    my $self = shift;
    my $ds;

    if(@_ == 1) {
	$ds = shift;
    }
    else {
	croak 'Bad arg count' if @_ % 2;
	my %arg = @_;
	$ds = $self->stat
		or return undef;
	my($key,$val);
	$ds->$key($val)
	    while(($key,$val) = each %arg);
    }

    msgctl($$self,IPC_SET,$ds->pack);
}

sub remove {
    my $self = shift;
    (msgctl($$self,IPC_RMID,0), undef $$self)[0];
}

sub rcv {
    @_ <= 5 && @_ >= 3 or croak '$msg->rcv( BUF, LEN, TYPE, FLAGS )';
    my $self = shift;
    my $buf = "";
    msgrcv($$self,$buf,$_[1],$_[2] || 0, $_[3] || 0) or
	return;
    my $type;
    ($type,$_[0]) = unpack("l! a*",$buf);
    $type;
}

sub snd {
    @_ <= 4 && @_ >= 3 or  croak '$msg->snd( TYPE, BUF, FLAGS )';
    my $self = shift;
    msgsnd($$self,pack("l! a*",$_[0],$_[1]), $_[2] || 0);
}


1;

__END__


