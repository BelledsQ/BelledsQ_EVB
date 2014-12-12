package Carp;


use Carp;  our $VERSION = $Carp::VERSION;



$CarpInternal{Carp}++;
$CarpInternal{warnings}++;
$Internal{Exporter}++;
$Internal{'Exporter::Heavy'}++;


our ($CarpLevel, $MaxArgNums, $MaxEvalLen, $MaxArgLen, $Verbose);


sub  longmess_real {
    # Icky backwards compatibility wrapper. :-(
    #
    # The story is that the original implementation hard-coded the
    # number of call levels to go back, so calls to longmess were off
    # by one.  Other code began calling longmess and expecting this
    # behaviour, so the replacement has to emulate that behaviour.
    my $call_pack = caller();
    if ($Internal{$call_pack} or $CarpInternal{$call_pack}) {
      return longmess_heavy(@_);
    }
    else {
      local $CarpLevel = $CarpLevel + 1;
      return longmess_heavy(@_);
    }
};

sub shortmess_real {
    # Icky backwards compatibility wrapper. :-(
    local @CARP_NOT = caller();
    shortmess_heavy(@_);
};



delete $Carp::{shortmess_jmp};
delete $Carp::{longmess_jmp};
*longmess_jmp  = *longmess_real;
*shortmess_jmp = *shortmess_real;


sub caller_info {
  my $i = shift(@_) + 1;
  package DB;
  my %call_info;
  @call_info{
    qw(pack file line sub has_args wantarray evaltext is_require)
  } = caller($i);
  
  unless (defined $call_info{pack}) {
    return ();
  }

  my $sub_name = Carp::get_subname(\%call_info);
  if ($call_info{has_args}) {
    my @args = map {Carp::format_arg($_)} @DB::args;
    if ($MaxArgNums and @args > $MaxArgNums) { # More than we want to show?
      $#args = $MaxArgNums;
      push @args, '...';
    }
    # Push the args onto the subroutine
    $sub_name .= '(' . join (', ', @args) . ')';
  }
  $call_info{sub_name} = $sub_name;
  return wantarray() ? %call_info : \%call_info;
}

sub format_arg {
  my $arg = shift;
  if (ref($arg)) {
      $arg = defined($overload::VERSION) ? overload::StrVal($arg) : "$arg";
  }
  if (defined($arg)) {
      $arg =~ s/'/\\'/g;
      $arg = str_len_trim($arg, $MaxArgLen);
  
      # Quote it?
      $arg = "'$arg'" unless $arg =~ /^-?[\d.]+\z/;
  } else {
      $arg = 'undef';
  }

  # The following handling of "control chars" is direct from
  # the original code - it is broken on Unicode though.
  # Suggestions?
  utf8::is_utf8($arg)
    or $arg =~ s/([[:cntrl:]]|[[:^ascii:]])/sprintf("\\x{%x}",ord($1))/eg;
  return $arg;
}

sub get_status {
    my $cache = shift;
    my $pkg = shift;
    $cache->{$pkg} ||= [{$pkg => $pkg}, [trusts_directly($pkg)]];
    return @{$cache->{$pkg}};
}

sub get_subname {
  my $info = shift;
  if (defined($info->{evaltext})) {
    my $eval = $info->{evaltext};
    if ($info->{is_require}) {
      return "require $eval";
    }
    else {
      $eval =~ s/([\\\'])/\\$1/g;
      return "eval '" . str_len_trim($eval, $MaxEvalLen) . "'";
    }
  }

  return ($info->{sub} eq '(eval)') ? 'eval {...}' : $info->{sub};
}

sub long_error_loc {
  my $i;
  my $lvl = $CarpLevel;
  {
    my $pkg = caller(++$i);
    unless(defined($pkg)) {
      # This *shouldn't* happen.
      if (%Internal) {
        local %Internal;
        $i = long_error_loc();
        last;
      }
      else {
        # OK, now I am irritated.
        return 2;
      }
    }
    redo if $CarpInternal{$pkg};
    redo unless 0 > --$lvl;
    redo if $Internal{$pkg};
  }
  return $i - 1;
}


sub longmess_heavy {
  return @_ if ref($_[0]); # don't break references as exceptions
  my $i = long_error_loc();
  return ret_backtrace($i, @_);
}

sub ret_backtrace {
  my ($i, @error) = @_;
  my $mess;
  my $err = join '', @error;
  $i++;

  my $tid_msg = '';
  if (defined &threads::tid) {
    my $tid = threads->tid;
    $tid_msg = " thread $tid" if $tid;
  }

  my %i = caller_info($i);
  $mess = "$err at $i{file} line $i{line}$tid_msg\n";

  while (my %i = caller_info(++$i)) {
      $mess .= "\t$i{sub_name} called at $i{file} line $i{line}$tid_msg\n";
  }
  
  return $mess;
}

sub ret_summary {
  my ($i, @error) = @_;
  my $err = join '', @error;
  $i++;

  my $tid_msg = '';
  if (defined &threads::tid) {
    my $tid = threads->tid;
    $tid_msg = " thread $tid" if $tid;
  }

  my %i = caller_info($i);
  return "$err at $i{file} line $i{line}$tid_msg\n";
}


sub short_error_loc {
  # You have to create your (hash)ref out here, rather than defaulting it
  # inside trusts *on a lexical*, as you want it to persist across calls.
  # (You can default it on $_[2], but that gets messy)
  my $cache = {};
  my $i = 1;
  my $lvl = $CarpLevel;
  {
    my $called = caller($i++);
    my $caller = caller($i);

    return 0 unless defined($caller); # What happened?
    redo if $Internal{$caller};
    redo if $CarpInternal{$caller};
    redo if $CarpInternal{$called};
    redo if trusts($called, $caller, $cache);
    redo if trusts($caller, $called, $cache);
    redo unless 0 > --$lvl;
  }
  return $i - 1;
}


sub shortmess_heavy {
  return longmess_heavy(@_) if $Verbose;
  return @_ if ref($_[0]); # don't break references as exceptions
  my $i = short_error_loc();
  if ($i) {
    ret_summary($i, @_);
  }
  else {
    longmess_heavy(@_);
  }
}

sub str_len_trim {
  my $str = shift;
  my $max = shift || 0;
  if (2 < $max and $max < length($str)) {
    substr($str, $max - 3) = '...';
  }
  return $str;
}

sub trusts {
    my $child = shift;
    my $parent = shift;
    my $cache = shift;
    my ($known, $partial) = get_status($cache, $child);
    # Figure out consequences until we have an answer
    while (@$partial and not exists $known->{$parent}) {
        my $anc = shift @$partial;
        next if exists $known->{$anc};
        $known->{$anc}++;
        my ($anc_knows, $anc_partial) = get_status($cache, $anc);
        my @found = keys %$anc_knows;
        @$known{@found} = ();
        push @$partial, @$anc_partial;
    }
    return exists $known->{$parent};
}

sub trusts_directly {
    my $class = shift;
    no strict 'refs';
    no warnings 'once'; 
    return @{"$class\::CARP_NOT"}
      ? @{"$class\::CARP_NOT"}
      : @{"$class\::ISA"};
}

1;

