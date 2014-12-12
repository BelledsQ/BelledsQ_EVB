package File::Spec::Win32;

use strict;

use vars qw(@ISA $VERSION);
require File::Spec::Unix;

$VERSION = '3.2501';

@ISA = qw(File::Spec::Unix);

my $DRIVE_RX = '[a-zA-Z]:';
my $UNC_RX = '(?:\\\\\\\\|//)[^\\\\/]+[\\\\/][^\\\\/]+';
my $VOL_RX = "(?:$DRIVE_RX|$UNC_RX)";



sub devnull {
    return "nul";
}

sub rootdir () { '\\' }



my $tmpdir;
sub tmpdir {
    return $tmpdir if defined $tmpdir;
    $tmpdir = $_[0]->_tmpdir( map( $ENV{$_}, qw(TMPDIR TEMP TMP) ),
			      'SYS:/temp',
			      'C:\system\temp',
			      'C:/temp',
			      '/tmp',
			      '/'  );
}


sub case_tolerant () {
  eval { require Win32API::File; } or return 1;
  my $drive = shift || "C:";
  my $osFsType = "\0"x256;
  my $osVolName = "\0"x256;
  my $ouFsFlags = 0;
  Win32API::File::GetVolumeInformation($drive, $osVolName, 256, [], [], $ouFsFlags, $osFsType, 256 );
  if ($ouFsFlags & Win32API::File::FS_CASE_SENSITIVE()) { return 0; }
  else { return 1; }
}


sub file_name_is_absolute {

    my ($self,$file) = @_;

    if ($file =~ m{^($VOL_RX)}o) {
      my $vol = $1;
      return ($vol =~ m{^$UNC_RX}o ? 2
	      : $file =~ m{^$DRIVE_RX[\\/]}o ? 2
	      : 0);
    }
    return $file =~  m{^[\\/]} ? 1 : 0;
}


sub catfile {
    my $self = shift;
    my $file = $self->canonpath(pop @_);
    return $file unless @_;
    my $dir = $self->catdir(@_);
    $dir .= "\\" unless substr($dir,-1) eq "\\";
    return $dir.$file;
}

sub catdir {
    my $self = shift;
    my @args = @_;
    foreach (@args) {
	tr[/][\\];
        # append a backslash to each argument unless it has one there
        $_ .= "\\" unless m{\\$};
    }
    return $self->canonpath(join('', @args));
}

sub path {
    my @path = split(';', $ENV{PATH});
    s/"//g for @path;
    @path = grep length, @path;
    unshift(@path, ".");
    return @path;
}


sub canonpath {
    my ($self,$path) = @_;
    
    $path =~ s/^([a-z]:)/\u$1/s;
    $path =~ s|/|\\|g;
    $path =~ s|([^\\])\\+|$1\\|g;                  # xx\\\\xx  -> xx\xx
    $path =~ s|(\\\.)+\\|\\|g;                     # xx\.\.\xx -> xx\xx
    $path =~ s|^(\.\\)+||s unless $path eq ".\\";  # .\xx      -> xx
    $path =~ s|\\\Z(?!\n)||
	unless $path =~ m{^([A-Z]:)?\\\Z(?!\n)}s;  # xx\       -> xx
    # xx1/xx2/xx3/../../xx -> xx1/xx
    $path =~ s|\\\.\.\.\\|\\\.\.\\\.\.\\|g; # \...\ is 2 levels up
    $path =~ s|^\.\.\.\\|\.\.\\\.\.\\|g;    # ...\ is 2 levels up
    return $path if $path =~ m|^\.\.|;      # skip relative paths
    return $path unless $path =~ /\.\./;    # too few .'s to cleanup
    return $path if $path =~ /\.\.\.\./;    # too many .'s to cleanup
    $path =~ s{^\\\.\.$}{\\};                      # \..    -> \
    1 while $path =~ s{^\\\.\.}{};                 # \..\xx -> \xx

    return $self->_collapse($path);
}


sub splitpath {
    my ($self,$path, $nofile) = @_;
    my ($volume,$directory,$file) = ('','','');
    if ( $nofile ) {
        $path =~ 
            m{^ ( $VOL_RX ? ) (.*) }sox;
        $volume    = $1;
        $directory = $2;
    }
    else {
        $path =~ 
            m{^ ( $VOL_RX ? )
                ( (?:.*[\\/](?:\.\.?\Z(?!\n))?)? )
                (.*)
             }sox;
        $volume    = $1;
        $directory = $2;
        $file      = $3;
    }

    return ($volume,$directory,$file);
}



sub splitdir {
    my ($self,$directories) = @_ ;
    #
    # split() likes to forget about trailing null fields, so here we
    # check to be sure that there will not be any before handling the
    # simple case.
    #
    if ( $directories !~ m|[\\/]\Z(?!\n)| ) {
        return split( m|[\\/]|, $directories );
    }
    else {
        #
        # since there was a trailing separator, add a file name to the end, 
        # then do the split, then replace it with ''.
        #
        my( @directories )= split( m|[\\/]|, "${directories}dummy" ) ;
        $directories[ $#directories ]= '' ;
        return @directories ;
    }
}



sub catpath {
    my ($self,$volume,$directory,$file) = @_;

    # If it's UNC, make sure the glue separator is there, reusing
    # whatever separator is first in the $volume
    my $v;
    $volume .= $v
        if ( (($v) = $volume =~ m@^([\\/])[\\/][^\\/]+[\\/][^\\/]+\Z(?!\n)@s) &&
             $directory =~ m@^[^\\/]@s
           ) ;

    $volume .= $directory ;

    # If the volume is not just A:, make sure the glue separator is 
    # there, reusing whatever separator is first in the $volume if possible.
    if ( $volume !~ m@^[a-zA-Z]:\Z(?!\n)@s &&
         $volume =~ m@[^\\/]\Z(?!\n)@      &&
         $file   =~ m@[^\\/]@
       ) {
        $volume =~ m@([\\/])@ ;
        my $sep = $1 ? $1 : '\\' ;
        $volume .= $sep ;
    }

    $volume .= $file ;

    return $volume ;
}

sub _same {
  lc($_[1]) eq lc($_[2]);
}

sub rel2abs {
    my ($self,$path,$base ) = @_;

    my $is_abs = $self->file_name_is_absolute($path);

    # Check for volume (should probably document the '2' thing...)
    return $self->canonpath( $path ) if $is_abs == 2;

    if ($is_abs) {
      # It's missing a volume, add one
      my $vol = ($self->splitpath( $self->_cwd() ))[0];
      return $self->canonpath( $vol . $path );
    }

    if ( !defined( $base ) || $base eq '' ) {
      require Cwd ;
      $base = Cwd::getdcwd( ($self->splitpath( $path ))[0] ) if defined &Cwd::getdcwd ;
      $base = $self->_cwd() unless defined $base ;
    }
    elsif ( ! $self->file_name_is_absolute( $base ) ) {
      $base = $self->rel2abs( $base ) ;
    }
    else {
      $base = $self->canonpath( $base ) ;
    }

    my ( $path_directories, $path_file ) =
      ($self->splitpath( $path, 1 ))[1,2] ;

    my ( $base_volume, $base_directories ) =
      $self->splitpath( $base, 1 ) ;

    $path = $self->catpath( 
			   $base_volume, 
			   $self->catdir( $base_directories, $path_directories ), 
			   $path_file
			  ) ;

    return $self->canonpath( $path ) ;
}


1;
