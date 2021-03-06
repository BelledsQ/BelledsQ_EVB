#!/usr/local/bin/perl

package Class::ISA;
require 5;
use strict;
use vars qw($Debug $VERSION);
$VERSION = '0.33';
$Debug = 0 unless defined $Debug;



sub self_and_super_versions {
  no strict 'refs';
  map {
        $_ => (defined(${"$_\::VERSION"}) ? ${"$_\::VERSION"} : undef)
      } self_and_super_path($_[0])
}


sub super_path {
  my @ret = &self_and_super_path(@_);
  shift @ret if @ret;
  return @ret;
}

sub self_and_super_path {
  # Assumption: searching is depth-first.
  # Assumption: '' (empty string) can't be a class package name.
  # Note: 'UNIVERSAL' is not given any special treatment.
  return () unless @_;

  my @out = ();

  my @in_stack = ($_[0]);
  my %seen = ($_[0] => 1);

  my $current;
  while(@in_stack) {
    next unless defined($current = shift @in_stack) && length($current);
    print "At $current\n" if $Debug;
    push @out, $current;
    no strict 'refs';
    unshift @in_stack,
      map
        { my $c = $_; # copy, to avoid being destructive
          substr($c,0,2) = "main::" if substr($c,0,2) eq '::';
           # Canonize the :: -> main::, ::foo -> main::foo thing.
           # Should I ever canonize the Foo'Bar = Foo::Bar thing? 
          $seen{$c}++ ? () : $c;
        }
        @{"$current\::ISA"}
    ;
    # I.e., if this class has any parents (at least, ones I've never seen
    # before), push them, in order, onto the stack of classes I need to
    # explore.
  }

  return @out;
}
1;

__END__
