package IO::Uncompress::Unzip;

require 5.004 ;


use strict ;
use warnings;
use bytes;

use IO::Uncompress::RawInflate  2.008 ;
use IO::Compress::Base::Common  2.008 qw(:Status createSelfTiedObject);
use IO::Uncompress::Adapter::Identity 2.008 ;
use IO::Compress::Zlib::Extra 2.008 ;
use IO::Compress::Zip::Constants 2.008 ;

use Compress::Raw::Zlib  2.008 qw(crc32) ;

BEGIN
{
    eval { require IO::Uncompress::Adapter::Bunzip2 ;
           import  IO::Uncompress::Adapter::Bunzip2 } ;
}


require Exporter ;

our ($VERSION, @ISA, @EXPORT_OK, %EXPORT_TAGS, $UnzipError, %headerLookup);

$VERSION = '2.008';
$UnzipError = '';

@ISA    = qw(Exporter IO::Uncompress::RawInflate);
@EXPORT_OK = qw( $UnzipError unzip );
%EXPORT_TAGS = %IO::Uncompress::RawInflate::EXPORT_TAGS ;
push @{ $EXPORT_TAGS{all} }, @EXPORT_OK ;
Exporter::export_ok_tags('all');

%headerLookup = (
        ZIP_CENTRAL_HDR_SIG,            \&skipCentralDirectory,
        ZIP_END_CENTRAL_HDR_SIG,        \&skipEndCentralDirectory,
        ZIP64_END_CENTRAL_REC_HDR_SIG,  \&skipCentralDirectory64Rec,
        ZIP64_END_CENTRAL_LOC_HDR_SIG,  \&skipCentralDirectory64Loc,
        ZIP64_ARCHIVE_EXTRA_SIG,        \&skipArchiveExtra,
        ZIP64_DIGITAL_SIGNATURE_SIG,    \&skipDigitalSignature,
        );

sub new
{
    my $class = shift ;
    my $obj = createSelfTiedObject($class, \$UnzipError);
    $obj->_create(undef, 0, @_);
}

sub unzip
{
    my $obj = createSelfTiedObject(undef, \$UnzipError);
    return $obj->_inf(@_) ;
}

sub getExtraParams
{
    use IO::Compress::Base::Common  2.008 qw(:Parse);

    
    return (
            'Name'      => [1, 1, Parse_any,       undef],

        );    
}

sub ckParams
{
    my $self = shift ;
    my $got = shift ;

    # unzip always needs crc32
    $got->value('CRC32' => 1);

    *$self->{UnzipData}{Name} = $got->value('Name');

    return 1;
}


sub ckMagic
{
    my $self = shift;

    my $magic ;
    $self->smartReadExact(\$magic, 4);

    *$self->{HeaderPending} = $magic ;

    return $self->HeaderError("Minimum header size is " . 
                              4 . " bytes") 
        if length $magic != 4 ;                                    

    return $self->HeaderError("Bad Magic")
        if ! _isZipMagic($magic) ;

    *$self->{Type} = 'zip';

    return $magic ;
}



sub readHeader
{
    my $self = shift;
    my $magic = shift ;

    my $name =  *$self->{UnzipData}{Name} ;
    my $hdr = $self->_readZipHeader($magic) ;

    while (defined $hdr)
    {
        if (! defined $name || $hdr->{Name} eq $name)
        {
            return $hdr ;
        }

        # skip the data
        my $buffer;
        if (*$self->{ZipData}{Streaming}) {

            while (1) {

                my $b;
                my $status = $self->smartRead(\$b, 1024 * 16);
                return undef
                    if $status <= 0 ;

                my $temp_buf;
                my $out;
                $status = *$self->{Uncomp}->uncompr(\$b, \$temp_buf, 0, $out);

                return $self->saveErrorString(undef, *$self->{Uncomp}{Error}, 
                                                     *$self->{Uncomp}{ErrorNo})
                    if $self->saveStatus($status) == STATUS_ERROR;                

                if ($status == STATUS_ENDSTREAM) {
                    *$self->{Uncomp}->reset();
                    $self->pushBack($b)  ;
                    last;
                }
            }

            # skip the trailer
            $self->smartReadExact(\$buffer, $hdr->{TrailerLength})
                or return $self->saveErrorString(undef, "Truncated file");
        }
        else {
            my $c = $hdr->{CompressedLength}->get32bit();
            $self->smartReadExact(\$buffer, $c)
                or return $self->saveErrorString(undef, "Truncated file");
            $buffer = '';
        }

        $self->chkTrailer($buffer) == STATUS_OK
            or return $self->saveErrorString(undef, "Truncated file");

        $hdr = $self->_readFullZipHeader();

        return $self->saveErrorString(undef, "Cannot find '$name'")
            if $self->smartEof();
    }

    return undef;
}

sub chkTrailer
{
    my $self = shift;
    my $trailer = shift;

    my ($sig, $CRC32, $cSize, $uSize) ;
    my ($cSizeHi, $uSizeHi) = (0, 0);
    if (*$self->{ZipData}{Streaming}) {
        $sig   = unpack ("V", substr($trailer, 0, 4));
        $CRC32 = unpack ("V", substr($trailer, 4, 4));

        if (*$self->{ZipData}{Zip64} ) {
            $cSize = U64::newUnpack_V64 substr($trailer,  8, 8);
            $uSize = U64::newUnpack_V64 substr($trailer, 16, 8);
        }
        else {
            $cSize = U64::newUnpack_V32 substr($trailer,  8, 4);
            $uSize = U64::newUnpack_V32 substr($trailer, 12, 4);
        }

        return $self->TrailerError("Data Descriptor signature, got $sig")
            if $sig != ZIP_DATA_HDR_SIG;
    }
    else {
        ($CRC32, $cSize, $uSize) = 
            (*$self->{ZipData}{Crc32},
             *$self->{ZipData}{CompressedLen},
             *$self->{ZipData}{UnCompressedLen});
    }

    if (*$self->{Strict}) {
        return $self->TrailerError("CRC mismatch")
            if $CRC32  != *$self->{ZipData}{CRC32} ;

        return $self->TrailerError("CSIZE mismatch.")
            if ! $cSize->equal(*$self->{CompSize});

        return $self->TrailerError("USIZE mismatch.")
            if ! $uSize->equal(*$self->{UnCompSize});
    }

    my $reachedEnd = STATUS_ERROR ;
    # check for central directory or end of central directory
    while (1)
    {
        my $magic ;
        my $got = $self->smartRead(\$magic, 4);

        return $self->saveErrorString(STATUS_ERROR, "Truncated file")
            if $got != 4 && *$self->{Strict};

        if ($got == 0) {
            return STATUS_EOF ;
        }
        elsif ($got < 0) {
            return STATUS_ERROR ;
        }
        elsif ($got < 4) {
            $self->pushBack($magic)  ;
            return STATUS_OK ;
        }

        my $sig = unpack("V", $magic) ;

        my $hdr;
        if ($hdr = $headerLookup{$sig})
        {
            if (&$hdr($self, $magic) != STATUS_OK ) {
                if (*$self->{Strict}) {
                    return STATUS_ERROR ;
                }
                else {
                    $self->clearError();
                    return STATUS_OK ;
                }
            }

            if ($sig == ZIP_END_CENTRAL_HDR_SIG)
            {
                return STATUS_OK ;
                last;
            }
        }
        elsif ($sig == ZIP_LOCAL_HDR_SIG)
        {
            $self->pushBack($magic)  ;
            return STATUS_OK ;
        }
        else
        {
            # put the data back
            $self->pushBack($magic)  ;
            last;
        }
    }

    return $reachedEnd ;
}

sub skipCentralDirectory
{
    my $self = shift;
    my $magic = shift ;

    my $buffer;
    $self->smartReadExact(\$buffer, 46 - 4)
        or return $self->TrailerError("Minimum header size is " . 
                                     46 . " bytes") ;

    my $keep = $magic . $buffer ;
    *$self->{HeaderPending} = $keep ;

   #my $versionMadeBy      = unpack ("v", substr($buffer, 4-4,  2));
   #my $extractVersion     = unpack ("v", substr($buffer, 6-4,  2));
   #my $gpFlag             = unpack ("v", substr($buffer, 8-4,  2));
   #my $compressedMethod   = unpack ("v", substr($buffer, 10-4, 2));
   #my $lastModTime        = unpack ("V", substr($buffer, 12-4, 4));
   #my $crc32              = unpack ("V", substr($buffer, 16-4, 4));
    my $compressedLength   = unpack ("V", substr($buffer, 20-4, 4));
    my $uncompressedLength = unpack ("V", substr($buffer, 24-4, 4));
    my $filename_length    = unpack ("v", substr($buffer, 28-4, 2)); 
    my $extra_length       = unpack ("v", substr($buffer, 30-4, 2));
    my $comment_length     = unpack ("v", substr($buffer, 32-4, 2));
   #my $disk_start         = unpack ("v", substr($buffer, 34-4, 2));
   #my $int_file_attrib    = unpack ("v", substr($buffer, 36-4, 2));
   #my $ext_file_attrib    = unpack ("V", substr($buffer, 38-4, 2));
   #my $lcl_hdr_offset     = unpack ("V", substr($buffer, 42-4, 2));

    
    my $filename;
    my $extraField;
    my $comment ;
    if ($filename_length)
    {
        $self->smartReadExact(\$filename, $filename_length)
            or return $self->TruncatedTrailer("filename");
        $keep .= $filename ;
    }

    if ($extra_length)
    {
        $self->smartReadExact(\$extraField, $extra_length)
            or return $self->TruncatedTrailer("extra");
        $keep .= $extraField ;
    }

    if ($comment_length)
    {
        $self->smartReadExact(\$comment, $comment_length)
            or return $self->TruncatedTrailer("comment");
        $keep .= $comment ;
    }

    return STATUS_OK ;
}

sub skipArchiveExtra
{
    my $self = shift;
    my $magic = shift ;

    my $buffer;
    $self->smartReadExact(\$buffer, 4)
        or return $self->TrailerError("Minimum header size is " . 
                                     4 . " bytes") ;

    my $keep = $magic . $buffer ;

    my $size = unpack ("V", $buffer);

    $self->smartReadExact(\$buffer, $size)
        or return $self->TrailerError("Minimum header size is " . 
                                     $size . " bytes") ;

    $keep .= $buffer ;
    *$self->{HeaderPending} = $keep ;

    return STATUS_OK ;
}


sub skipCentralDirectory64Rec
{
    my $self = shift;
    my $magic = shift ;

    my $buffer;
    $self->smartReadExact(\$buffer, 8)
        or return $self->TrailerError("Minimum header size is " . 
                                     8 . " bytes") ;

    my $keep = $magic . $buffer ;

    my ($sizeLo, $sizeHi)  = unpack ("V V", $buffer);

    # TODO - take SizeHi into account
    $self->smartReadExact(\$buffer, $sizeLo)
        or return $self->TrailerError("Minimum header size is " . 
                                     $sizeLo . " bytes") ;

    $keep .= $buffer ;
    *$self->{HeaderPending} = $keep ;

   #my $versionMadeBy      = unpack ("v",   substr($buffer,  0, 2));
   #my $extractVersion     = unpack ("v",   substr($buffer,  2, 2));
   #my $diskNumber         = unpack ("V",   substr($buffer,  4, 4));
   #my $cntrlDirDiskNo     = unpack ("V",   substr($buffer,  8, 4));
   #my $entriesInThisCD    = unpack ("V V", substr($buffer, 12, 8));
   #my $entriesInCD        = unpack ("V V", substr($buffer, 20, 8));
   #my $sizeOfCD           = unpack ("V V", substr($buffer, 28, 8));
   #my $offsetToCD         = unpack ("V V", substr($buffer, 36, 8));

    return STATUS_OK ;
}

sub skipCentralDirectory64Loc
{
    my $self = shift;
    my $magic = shift ;

    my $buffer;
    $self->smartReadExact(\$buffer, 20 - 4)
        or return $self->TrailerError("Minimum header size is " . 
                                     20 . " bytes") ;

    my $keep = $magic . $buffer ;
    *$self->{HeaderPending} = $keep ;

   #my $startCdDisk        = unpack ("V",   substr($buffer,  4-4, 4));
   #my $offsetToCD         = unpack ("V V", substr($buffer,  8-4, 8));
   #my $diskCount          = unpack ("V",   substr($buffer, 16-4, 4));

    return STATUS_OK ;
}

sub skipEndCentralDirectory
{
    my $self = shift;
    my $magic = shift ;

    my $buffer;
    $self->smartReadExact(\$buffer, 22 - 4)
        or return $self->TrailerError("Minimum header size is " . 
                                     22 . " bytes") ;

    my $keep = $magic . $buffer ;
    *$self->{HeaderPending} = $keep ;

   #my $diskNumber         = unpack ("v", substr($buffer, 4-4,  2));
   #my $cntrlDirDiskNo     = unpack ("v", substr($buffer, 6-4,  2));
   #my $entriesInThisCD    = unpack ("v", substr($buffer, 8-4,  2));
   #my $entriesInCD        = unpack ("v", substr($buffer, 10-4, 2));
   #my $sizeOfCD           = unpack ("V", substr($buffer, 12-4, 2));
   #my $offsetToCD         = unpack ("V", substr($buffer, 16-4, 2));
    my $comment_length     = unpack ("v", substr($buffer, 20-4, 2));

    
    my $comment ;
    if ($comment_length)
    {
        $self->smartReadExact(\$comment, $comment_length)
            or return $self->TruncatedTrailer("comment");
        $keep .= $comment ;
    }

    return STATUS_OK ;
}


sub _isZipMagic
{
    my $buffer = shift ;
    return 0 if length $buffer < 4 ;
    my $sig = unpack("V", $buffer) ;
    return $sig == ZIP_LOCAL_HDR_SIG ;
}


sub _readFullZipHeader($)
{
    my ($self) = @_ ;
    my $magic = '' ;

    $self->smartReadExact(\$magic, 4);

    *$self->{HeaderPending} = $magic ;

    return $self->HeaderError("Minimum header size is " . 
                              30 . " bytes") 
        if length $magic != 4 ;                                    


    return $self->HeaderError("Bad Magic")
        if ! _isZipMagic($magic) ;

    my $status = $self->_readZipHeader($magic);
    delete *$self->{Transparent} if ! defined $status ;
    return $status ;
}

sub _readZipHeader($)
{
    my ($self, $magic) = @_ ;
    my ($HeaderCRC) ;
    my ($buffer) = '' ;

    $self->smartReadExact(\$buffer, 30 - 4)
        or return $self->HeaderError("Minimum header size is " . 
                                     30 . " bytes") ;

    my $keep = $magic . $buffer ;
    *$self->{HeaderPending} = $keep ;

    my $extractVersion     = unpack ("v", substr($buffer, 4-4,  2));
    my $gpFlag             = unpack ("v", substr($buffer, 6-4,  2));
    my $compressedMethod   = unpack ("v", substr($buffer, 8-4,  2));
    my $lastModTime        = unpack ("V", substr($buffer, 10-4, 4));
    my $crc32              = unpack ("V", substr($buffer, 14-4, 4));
    my $compressedLength   = new U64 unpack ("V", substr($buffer, 18-4, 4));
    my $uncompressedLength = new U64 unpack ("V", substr($buffer, 22-4, 4));
    my $filename_length    = unpack ("v", substr($buffer, 26-4, 2)); 
    my $extra_length       = unpack ("v", substr($buffer, 28-4, 2));

    my $filename;
    my $extraField;
    my @EXTRA = ();
    my $streamingMode = ($gpFlag & ZIP_GP_FLAG_STREAMING_MASK) ? 1 : 0 ;

    return $self->HeaderError("Streamed Stored content not supported")
        if $streamingMode && $compressedMethod == 0 ;

    return $self->HeaderError("Encrypted content not supported")
        if $gpFlag & (ZIP_GP_FLAG_ENCRYPTED_MASK|ZIP_GP_FLAG_STRONG_ENCRYPTED_MASK);

    return $self->HeaderError("Patch content not supported")
        if $gpFlag & ZIP_GP_FLAG_PATCHED_MASK;

    *$self->{ZipData}{Streaming} = $streamingMode;


    if ($filename_length)
    {
        $self->smartReadExact(\$filename, $filename_length)
            or return $self->TruncatedHeader("Filename");
        $keep .= $filename ;
    }

    my $zip64 = 0 ;

    if ($extra_length)
    {
        $self->smartReadExact(\$extraField, $extra_length)
            or return $self->TruncatedHeader("Extra Field");

        my $bad = IO::Compress::Zlib::Extra::parseRawExtra($extraField,
                                                \@EXTRA, 1, 0);
        return $self->HeaderError($bad)
            if defined $bad;

        $keep .= $extraField ;

        my %Extra ;
        for (@EXTRA)
        {
            $Extra{$_->[0]} = \$_->[1];
        }
        
        if (defined $Extra{ZIP_EXTRA_ID_ZIP64()})
        {
            $zip64 = 1 ;

            my $buff = ${ $Extra{ZIP_EXTRA_ID_ZIP64()} };

            # TODO - This code assumes that all the fields in the Zip64
            # extra field aren't necessarily present. The spec says that
            # they only exist if the equivalent local headers are -1.
            # Need to check that info-zip fills out -1 in the local header
            # correctly.

            if (! $streamingMode) {
                my $offset = 0 ;

                $uncompressedLength = U64::newUnpack_V64 substr($buff,  0, 8)
                    if $uncompressedLength == 0xFFFF ;

                $offset += 8 ;

                $compressedLength = U64::newUnpack_V64 substr($buff, $offset, 8)
                    if $compressedLength == 0xFFFF ;

                $offset += 8 ;

                #my $cheaderOffset = U64::newUnpack_V64 substr($buff, 16, 8);
                #my $diskNumber = unpack ("V", substr($buff, 24, 4));
           }
        }
    }

    *$self->{ZipData}{Zip64} = $zip64;

    if (! $streamingMode) {
        *$self->{ZipData}{Streaming} = 0;
        *$self->{ZipData}{Crc32} = $crc32;
        *$self->{ZipData}{CompressedLen} = $compressedLength;
        *$self->{ZipData}{UnCompressedLen} = $uncompressedLength;
        *$self->{CompressedInputLengthRemaining} =
            *$self->{CompressedInputLength} = $compressedLength->get32bit();
    }

    *$self->{ZipData}{Method} = $compressedMethod;
    if ($compressedMethod == ZIP_CM_DEFLATE)
    {
        *$self->{Type} = 'zip-deflate';
    }
    elsif ($compressedMethod == ZIP_CM_BZIP2)
    {
    #if (! defined $IO::Uncompress::Adapter::Bunzip2::VERSION)
        
        *$self->{Type} = 'zip-bzip2';
        
        my $obj = IO::Uncompress::Adapter::Bunzip2::mkUncompObject(
                                                              );

        *$self->{Uncomp} = $obj;
        *$self->{ZipData}{CRC32} = crc32(undef);

    }
    elsif ($compressedMethod == ZIP_CM_STORE)
    {
        # TODO -- add support for reading uncompressed

        *$self->{Type} = 'zip-stored';
        
        my $obj = IO::Uncompress::Adapter::Identity::mkUncompObject(# $got->value('CRC32'),
                                                             # $got->value('ADLER32'),
                                                              );

        *$self->{Uncomp} = $obj;

    }
    else
    {
        return $self->HeaderError("Unsupported Compression format $compressedMethod");
    }

    return {
        'Type'               => 'zip',
        'FingerprintLength'  => 4,
        #'HeaderLength'       => $compressedMethod == 8 ? length $keep : 0,
        'HeaderLength'       => length $keep,
        'Zip64'              => $zip64,
        'TrailerLength'      => ! $streamingMode ? 0 : $zip64 ? 24 : 16,
        'Header'             => $keep,
        'CompressedLength'   => $compressedLength ,
        'UncompressedLength' => $uncompressedLength ,
        'CRC32'              => $crc32 ,
        'Name'               => $filename,
        'Time'               => _dosToUnixTime($lastModTime),
        'Stream'             => $streamingMode,

        'MethodID'           => $compressedMethod,
        'MethodName'         => $compressedMethod == ZIP_CM_DEFLATE 
                                 ? "Deflated" 
                                 : $compressedMethod == ZIP_CM_BZIP2
                                     ? "Bzip2"
                                     : $compressedMethod == ZIP_CM_STORE
                                         ? "Stored"
                                         : "Unknown" ,

        'ExtraFieldRaw' => $extraField,
        'ExtraField'    => [ @EXTRA ],


      }
}

sub filterUncompressed
{
    my $self = shift ;

    if (*$self->{ZipData}{Method} == 12) {
        *$self->{ZipData}{CRC32} = crc32(${$_[0]}, *$self->{ZipData}{CRC32});
    }
    else {
        *$self->{ZipData}{CRC32} = *$self->{Uncomp}->crc32() ;
    }
}    


sub _dosToUnixTime
{
    #use Time::Local 'timelocal_nocheck';
    use Time::Local 'timelocal';

	my $dt = shift;

	my $year = ( ( $dt >> 25 ) & 0x7f ) + 80;
	my $mon  = ( ( $dt >> 21 ) & 0x0f ) - 1;
	my $mday = ( ( $dt >> 16 ) & 0x1f );

	my $hour = ( ( $dt >> 11 ) & 0x1f );
	my $min  = ( ( $dt >> 5 ) & 0x3f );
	my $sec  = ( ( $dt << 1 ) & 0x3e );

	# catch errors
	my $time_t =
	  eval { timelocal( $sec, $min, $hour, $mday, $mon, $year ); };
	return 0 
        if $@;
	return $time_t;
}


1;

__END__


