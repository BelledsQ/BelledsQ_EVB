package IO::Compress::Zip ;

use strict ;
use warnings;
use bytes;

use IO::Compress::Base::Common  2.008 qw(:Status createSelfTiedObject);
use IO::Compress::RawDeflate 2.008 ;
use IO::Compress::Adapter::Deflate 2.008 ;
use IO::Compress::Adapter::Identity 2.008 ;
use IO::Compress::Zlib::Extra 2.008 ;
use IO::Compress::Zip::Constants 2.008 ;


use Compress::Raw::Zlib  2.008 qw(crc32) ;
BEGIN
{
    eval { require IO::Compress::Adapter::Bzip2 ; 
           import  IO::Compress::Adapter::Bzip2 2.008 ; 
           require IO::Compress::Bzip2 ; 
           import  IO::Compress::Bzip2 2.008 ; 
         } ;
}


require Exporter ;

our ($VERSION, @ISA, @EXPORT_OK, %EXPORT_TAGS, $ZipError);

$VERSION = '2.008';
$ZipError = '';

@ISA = qw(Exporter IO::Compress::RawDeflate);
@EXPORT_OK = qw( $ZipError zip ) ;
%EXPORT_TAGS = %IO::Compress::RawDeflate::DEFLATE_CONSTANTS ;
push @{ $EXPORT_TAGS{all} }, @EXPORT_OK ;

$EXPORT_TAGS{zip_method} = [qw( ZIP_CM_STORE ZIP_CM_DEFLATE ZIP_CM_BZIP2 )];
push @{ $EXPORT_TAGS{all} }, @{ $EXPORT_TAGS{zip_method} };

Exporter::export_ok_tags('all');

sub new
{
    my $class = shift ;

    my $obj = createSelfTiedObject($class, \$ZipError);    
    $obj->_create(undef, @_);
}

sub zip
{
    my $obj = createSelfTiedObject(undef, \$ZipError);    
    return $obj->_def(@_);
}

sub mkComp
{
    my $self = shift ;
    my $class = shift ;
    my $got = shift ;

    my ($obj, $errstr, $errno) ;

    if (*$self->{ZipData}{Method} == ZIP_CM_STORE) {
        ($obj, $errstr, $errno) = IO::Compress::Adapter::Identity::mkCompObject(
                                                 $got->value('Level'),
                                                 $got->value('Strategy')
                                                 );
    }
    elsif (*$self->{ZipData}{Method} == ZIP_CM_DEFLATE) {
        ($obj, $errstr, $errno) = IO::Compress::Adapter::Deflate::mkCompObject(
                                                 $got->value('CRC32'),
                                                 $got->value('Adler32'),
                                                 $got->value('Level'),
                                                 $got->value('Strategy')
                                                 );
    }
    elsif (*$self->{ZipData}{Method} == ZIP_CM_BZIP2) {
        ($obj, $errstr, $errno) = IO::Compress::Adapter::Bzip2::mkCompObject(
                                                $got->value('BlockSize100K'),
                                                $got->value('WorkFactor'),
                                                $got->value('Verbosity')
                                               );
        *$self->{ZipData}{CRC32} = crc32(undef);
    }

    return $self->saveErrorString(undef, $errstr, $errno)
       if ! defined $obj;

    if (! defined *$self->{ZipData}{StartOffset}) {
        *$self->{ZipData}{StartOffset} = 0;
        *$self->{ZipData}{Offset} = new U64 ;
    }

    return $obj;    
}

sub reset
{
    my $self = shift ;

    *$self->{Compress}->reset();
    *$self->{ZipData}{CRC32} = Compress::Raw::Zlib::crc32('');

    return STATUS_OK;    
}

sub filterUncompressed
{
    my $self = shift ;

    if (*$self->{ZipData}{Method} == ZIP_CM_DEFLATE) {
        *$self->{ZipData}{CRC32} = *$self->{Compress}->crc32();
    }
    else {
        *$self->{ZipData}{CRC32} = crc32(${$_[0]}, *$self->{ZipData}{CRC32});

    }
}

sub mkHeader
{
    my $self  = shift;
    my $param = shift ;
    
    *$self->{ZipData}{StartOffset} = *$self->{ZipData}{Offset}->get32bit() ;

    my $filename = '';
    $filename = $param->value('Name') || '';

    my $comment = '';
    $comment = $param->value('Comment') || '';

    my $hdr = '';

    my $time = _unixToDosTime($param->value('Time'));

    my $extra = '';
    my $ctlExtra = '';
    my $empty = 0;
    my $osCode = $param->value('OS_Code') ;
    my $extFileAttr = 0 ;

    if (*$self->{ZipData}{Zip64}) {
        $empty = 0xFFFF;

        my $x = '';
        $x .= pack "V V", 0, 0 ; # uncompressedLength   
        $x .= pack "V V", 0, 0 ; # compressedLength   
        $x .= *$self->{ZipData}{Offset}->getPacked_V64() ; # offset to local hdr
        #$x .= pack "V  ", 0    ; # disk no

        $x = IO::Compress::Zlib::Extra::mkSubField(ZIP_EXTRA_ID_ZIP64, $x);
        $extra .= $x;
        $ctlExtra .= $x;
    }

    if (! $param->value('Minimal')) {
        if (defined $param->value('exTime'))
        {
            $extra .= mkExtendedTime($param->value('MTime'), 
                                    $param->value('ATime'), 
                                    $param->value('CTime'));

            $ctlExtra .= mkExtendedTime($param->value('MTime'));
        }

        if ( $param->value('UID') && $osCode == ZIP_OS_CODE_UNIX)
        {
            $extra    .= mkUnix2Extra( $param->value('UID'), $param->value('GID'));
            $ctlExtra .= mkUnix2Extra();
        }

        # TODO - this code assumes Unix.
        #$extFileAttr = 0666 << 16 
        #    if $osCode == ZIP_OS_CODE_UNIX ;

        $extFileAttr = $param->value('ExtAttr') 
            if defined $param->value('ExtAttr') ;

        $extra .= $param->value('ExtraFieldLocal') 
            if defined $param->value('ExtraFieldLocal');

        $ctlExtra .= $param->value('ExtraFieldCentral') 
            if defined $param->value('ExtraFieldCentral');
    }

    my $gpFlag = 0 ;    
    $gpFlag |= ZIP_GP_FLAG_STREAMING_MASK
        if *$self->{ZipData}{Stream} ;

    my $method = *$self->{ZipData}{Method} ;

    my $version = $ZIP_CM_MIN_VERSIONS{$method};
    $version = ZIP64_MIN_VERSION
        if ZIP64_MIN_VERSION > $version && *$self->{ZipData}{Zip64};
    my $madeBy = ($param->value('OS_Code') << 8) + $version;
    my $extract = $version;

    *$self->{ZipData}{Version} = $version;
    *$self->{ZipData}{MadeBy} = $madeBy;

    my $ifa = 0;
    $ifa |= ZIP_IFA_TEXT_MASK
        if $param->value('TextFlag');

    $hdr .= pack "V", ZIP_LOCAL_HDR_SIG ; # signature
    $hdr .= pack 'v', $extract   ; # extract Version & OS
    $hdr .= pack 'v', $gpFlag    ; # general purpose flag (set streaming mode)
    $hdr .= pack 'v', $method    ; # compression method (deflate)
    $hdr .= pack 'V', $time      ; # last mod date/time
    $hdr .= pack 'V', 0          ; # crc32               - 0 when streaming
    $hdr .= pack 'V', $empty     ; # compressed length   - 0 when streaming
    $hdr .= pack 'V', $empty     ; # uncompressed length - 0 when streaming
    $hdr .= pack 'v', length $filename ; # filename length
    $hdr .= pack 'v', length $extra ; # extra length
    
    $hdr .= $filename ;
    $hdr .= $extra ;


    my $ctl = '';

    $ctl .= pack "V", ZIP_CENTRAL_HDR_SIG ; # signature
    $ctl .= pack 'v', $madeBy    ; # version made by
    $ctl .= pack 'v', $extract   ; # extract Version
    $ctl .= pack 'v', $gpFlag    ; # general purpose flag (streaming mode)
    $ctl .= pack 'v', $method    ; # compression method (deflate)
    $ctl .= pack 'V', $time      ; # last mod date/time
    $ctl .= pack 'V', 0          ; # crc32
    $ctl .= pack 'V', $empty     ; # compressed length
    $ctl .= pack 'V', $empty     ; # uncompressed length
    $ctl .= pack 'v', length $filename ; # filename length
    $ctl .= pack 'v', length $ctlExtra ; # extra length
    $ctl .= pack 'v', length $comment ;  # file comment length
    $ctl .= pack 'v', 0          ; # disk number start 
    $ctl .= pack 'v', $ifa       ; # internal file attributes
    $ctl .= pack 'V', $extFileAttr   ; # external file attributes
    if (! *$self->{ZipData}{Zip64}) {
        $ctl .= pack 'V', *$self->{ZipData}{Offset}->get32bit()  ; # offset to local header
    }
    else {
        $ctl .= pack 'V', $empty ; # offset to local header
    }
    
    $ctl .= $filename ;
    *$self->{ZipData}{StartOffset64} = 4 + length $ctl;
    $ctl .= $ctlExtra ;
    $ctl .= $comment ;

    *$self->{ZipData}{Offset}->add(length $hdr) ;

    *$self->{ZipData}{CentralHeader} = $ctl;

    return $hdr;
}

sub mkTrailer
{
    my $self = shift ;

    my $crc32 ;
    if (*$self->{ZipData}{Method} == ZIP_CM_DEFLATE) {
        $crc32 = pack "V", *$self->{Compress}->crc32();
    }
    else {
        $crc32 = pack "V", *$self->{ZipData}{CRC32};
    }

    my $ctl = *$self->{ZipData}{CentralHeader} ;

    my $sizes ;
    if (! *$self->{ZipData}{Zip64}) {
        $sizes .= *$self->{CompSize}->getPacked_V32() ;   # Compressed size
        $sizes .= *$self->{UnCompSize}->getPacked_V32() ; # Uncompressed size
    }
    else {
        $sizes .= *$self->{CompSize}->getPacked_V64() ;   # Compressed size
        $sizes .= *$self->{UnCompSize}->getPacked_V64() ; # Uncompressed size
    }

    my $data = $crc32 . $sizes ;


    my $hdr = '';

    if (*$self->{ZipData}{Stream}) {
        $hdr  = pack "V", ZIP_DATA_HDR_SIG ;                       # signature
        $hdr .= $data ;
    }
    else {
        $self->writeAt(*$self->{ZipData}{StartOffset} + 14, $data)
            or return undef;
    }

    if (! *$self->{ZipData}{Zip64})
      { substr($ctl, 16, length $data) = $data }
    else {
        substr($ctl, 16, length $crc32) = $crc32 ;
        my $s  = *$self->{UnCompSize}->getPacked_V64() ; # Uncompressed size
           $s .= *$self->{CompSize}->getPacked_V64() ;   # Compressed size
        substr($ctl, *$self->{ZipData}{StartOffset64}, length $s) = $s ;
    }

    *$self->{ZipData}{Offset}->add(length($hdr));
    *$self->{ZipData}{Offset}->add( *$self->{CompSize} );
    push @{ *$self->{ZipData}{CentralDir} }, $ctl ;

    return $hdr;
}

sub mkFinalTrailer
{
    my $self = shift ;

    my $comment = '';
    $comment = *$self->{ZipData}{ZipComment} ;

    my $cd_offset = *$self->{ZipData}{Offset}->get32bit() ; # offset to start central dir

    my $entries = @{ *$self->{ZipData}{CentralDir} };
    my $cd = join '', @{ *$self->{ZipData}{CentralDir} };
    my $cd_len = length $cd ;

    my $z64e = '';

    if ( *$self->{ZipData}{Zip64} ) {

        my $v  = *$self->{ZipData}{Version} ;
        my $mb = *$self->{ZipData}{MadeBy} ;
        $z64e .= pack 'v', $v             ; # Version made by
        $z64e .= pack 'v', $mb            ; # Version to extract
        $z64e .= pack 'V', 0              ; # number of disk
        $z64e .= pack 'V', 0              ; # number of disk with central dir
        $z64e .= U64::pack_V64 $entries   ; # entries in central dir on this disk
        $z64e .= U64::pack_V64 $entries   ; # entries in central dir
        $z64e .= U64::pack_V64 $cd_len    ; # size of central dir
        $z64e .= *$self->{ZipData}{Offset}->getPacked_V64() ; # offset to start central dir

        $z64e  = pack("V", ZIP64_END_CENTRAL_REC_HDR_SIG) # signature
              .  U64::pack_V64(length $z64e)
              .  $z64e ;

        *$self->{ZipData}{Offset}->add(length $cd) ; 

        $z64e .= pack "V", ZIP64_END_CENTRAL_LOC_HDR_SIG; # signature
        $z64e .= pack 'V', 0              ; # number of disk with central dir
        $z64e .= *$self->{ZipData}{Offset}->getPacked_V64() ; # offset to end zip64 central dir
        $z64e .= pack 'V', 1              ; # Total number of disks 

        # TODO - fix these when info-zip 3 is fixed.
        #$cd_len = 
        #$cd_offset = 
        $entries = 0xFFFF ;
    }

    my $ecd = '';
    $ecd .= pack "V", ZIP_END_CENTRAL_HDR_SIG ; # signature
    $ecd .= pack 'v', 0          ; # number of disk
    $ecd .= pack 'v', 0          ; # number of disk with central dir
    $ecd .= pack 'v', $entries   ; # entries in central dir on this disk
    $ecd .= pack 'v', $entries   ; # entries in central dir
    $ecd .= pack 'V', $cd_len    ; # size of central dir
    $ecd .= pack 'V', $cd_offset ; # offset to start central dir
    $ecd .= pack 'v', length $comment ; # zipfile comment length
    $ecd .= $comment;

    return $cd . $z64e . $ecd ;
}

sub ckParams
{
    my $self = shift ;
    my $got = shift;
    
    $got->value('CRC32' => 1);

    if (! $got->parsed('Time') ) {
        # Modification time defaults to now.
        $got->value('Time' => time) ;
    }

    if (! $got->parsed('exTime') ) {
        my $timeRef = $got->value('exTime');
        if ( defined $timeRef) {
            return $self->saveErrorString(undef, "exTime not a 3-element array ref")   
                if ref $timeRef ne 'ARRAY' || @$timeRef != 3;
        }

        $got->value("MTime", $timeRef->[1]);
        $got->value("ATime", $timeRef->[0]);
        $got->value("CTime", $timeRef->[2]);
    }

    *$self->{ZipData}{Zip64} = $got->value('Zip64');
    *$self->{ZipData}{Stream} = $got->value('Stream');

    return $self->saveErrorString(undef, "Zip64 only supported if Stream enabled")   
        if  *$self->{ZipData}{Zip64} && ! *$self->{ZipData}{Stream} ;

    my $method = $got->value('Method');
    return $self->saveErrorString(undef, "Unknown Method '$method'")   
        if ! defined $ZIP_CM_MIN_VERSIONS{$method};

    return $self->saveErrorString(undef, "Bzip2 not available")
        if $method == ZIP_CM_BZIP2 and 
           ! defined $IO::Compress::Adapter::Bzip2::VERSION;

    *$self->{ZipData}{Method} = $method;

    *$self->{ZipData}{ZipComment} = $got->value('ZipComment') ;

    for my $name (qw( ExtraFieldLocal ExtraFieldCentral ))
    {
        my $data = $got->value($name) ;
        if (defined $data) {
            my $bad = IO::Compress::Zlib::Extra::parseExtraField($data, 1, 0) ;
            return $self->saveErrorString(undef, "Error with $name Parameter: $bad")
                if $bad ;

            $got->value($name, $data) ;
        }
    }

    return undef
        if defined $IO::Compress::Bzip2::VERSION
            and ! IO::Compress::Bzip2::ckParams($self, $got);

    return 1 ;
}


sub getExtraParams
{
    my $self = shift ;

    use IO::Compress::Base::Common  2.008 qw(:Parse);
    use Compress::Raw::Zlib  2.008 qw(Z_DEFLATED Z_DEFAULT_COMPRESSION Z_DEFAULT_STRATEGY);

    my @Bzip2 = ();
    
    @Bzip2 = IO::Compress::Bzip2::getExtraParams($self)
        if defined $IO::Compress::Bzip2::VERSION;
    
    return (
            # zlib behaviour
            $self->getZlibParams(),

            'Stream'    => [1, 1, Parse_boolean,   1],
           #'Store'     => [0, 1, Parse_boolean,   0],
            'Method'    => [0, 1, Parse_unsigned,  ZIP_CM_DEFLATE],
            
            'Minimal'   => [0, 1, Parse_boolean,   0],
            'Zip64'     => [0, 1, Parse_boolean,   0],
            'Comment'   => [0, 1, Parse_any,       ''],
            'ZipComment'=> [0, 1, Parse_any,       ''],
            'Name'      => [0, 1, Parse_any,       ''],
            'Time'      => [0, 1, Parse_any,       undef],
            'exTime'    => [0, 1, Parse_any,       undef],
            'ExtAttr'   => [0, 1, Parse_any,       0],
            'OS_Code'   => [0, 1, Parse_unsigned,  $Compress::Raw::Zlib::gzip_os_code],
            
           'TextFlag'  => [0, 1, Parse_boolean,   0],
           'ExtraFieldLocal'  => [0, 1, Parse_any,    undef],
           'ExtraFieldCentral'=> [0, 1, Parse_any,    undef],

            @Bzip2,
        );
}

sub getInverseClass
{
    return ('IO::Uncompress::Unzip',
                \$IO::Uncompress::Unzip::UnzipError);
}

sub getFileInfo
{
    my $self = shift ;
    my $params = shift;
    my $filename = shift ;

    my ($mode, $uid, $gid, $atime, $mtime, $ctime) 
                = (stat($filename))[2, 4,5, 8,9,10] ;

    $params->value('Name' => $filename)
        if ! $params->parsed('Name') ;

    $params->value('Time' => $mtime) 
        if ! $params->parsed('Time') ;
    
    if ( ! $params->parsed('exTime'))
    {
        $params->value('MTime' => $mtime) ;
        $params->value('ATime' => $atime) ;
        $params->value('CTime' => undef) ; # No Creation time
    }

    $params->value('ExtAttr' => $mode << 16) 
        if ! $params->parsed('ExtAttr');

    $params->value('UID' => $uid) ;
    $params->value('GID' => $gid) ;
    
}

sub mkExtendedTime
{
    # order expected is m, a, c

    my $times = '';
    my $bit = 1 ;
    my $flags = 0;

    for my $time (@_)
    {
        if (defined $time)
        {
            $flags |= $bit;
            $times .= pack("V", $time);
        }

        $bit <<= 1 ;
    }

    return IO::Compress::Zlib::Extra::mkSubField(ZIP_EXTRA_ID_EXT_TIMESTAMP,
                                                 pack("C", $flags) .  $times);
}

sub mkUnix2Extra
{
    my $ids = '';
    for my $id (@_)
    {
        $ids .= pack("v", $id);
    }

    return IO::Compress::Zlib::Extra::mkSubField(ZIP_EXTRA_ID_INFO_ZIP_UNIX2, 
                                                 $ids);
}


sub _unixToDosTime    # Archive::Zip::Member
{
	my $time_t = shift;
    # TODO - add something to cope with unix time < 1980 
	my ( $sec, $min, $hour, $mday, $mon, $year ) = localtime($time_t);
	my $dt = 0;
	$dt += ( $sec >> 1 );
	$dt += ( $min << 5 );
	$dt += ( $hour << 11 );
	$dt += ( $mday << 16 );
	$dt += ( ( $mon + 1 ) << 21 );
	$dt += ( ( $year - 80 ) << 25 );
	return $dt;
}

1;

__END__

