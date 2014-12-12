package IPC::Cmd;

use strict;

BEGIN {

    use constant IS_VMS   => $^O eq 'VMS'                       ? 1 : 0;    
    use constant IS_WIN32 => $^O eq 'MSWin32'                   ? 1 : 0;
    use constant IS_WIN98 => (IS_WIN32 and !Win32::IsWinNT())   ? 1 : 0;

    use Exporter    ();
    use vars        qw[ @ISA $VERSION @EXPORT_OK $VERBOSE $DEBUG
                        $USE_IPC_RUN $USE_IPC_OPEN3 $WARN
                    ];

    $VERSION        = '0.40_1';
    $VERBOSE        = 0;
    $DEBUG          = 0;
    $WARN           = 1;
    $USE_IPC_RUN    = IS_WIN32 && !IS_WIN98;
    $USE_IPC_OPEN3  = not IS_VMS;

    @ISA            = qw[Exporter];
    @EXPORT_OK      = qw[can_run run];
}

require Carp;
use File::Spec;
use Params::Check               qw[check];
use Module::Load::Conditional   qw[can_load];
use Locale::Maketext::Simple    Style => 'gettext';



sub can_use_ipc_run     { 
    my $self    = shift;
    my $verbose = shift || 0;
    
    ### ipc::run doesn't run on win98    
    return if IS_WIN98;

    ### if we dont have ipc::run, we obviously can't use it.
    return unless can_load(
                        modules => { 'IPC::Run' => '0.55' },        
                        verbose => ($WARN && $verbose),
                    );
                    
    ### otherwise, we're good to go
    return 1;                    
}



sub can_use_ipc_open3   { 
    my $self    = shift;
    my $verbose = shift || 0;

    ### ipc::open3 is not working on VMS becasue of a lack of fork.
    ### todo, win32 also does not have fork, so need to do more research.
    return 0 if IS_VMS;

    ### ipc::open3 works on every platform, but it can't capture buffers
    ### on win32 :(
    return unless can_load(
        modules => { map {$_ => '0.0'} qw|IPC::Open3 IO::Select Symbol| },
        verbose => ($WARN && $verbose),
    );
    
    return 1;
}


sub can_capture_buffer {
    my $self    = shift;

    return 1 if $USE_IPC_RUN    && $self->can_use_ipc_run; 
    return 1 if $USE_IPC_OPEN3  && $self->can_use_ipc_open3 && !IS_WIN32; 
    return;
}



sub can_run {
    my $command = shift;

    # a lot of VMS executables have a symbol defined
    # check those first
    if ( $^O eq 'VMS' ) {
        require VMS::DCLsym;
        my $syms = VMS::DCLsym->new;
        return $command if scalar $syms->getsym( uc $command );
    }

    require Config;
    require File::Spec;
    require ExtUtils::MakeMaker;

    if( File::Spec->file_name_is_absolute($command) ) {
        return MM->maybe_command($command);

    } else {
        for my $dir (
            (split /\Q$Config::Config{path_sep}\E/, $ENV{PATH}),
            File::Spec->curdir
        ) {           
            my $abs = File::Spec->catfile($dir, $command);
            return $abs if $abs = MM->maybe_command($abs);
        }
    }
}


sub run {
    my %hash = @_;
    
    ### if the user didn't provide a buffer, we'll store it here.
    my $def_buf = '';
    
    my($verbose,$cmd,$buffer);
    my $tmpl = {
        verbose => { default  => $VERBOSE,  store => \$verbose },
        buffer  => { default  => \$def_buf, store => \$buffer },
        command => { required => 1,         store => \$cmd,
                     allow    => sub { !ref($_[0]) or ref($_[0]) eq 'ARRAY' } 
        },
    };

    unless( check( $tmpl, \%hash, $VERBOSE ) ) {
        Carp::carp(loc("Could not validate input: %1", Params::Check->last_error));
        return;
    };        

    print loc("Running [%1]...\n", (ref $cmd ? "@$cmd" : $cmd)) if $verbose;

    ### did the user pass us a buffer to fill or not? if so, set this
    ### flag so we know what is expected of us
    ### XXX this is now being ignored. in the future, we could add diagnostic
    ### messages based on this logic
    #my $user_provided_buffer = $buffer == \$def_buf ? 0 : 1;
    
    ### buffers that are to be captured
    my( @buffer, @buff_err, @buff_out );

    ### capture STDOUT
    my $_out_handler = sub {
        my $buf = shift;
        return unless defined $buf;
        
        print STDOUT $buf if $verbose;
        push @buffer,   $buf;
        push @buff_out, $buf;
    };
    
    ### capture STDERR
    my $_err_handler = sub {
        my $buf = shift;
        return unless defined $buf;
        
        print STDERR $buf if $verbose;
        push @buffer,   $buf;
        push @buff_err, $buf;
    };
    

    ### flag to indicate we have a buffer captured
    my $have_buffer = __PACKAGE__->can_capture_buffer ? 1 : 0;
    
    ### flag indicating if the subcall went ok
    my $ok;
    
    ### IPC::Run is first choice if $USE_IPC_RUN is set.
    if( $USE_IPC_RUN and __PACKAGE__->can_use_ipc_run( 1 ) ) {
        ### ipc::run handlers needs the command as a string or an array ref

        __PACKAGE__->_debug( "# Using IPC::Run. Have buffer: $have_buffer" )
            if $DEBUG;
            
        $ok = __PACKAGE__->_ipc_run( $cmd, $_out_handler, $_err_handler );

    ### since IPC::Open3 works on all platforms, and just fails on
    ### win32 for capturing buffers, do that ideally
    } elsif ( $USE_IPC_OPEN3 and __PACKAGE__->can_use_ipc_open3( 1 ) ) {

        __PACKAGE__->_debug( "# Using IPC::Open3. Have buffer: $have_buffer" )
            if $DEBUG;

        ### in case there are pipes in there;
        ### IPC::Open3 will call exec and exec will do the right thing 
        $ok = __PACKAGE__->_open3_run( 
                                ( ref $cmd ? "@$cmd" : $cmd ),
                                $_out_handler, $_err_handler, $verbose 
                            );
        
    ### if we are allowed to run verbose, just dispatch the system command
    } else {
        __PACKAGE__->_debug( "# Using system(). Have buffer: $have_buffer" )
            if $DEBUG;
        $ok = __PACKAGE__->_system_run( (ref $cmd ? "@$cmd" : $cmd), $verbose );
    }
    
    ### fill the buffer;
    $$buffer = join '', @buffer if @buffer;
    
    ### return a list of flags and buffers (if available) in list
    ### context, or just a simple 'ok' in scalar
    return wantarray
                ? $have_buffer
                    ? ($ok, $?, \@buffer, \@buff_out, \@buff_err)
                    : ($ok, $? )
                : $ok
    
    
}

sub _open3_run { 
    my $self            = shift;
    my $cmd             = shift;
    my $_out_handler    = shift;
    my $_err_handler    = shift;
    my $verbose         = shift || 0;

    ### Following code are adapted from Friar 'abstracts' in the
    ### Perl Monastery (http://www.perlmonks.org/index.pl?node_id=151886).
    ### XXX that code didn't work.
    ### we now use the following code, thanks to theorbtwo

    ### define them beforehand, so we always have defined FH's
    ### to read from.
    use Symbol;    
    my $kidout      = Symbol::gensym();
    my $kiderror    = Symbol::gensym();

    ### Dup the filehandle so we can pass 'our' STDIN to the
    ### child process. This stops us from having to pump input
    ### from ourselves to the childprocess. However, we will need
    ### to revive the FH afterwards, as IPC::Open3 closes it.
    ### We'll do the same for STDOUT and STDERR. It works without
    ### duping them on non-unix derivatives, but not on win32.
    my @fds_to_dup = ( IS_WIN32 && !$verbose 
                            ? qw[STDIN STDOUT STDERR] 
                            : qw[STDIN]
                        );
    __PACKAGE__->__dup_fds( @fds_to_dup );
    

    my $pid = IPC::Open3::open3(
                    '<&STDIN',
                    (IS_WIN32 ? '>&STDOUT' : $kidout),
                    (IS_WIN32 ? '>&STDERR' : $kiderror),
                    $cmd
                );

    ### use OUR stdin, not $kidin. Somehow,
    ### we never get the input.. so jump through
    ### some hoops to do it :(
    my $selector = IO::Select->new(
                        (IS_WIN32 ? \*STDERR : $kiderror), 
                        \*STDIN,   
                        (IS_WIN32 ? \*STDOUT : $kidout)     
                    );              

    STDOUT->autoflush(1);   STDERR->autoflush(1);   STDIN->autoflush(1);
    $kidout->autoflush(1)   if UNIVERSAL::can($kidout,   'autoflush');
    $kiderror->autoflush(1) if UNIVERSAL::can($kiderror, 'autoflush');

    ### add an epxlicit break statement
    ### code courtesy of theorbtwo from #london.pm
    my $stdout_done = 0;
    my $stderr_done = 0;
    OUTER: while ( my @ready = $selector->can_read ) {

        for my $h ( @ready ) {
            my $buf;
            
            ### $len is the amount of bytes read
            my $len = sysread( $h, $buf, 4096 );    # try to read 4096 bytes
            
            ### see perldoc -f sysread: it returns undef on error,
            ### so bail out.
            if( not defined $len ) {
                warn(loc("Error reading from process: %1", $!));
                last OUTER;
            }
            
            ### check for $len. it may be 0, at which point we're
            ### done reading, so don't try to process it.
            ### if we would print anyway, we'd provide bogus information
            $_out_handler->( "$buf" ) if $len && $h == $kidout;
            $_err_handler->( "$buf" ) if $len && $h == $kiderror;

            ### Wait till child process is done printing to both
            ### stdout and stderr.
            $stdout_done = 1 if $h == $kidout   and $len == 0;
            $stderr_done = 1 if $h == $kiderror and $len == 0;
            last OUTER if ($stdout_done && $stderr_done);
        }
    }

    waitpid $pid, 0; # wait for it to die

    ### restore STDIN after duping, or STDIN will be closed for
    ### this current perl process!
    __PACKAGE__->__reopen_fds( @fds_to_dup );
    
    return if $?;   # some error occurred
    return 1;
}


sub _ipc_run {  
    my $self            = shift;
    my $cmd             = shift;
    my $_out_handler    = shift;
    my $_err_handler    = shift;
    
    STDOUT->autoflush(1); STDERR->autoflush(1);

    ### a command like:
    # [
    #     '/usr/bin/gzip',
    #     '-cdf',
    #     '/Users/kane/sources/p4/other/archive-extract/t/src/x.tgz',
    #     '|',
    #     '/usr/bin/tar',
    #     '-tf -'
    # ]
    ### needs to become:
    # [
    #     ['/usr/bin/gzip', '-cdf',
    #       '/Users/kane/sources/p4/other/archive-extract/t/src/x.tgz']
    #     '|',
    #     ['/usr/bin/tar', '-tf -']
    # ]

    
    my @command; my $special_chars;
    if( ref $cmd ) {
        my $aref = [];
        for my $item (@$cmd) {
            if( $item =~ /([<>|&])/ ) {
                push @command, $aref, $item;
                $aref = [];
                $special_chars .= $1;
            } else {
                push @$aref, $item;
            }
        }
        push @command, $aref;
    } else {
        @command = map { if( /([<>|&])/ ) {
                            $special_chars .= $1; $_;
                         } else {
                            [ split / +/ ]
                         }
                    } split( /\s*([<>|&])\s*/, $cmd );
    }
 
    ### if there's a pipe in the command, *STDIN needs to 
    ### be inserted *BEFORE* the pipe, to work on win32
    ### this also works on *nix, so we should do it when possible
    ### this should *also* work on multiple pipes in the command
    ### if there's no pipe in the command, append STDIN to the back
    ### of the command instead.
    ### XXX seems IPC::Run works it out for itself if you just
    ### dont pass STDIN at all.
    #     if( $special_chars and $special_chars =~ /\|/ ) {
    #         ### only add STDIN the first time..
    #         my $i;
    #         @command = map { ($_ eq '|' && not $i++) 
    #                             ? ( \*STDIN, $_ ) 
    #                             : $_ 
    #                         } @command; 
    #     } else {
    #         push @command, \*STDIN;
    #     }
  
 
    # \*STDIN is already included in the @command, see a few lines up
    return IPC::Run::run(   @command, 
                            fileno(STDOUT).'>',
                            $_out_handler,
                            fileno(STDERR).'>',
                            $_err_handler
                        );
}

sub _system_run { 
    my $self    = shift;
    my $cmd     = shift;
    my $verbose = shift || 0;

    my @fds_to_dup = $verbose ? () : qw[STDOUT STDERR];
    __PACKAGE__->__dup_fds( @fds_to_dup );
    
    ### system returns 'true' on failure -- the exit code of the cmd
    system( $cmd );
    
    __PACKAGE__->__reopen_fds( @fds_to_dup );
    
    return if $?;
    return 1;
}

{   use File::Spec;
    use Symbol;

    my %Map = (
        STDOUT => [qw|>&|, \*STDOUT, Symbol::gensym() ],
        STDERR => [qw|>&|, \*STDERR, Symbol::gensym() ],
        STDIN  => [qw|<&|, \*STDIN,  Symbol::gensym() ],
    );

    ### dups FDs and stores them in a cache
    sub __dup_fds {
        my $self    = shift;
        my @fds     = @_;

        __PACKAGE__->_debug( "# Closing the following fds: @fds" ) if $DEBUG;

        for my $name ( @fds ) {
            my($redir, $fh, $glob) = @{$Map{$name}} or (
                Carp::carp(loc("No such FD: '%1'", $name)), next );
            
            ### MUST use the 2-arg version of open for dup'ing for 
            ### 5.6.x compatibilty. 5.8.x can use 3-arg open
            ### see perldoc5.6.2 -f open for details            
            open $glob, $redir . fileno($fh) or (
                        Carp::carp(loc("Could not dup '$name': %1", $!)),
                        return
                    );        
                
            ### we should re-open this filehandle right now, not
            ### just dup it
            ### Use 2-arg version of open, as 5.5.x doesn't support
            ### 3-arg version =/
            if( $redir eq '>&' ) {
                open( $fh, '>' . File::Spec->devnull ) or (
                    Carp::carp(loc("Could not reopen '$name': %1", $!)),
                    return
                );
            }
        }
        
        return 1;
    }

    ### reopens FDs from the cache    
    sub __reopen_fds {
        my $self    = shift;
        my @fds     = @_;

        __PACKAGE__->_debug( "# Reopening the following fds: @fds" ) if $DEBUG;

        for my $name ( @fds ) {
            my($redir, $fh, $glob) = @{$Map{$name}} or (
                Carp::carp(loc("No such FD: '%1'", $name)), next );

            ### MUST use the 2-arg version of open for dup'ing for 
            ### 5.6.x compatibilty. 5.8.x can use 3-arg open
            ### see perldoc5.6.2 -f open for details
            open( $fh, $redir . fileno($glob) ) or (
                    Carp::carp(loc("Could not restore '$name': %1", $!)),
                    return
                ); 
           
            ### close this FD, we're not using it anymore
            close $glob;                
        }                
        return 1;                
    
    }
}    

sub _debug {
    my $self    = shift;
    my $msg     = shift or return;
    my $level   = shift || 0;
    
    local $Carp::CarpLevel += $level;
    Carp::carp($msg);
    
    return 1;
}


1;


__END__

