package File::Spec::Cygwin;

use strict;
use vars qw(@ISA $VERSION);
require File::Spec::Unix;

$VERSION = '3.2501';

@ISA = qw(File::Spec::Unix);



sub canonpath {
    my($self,$path) = @_;
    $path =~ s|\\|/|g;

    # Handle network path names beginning with double slash
    my $node = '';
    if ( $path =~ s@^(//[^/]+)(?:/|\z)@/@s ) {
        $node = $1;
    }
    return $node . $self->SUPER::canonpath($path);
}

sub catdir {
    my $self = shift;

    # Don't create something that looks like a //network/path
    if ($_[0] and ($_[0] eq '/' or $_[0] eq '\\')) {
        shift;
        return $self->SUPER::catdir('', @_);
    }

    $self->SUPER::catdir(@_);
}



sub file_name_is_absolute {
    my ($self,$file) = @_;
    return 1 if $file =~ m{^([a-z]:)?[\\/]}is; # C:/test
    return $self->SUPER::file_name_is_absolute($file);
}


my $tmpdir;
sub tmpdir {
    return $tmpdir if defined $tmpdir;
    $tmpdir = $_[0]->_tmpdir( $ENV{TMPDIR}, "/tmp", $ENV{'TMP'}, $ENV{'TEMP'}, 'C:/temp' );
}


sub case_tolerant () {
  if ($^O ne 'cygwin') {
    return 1;
  }
  my $drive = shift;
  if (! $drive) {
      my @flags = split(/,/, Cygwin::mount_flags('/cygwin'));
      my $prefix = pop(@flags);
      if (! $prefix || $prefix eq 'cygdrive') {
          $drive = '/cygdrive/c';
      } elsif ($prefix eq '/') {
          $drive = '/c';
      } else {
          $drive = "$prefix/c";
      }
  }
  my $mntopts = Cygwin::mount_flags($drive);
  if ($mntopts and ($mntopts =~ /,managed/)) {
    return 0;
  }
  eval { require Win32API::File; } or return 1;
  my $osFsType = "\0"x256;
  my $osVolName = "\0"x256;
  my $ouFsFlags = 0;
  Win32API::File::GetVolumeInformation($drive, $osVolName, 256, [], [], $ouFsFlags, $osFsType, 256 );
  if ($ouFsFlags & Win32API::File::FS_CASE_SENSITIVE()) { return 0; }
  else { return 1; }
}


1;
