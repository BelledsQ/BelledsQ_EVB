

package DynaLoader;



BEGIN {
    $VERSION = '1.08';
}

require AutoLoader;
*AUTOLOAD = \&AutoLoader::AUTOLOAD;

use Config;

$dl_debug = $ENV{PERL_DL_DEBUG} || 0 unless defined $dl_debug;


sub dl_load_flags { 0x00 }

  ($dl_dlext, $dlsrc) = ('so', 'dl_dlopen.xs')
;

$do_expand = 0;



@dl_require_symbols = ();       # names of symbols we need
@dl_resolve_using   = ();       # names of files to link with
@dl_library_path    = ();       # path to look for files


@dl_resolve_using = dl_findfile('-lc') if $dlsrc eq "dl_dld.xs";


push(@dl_library_path, split(' ', $Config::Config{libpth}));


my $ldlibpthname         = $Config::Config{ldlibpthname};
my $ldlibpthname_defined = defined $Config::Config{ldlibpthname};
my $pthsep               = $Config::Config{path_sep};


if ($ldlibpthname_defined &&
    exists $ENV{$ldlibpthname}) {
    push(@dl_library_path, split(/$pthsep/, $ENV{$ldlibpthname}));
}


if ($ldlibpthname_defined &&
    $ldlibpthname ne 'LD_LIBRARY_PATH' &&
    exists $ENV{LD_LIBRARY_PATH}) {
    push(@dl_library_path, split(/$pthsep/, $ENV{LD_LIBRARY_PATH}));
}


boot_DynaLoader('DynaLoader') if defined(&boot_DynaLoader) &&
                                !defined(&dl_error);

if ($dl_debug) {
    print STDERR "DynaLoader.pm loaded (@INC, @dl_library_path)\n";
    print STDERR "DynaLoader not linked into this perl\n"
	    unless defined(&boot_DynaLoader);
}

1; # End of main code


sub croak   { require Carp; Carp::croak(@_)   }

sub bootstrap_inherit {
    my $module = $_[0];
    local *isa = *{"$module\::ISA"};
    local @isa = (@isa, 'DynaLoader');
    # Cannot goto due to delocalization.  Will report errors on a wrong line?
    bootstrap(@_);
}


sub bootstrap {
    # use local vars to enable $module.bs script to edit values
    local(@args) = @_;
    local($module) = $args[0];
    local(@dirs, $file);

    unless ($module) {
	require Carp;
	Carp::confess("Usage: DynaLoader::bootstrap(module)");
    }

    # A common error on platforms which don't support dynamic loading.
    # Since it's fatal and potentially confusing we give a detailed message.
    croak("Can't load module $module, dynamic loading not available in this perl.\n".
	"  (You may need to build a new perl executable which either supports\n".
	"  dynamic loading or has the $module module statically linked into it.)\n")
	unless defined(&dl_load_file);


    
    my @modparts = split(/::/,$module);
    my $modfname = $modparts[-1];

    # Some systems have restrictions on files names for DLL's etc.
    # mod2fname returns appropriate file base name (typically truncated)
    # It may also edit @modparts if required.
    $modfname = &mod2fname(\@modparts) if defined &mod2fname;

    

    my $modpname = join('/',@modparts);

    print STDERR "DynaLoader::bootstrap for $module ",
		       "(auto/$modpname/$modfname.so)\n"
	if $dl_debug;

    foreach (@INC) {
	
	
	    my $dir = "$_/auto/$modpname";
	
	
	next unless -d $dir; # skip over uninteresting directories
	
	# check for common cases to avoid autoload of dl_findfile
	my $try =  "$dir/$modfname.so";
	last if $file = (-f $try) && $try;
	
	
	# no luck here, save dir for possible later dl_findfile search
	push @dirs, $dir;
    }
    # last resort, let dl_findfile have a go in all known locations
    $file = dl_findfile(map("-L$_",@dirs,@INC), $modfname) unless $file;

    croak("Can't locate loadable object for module $module in \@INC (\@INC contains: @INC)")
	unless $file;	# wording similar to error from 'require'

    
    my $bootname = "boot_$module";
    $bootname =~ s/\W/_/g;
    @dl_require_symbols = ($bootname);

    # Execute optional '.bootstrap' perl script for this module.
    # The .bs file can be used to configure @dl_resolve_using etc to
    # match the needs of the individual module on this architecture.
    my $bs = $file;
    $bs =~ s/(\.\w+)?(;\d*)?$/\.bs/; # look for .bs 'beside' the library
    if (-s $bs) { # only read file if it's not empty
        print STDERR "BS: $bs ($^O, $dlsrc)\n" if $dl_debug;
        eval { do $bs; };
        warn "$bs: $@\n" if $@;
    }

    my $boot_symbol_ref;

    

    # Many dynamic extension loading problems will appear to come from
    # this section of code: XYZ failed at line 123 of DynaLoader.pm.
    # Often these errors are actually occurring in the initialisation
    # C code of the extension XS file. Perl reports the error as being
    # in this perl code simply because this was the last perl code
    # it executed.

    my $libref = dl_load_file($file, $module->dl_load_flags) or
	croak("Can't load '$file' for module $module: ".dl_error());

    push(@dl_librefs,$libref);  # record loaded object

    my @unresolved = dl_undef_symbols();
    if (@unresolved) {
	require Carp;
	Carp::carp("Undefined symbols present after loading $file: @unresolved\n");
    }

    $boot_symbol_ref = dl_find_symbol($libref, $bootname) or
         croak("Can't find '$bootname' symbol in $file\n");

    push(@dl_modules, $module); # record loaded module

  boot:
    my $xs = dl_install_xsub("${module}::bootstrap", $boot_symbol_ref, $file);

    # See comment block above

	push(@dl_shared_objects, $file); # record files loaded

    &$xs(@args);
}




__END__


sub dl_findfile {
    # Read ext/DynaLoader/DynaLoader.doc for detailed information.
    # This function does not automatically consider the architecture
    # or the perl library auto directories.
    my (@args) = @_;
    my (@dirs,  $dir);   # which directories to search
    my (@found);         # full paths to real files we have found
    #my $dl_ext= 'so'; # $Config::Config{'dlext'} suffix for perl extensions
    #my $dl_so = 'so'; # $Config::Config{'so'} suffix for shared libraries

    print STDERR "dl_findfile(@args)\n" if $dl_debug;

    # accumulate directories but process files as they appear
    arg: foreach(@args) {
        #  Special fast case: full filepath requires no search
	
	
	
        if (m:/: && -f $_) {
	    push(@found,$_);
	    last arg unless wantarray;
	    next;
	}
	

        # Deal with directories first:
        #  Using a -L prefix is the preferred option (faster and more robust)
        if (m:^-L:) { s/^-L//; push(@dirs, $_); next; }

	
	
        #  Otherwise we try to try to spot directories by a heuristic
        #  (this is a more complicated issue than it first appears)
        if (m:/: && -d $_) {   push(@dirs, $_); next; }

	

        #  Only files should get this far...
        my(@names, $name);    # what filenames to look for
        if (m:-l: ) {          # convert -lname to appropriate library name
            s/-l//;
            push(@names,"lib$_.so");
            push(@names,"lib$_.a");
        } else {                # Umm, a bare name. Try various alternatives:
            # these should be ordered with the most likely first
            push(@names,"$_.so")    unless m/\.so$/o;
            push(@names,"$_.so")     unless m/\.so$/o;
            push(@names,"lib$_.so")  unless m:/:;
            push(@names,"$_.a")          if !m/\.a$/ and $dlsrc eq "dl_dld.xs";
            push(@names, $_);
        }
	my $dirsep = '/';
	
        foreach $dir (@dirs, @dl_library_path) {
            next unless -d $dir;
	    
            foreach $name (@names) {
		my($file) = "$dir$dirsep$name";
                print STDERR " checking in $dir for $name\n" if $dl_debug;
		$file = ($do_expand) ? dl_expandspec($file) : (-f $file && $file);
		#$file = _check_file($file);
		if ($file) {
                    push(@found, $file);
                    next arg; # no need to look any further
                }
            }
        }
    }
    if ($dl_debug) {
        foreach(@dirs) {
            print STDERR " dl_findfile ignored non-existent directory: $_\n" unless -d $_;
        }
        print STDERR "dl_findfile found: @found\n";
    }
    return $found[0] unless wantarray;
    @found;
}


sub dl_expandspec {
    my($spec) = @_;
    # Optional function invoked if DynaLoader.pm sets $do_expand.
    # Most systems do not require or use this function.
    # Some systems may implement it in the dl_*.xs file in which case
    # this autoload version will not be called but is harmless.

    # This function is designed to deal with systems which treat some
    # 'filenames' in a special way. For example VMS 'Logical Names'
    # (something like unix environment variables - but different).
    # This function should recognise such names and expand them into
    # full file paths.
    # Must return undef if $spec is invalid or file does not exist.

    my $file = $spec; # default output to input

    
	return undef unless -f $file;
    
    print STDERR "dl_expandspec($spec) => $file\n" if $dl_debug;
    $file;
}

sub dl_find_symbol_anywhere
{
    my $sym = shift;
    my $libref;
    foreach $libref (@dl_librefs) {
	my $symref = dl_find_symbol($libref,$sym);
	return $symref if $symref;
    }
    return undef;
}

