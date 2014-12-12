package IO::Uncompress::AnyUncompress ;

use strict;
use warnings;
use bytes;

use IO::Compress::Base::Common 2.008 qw(createSelfTiedObject);

use IO::Uncompress::Base 2.008 ;


require Exporter ;

our ($VERSION, @ISA, @EXPORT_OK, %EXPORT_TAGS, $AnyUncompressError);

$VERSION = '2.008';
$AnyUncompressError = '';

@ISA = qw( Exporter IO::Uncompress::Base );
@EXPORT_OK = qw( $AnyUncompressError anyuncompress ) ;
%EXPORT_TAGS = %IO::Uncompress::Base::DEFLATE_CONSTANTS ;
push @{ $EXPORT_TAGS{all} }, @EXPORT_OK ;
Exporter::export_ok_tags('all');


BEGIN
{
   eval ' use IO::Uncompress::Adapter::Inflate 2.008 ;';
   eval ' use IO::Uncompress::Adapter::Bunzip2 2.008 ;';
   eval ' use IO::Uncompress::Adapter::LZO 2.008 ;';
   eval ' use IO::Uncompress::Adapter::Lzf 2.008 ;';

   eval ' use IO::Uncompress::Bunzip2 2.008 ;';
   eval ' use IO::Uncompress::UnLzop 2.008 ;';
   eval ' use IO::Uncompress::Gunzip 2.008 ;';
   eval ' use IO::Uncompress::Inflate 2.008 ;';
   eval ' use IO::Uncompress::RawInflate 2.008 ;';
   eval ' use IO::Uncompress::Unzip 2.008 ;';
   eval ' use IO::Uncompress::UnLzf 2.008 ;';
}

sub new
{
    my $class = shift ;
    my $obj = createSelfTiedObject($class, \$AnyUncompressError);
    $obj->_create(undef, 0, @_);
}

sub anyuncompress
{
    my $obj = createSelfTiedObject(undef, \$AnyUncompressError);
    return $obj->_inf(@_) ;
}

sub getExtraParams
{
    use IO::Compress::Base::Common 2.008 qw(:Parse);
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

    my $magic ;

    # try zlib first
    if (defined $IO::Uncompress::RawInflate::VERSION )
    {
        my ($obj, $errstr, $errno) = IO::Uncompress::Adapter::Inflate::mkUncompObject();

        return $self->saveErrorString(undef, $errstr, $errno)
            if ! defined $obj;

        *$self->{Uncomp} = $obj;
        
        my @possible = qw( Inflate Gunzip Unzip );
        unshift @possible, 'RawInflate' 
            if $got->value('RawInflate');

        $magic = $self->ckMagic( @possible );
        
        if ($magic) {
            *$self->{Info} = $self->readHeader($magic)
                or return undef ;

            return 1;
        }
     }

     if (defined $IO::Uncompress::Bunzip2::VERSION and
         $magic = $self->ckMagic('Bunzip2')) {
        *$self->{Info} = $self->readHeader($magic)
            or return undef ;

        my ($obj, $errstr, $errno) = IO::Uncompress::Adapter::Bunzip2::mkUncompObject();

        return $self->saveErrorString(undef, $errstr, $errno)
            if ! defined $obj;

        *$self->{Uncomp} = $obj;

         return 1;
     }

     if (defined $IO::Uncompress::UnLzop::VERSION and
            $magic = $self->ckMagic('UnLzop')) {

        *$self->{Info} = $self->readHeader($magic)
            or return undef ;

        my ($obj, $errstr, $errno) = IO::Uncompress::Adapter::LZO::mkUncompObject();

        return $self->saveErrorString(undef, $errstr, $errno)
            if ! defined $obj;

        *$self->{Uncomp} = $obj;

         return 1;
     }

     if (defined $IO::Uncompress::UnLzf::VERSION and
            $magic = $self->ckMagic('UnLzf')) {

        *$self->{Info} = $self->readHeader($magic)
            or return undef ;

        my ($obj, $errstr, $errno) = IO::Uncompress::Adapter::Lzf::mkUncompObject();

        return $self->saveErrorString(undef, $errstr, $errno)
            if ! defined $obj;

        *$self->{Uncomp} = $obj;

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


