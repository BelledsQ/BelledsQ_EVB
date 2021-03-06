package lib;


use Config;

use strict;

my $archname         = $Config{archname};
my $version          = $Config{version};
my @inc_version_list = reverse split / /, $Config{inc_version_list};


our @ORIG_INC = @INC;	# take a handy copy of 'original' value
our $VERSION = '0.5565';
my $Is_MacOS = $^O eq 'MacOS';
my $Mac_FS;
if ($Is_MacOS) {
	require File::Spec;
	$Mac_FS = eval { require Mac::FileSpec::Unixish };
}

sub import {
    shift;

    my %names;
    foreach (reverse @_) {
	my $path = $_;		# we'll be modifying it, so break the alias
	if ($path eq '') {
	    require Carp;
	    Carp::carp("Empty compile time value given to use lib");
	}

	$path = _nativize($path);

	if (-e $path && ! -d _) {
	    require Carp;
	    Carp::carp("Parameter to use lib must be directory, not file");
	}
	unshift(@INC, $path);
	# Add any previous version directories we found at configure time
	foreach my $incver (@inc_version_list)
	{
	    my $dir = $Is_MacOS
		? File::Spec->catdir( $path, $incver )
		: "$path/$incver";
	    unshift(@INC, $dir) if -d $dir;
	}
	# Put a corresponding archlib directory in front of $path if it
	# looks like $path has an archlib directory below it.
	my($arch_auto_dir, $arch_dir, $version_dir, $version_arch_dir)
	    = _get_dirs($path);
	unshift(@INC, $arch_dir)         if -d $arch_auto_dir;
	unshift(@INC, $version_dir)      if -d $version_dir;
	unshift(@INC, $version_arch_dir) if -d $version_arch_dir;
    }

    # remove trailing duplicates
    @INC = grep { ++$names{$_} == 1 } @INC;
    return;
}


sub unimport {
    shift;

    my %names;
    foreach (@_) {
	my $path = _nativize($_);

	my($arch_auto_dir, $arch_dir, $version_dir, $version_arch_dir)
	    = _get_dirs($path);
	++$names{$path};
	++$names{$arch_dir}         if -d $arch_auto_dir;
	++$names{$version_dir}      if -d $version_dir;
	++$names{$version_arch_dir} if -d $version_arch_dir;
    }

    # Remove ALL instances of each named directory.
    @INC = grep { !exists $names{$_} } @INC;
    return;
}

sub _get_dirs {
    my($dir) = @_;
    my($arch_auto_dir, $arch_dir, $version_dir, $version_arch_dir);

    # we could use this for all platforms in the future, but leave it
    # Mac-only for now, until there is more time for testing it.
    if ($Is_MacOS) {
	$arch_auto_dir    = File::Spec->catdir( $dir, $archname, 'auto' );
	$arch_dir         = File::Spec->catdir( $dir, $archname, );
	$version_dir      = File::Spec->catdir( $dir, $version );
	$version_arch_dir = File::Spec->catdir( $dir, $version, $archname );
    } else {
	$arch_auto_dir    = "$dir/$archname/auto";
	$arch_dir         = "$dir/$archname";
	$version_dir      = "$dir/$version";
	$version_arch_dir = "$dir/$version/$archname";
    }
    return($arch_auto_dir, $arch_dir, $version_dir, $version_arch_dir);
}

sub _nativize {
    my($dir) = @_;

    if ($Is_MacOS && $Mac_FS && ! -d $dir) {
	$dir = Mac::FileSpec::Unixish::nativize($dir);
	$dir .= ":" unless $dir =~ /:$/;
    }

    return $dir;
}

1;
__END__

