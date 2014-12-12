
package IPC::Semaphore;

use IPC::SysV qw(GETNCNT GETZCNT GETVAL SETVAL GETPID GETALL SETALL
		 IPC_STAT IPC_SET IPC_RMID);
use strict;
use vars qw($VERSION);
use Carp;

$VERSION = "1.02";
$VERSION = eval $VERSION;

{
    package IPC::Semaphore::stat;

    use Class::Struct qw(struct);

    struct 'IPC::Semaphore::stat' => [
	uid	=> '$',
	gid	=> '$',
	cuid	=> '$',
	cgid	=> '$',
	mode	=> '$',
	ctime	=> '$',
	otime	=> '$',
	nsems	=> '$',
    ];
}

sub new {
    @_ == 4 || croak 'new ' . __PACKAGE__ . '( KEY, NSEMS, FLAGS )';
    my $class = shift;

    my $id = semget($_[0],$_[1],$_[2]);

    defined($id)
	? bless \$id, $class
	: undef;
}

sub id {
    my $self = shift;
    $$self;
}

sub remove {
    my $self = shift;
    (semctl($$self,0,IPC_RMID,0), undef $$self)[0];
}

sub getncnt {
    @_ == 2 || croak '$sem->getncnt( SEM )';
    my $self = shift;
    my $sem = shift;
    my $v = semctl($$self,$sem,GETNCNT,0);
    $v ? 0 + $v : undef;
}

sub getzcnt {
    @_ == 2 || croak '$sem->getzcnt( SEM )';
    my $self = shift;
    my $sem = shift;
    my $v = semctl($$self,$sem,GETZCNT,0);
    $v ? 0 + $v : undef;
}

sub getval {
    @_ == 2 || croak '$sem->getval( SEM )';
    my $self = shift;
    my $sem = shift;
    my $v = semctl($$self,$sem,GETVAL,0);
    $v ? 0 + $v : undef;
}

sub getpid {
    @_ == 2 || croak '$sem->getpid( SEM )';
    my $self = shift;
    my $sem = shift;
    my $v = semctl($$self,$sem,GETPID,0);
    $v ? 0 + $v : undef;
}

sub op {
    @_ >= 4 || croak '$sem->op( OPLIST )';
    my $self = shift;
    croak 'Bad arg count' if @_ % 3;
    my $data = pack("s!*",@_);
    semop($$self,$data);
}

sub stat {
    my $self = shift;
    my $data = "";
    semctl($$self,0,IPC_STAT,$data)
	or return undef;
    IPC::Semaphore::stat->new->unpack($data);
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

    my $v = semctl($$self,0,IPC_SET,$ds->pack);
    $v ? 0 + $v : undef;
}

sub getall {
    my $self = shift;
    my $data = "";
    semctl($$self,0,GETALL,$data)
	or return ();
    (unpack("s!*",$data));
}

sub setall {
    my $self = shift;
    my $data = pack("s!*",@_);
    semctl($$self,0,SETALL,$data);
}

sub setval {
    @_ == 3 || croak '$sem->setval( SEM, VAL )';
    my $self = shift;
    my $sem = shift;
    my $val = shift;
    semctl($$self,$sem,SETVAL,$val);
}

1;

__END__

