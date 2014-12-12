package IO::Uncompress::AnyInflate ;


use strict;
use warnings;
use bytes;

use IO::Compress::Base::Common  2.008 qw(createSelfTiedObject);

use IO::Uncompress::Adapter::Inflate  2.008 ();


use IO::Uncompress::Base  2.008 ;
use IO::Uncompress::Gunzip  2.008 ;
use IO::Uncompress::Inflate  2.008 ;
use IO::Uncompress::RawInflate  2.008 ;
use IO::Uncompress::Unzip  2.008 ;

require Exporter ;

our ($VERSION, @ISA, @EXPORT_OK, %EXPORT_TAGS, $AnyInflateError);

$VERSION = '2.008';
$AnyInflateError = '';

@ISA = qw( Exporter IO::Uncompress::Base );
@EXPORT_OK = qw( $AnyInflateError anyinflate ) ;
%EXPORT_TAGS = %IO::Uncompress::Base::DEFLATE_CONSTANTS ;
push @{ $EXPORT_TAGS{all} }, @EXPORT_OK ;
Exporter::export_ok_tags('all');


sub new
{
    my $class = shift ;
    my $obj = createSelfTiedObject($class, \$AnyInflateError);
    $obj->_create(undef, 0, @_);
}

sub anyinflate
{
    my $obj = createSelfTiedObject(undef, \$AnyInflateError);
    return $obj->_inf(@_) ;
}

sub getExtraParams
{
    use IO::Compress::Base::Common  2.008 qw(:Parse);
    return ( 'RawInflate' => [1, 1, Parse_boolean,  0] ) ;
}

sub ckParams
{
    my $self = shift ;
    my $got = shift ;

    # any always needs both crc32 and adler32
    $got->value('CRC32' => 1);
    $got->value('ADLER32' => 1);

    return 1;
}

sub mkUncomp
{
    my $self = shift ;
    my $class = shift ;
    my $got = shift ;

    my ($obj, $errstr, $errno) = IO::Uncompress::Adapter::Inflate::mkUncompObject();

    return $self->saveErrorString(undef, $errstr, $errno)
        if ! defined $obj;

    *$self->{Uncomp} = $obj;
    
     my @possible = qw( Inflate Gunzip Unzip );
     unshift @possible, 'RawInflate' 
        if 1 || $got->value('RawInflate');

     my $magic = $self->ckMagic( @possible );

     if ($magic) {
        *$self->{Info} = $self->readHeader($magic)
            or return undef ;

        return 1;
     }

     return 0 ;
}



sub ckMagic
{
    my $self = shift;
    my @names = @_ ;

    my $keep = ref $self ;
    for my $class ( map { "IO::Uncompress::$_" } @names)
    {
        bless $self => $class;
        my $magic = $self->ckMagic();

        if ($magic)
        {
            #bless $self => $class;
            return $magic ;
        }

        $self->pushBack(*$self->{HeaderPending})  ;
        *$self->{HeaderPending} = ''  ;
    }    

    bless $self => $keep;
    return undef;
}

1 ;

__END__


