package Carp;

our $VERSION = '1.08';

our $MaxEvalLen = 0;
our $Verbose    = 0;
our $CarpLevel  = 0;
our $MaxArgLen  = 64;   # How much of each argument to print. 0 = all.
our $MaxArgNums = 8;    # How many arguments to print. 0 = all.

require Exporter;
our @ISA = ('Exporter');
our @EXPORT = qw(confess croak carp);
our @EXPORT_OK = qw(cluck verbose longmess shortmess);
our @EXPORT_FAIL = qw(verbose);	# hook to enable verbose mode


sub export_fail { shift; $Verbose = shift if $_[0] eq 'verbose'; @_ }

sub longmess  { goto &longmess_jmp }
sub shortmess { goto &shortmess_jmp }
sub longmess_jmp  {
    local($@, $!);
    eval { require Carp::Heavy };
    return $@ if $@;
    goto &longmess_real;
}
sub shortmess_jmp  {
    local($@, $!);
    eval { require Carp::Heavy };
    return $@ if $@;
    goto &shortmess_real;
}

sub croak   { die  shortmess @_ }
sub confess { die  longmess  @_ }
sub carp    { warn shortmess @_ }
sub cluck   { warn longmess  @_ }

1;
__END__

