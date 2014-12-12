package Net::SDP::Media;


use strict;
use vars qw/$VERSION %avt_profile_map/;
use Carp;
$VERSION="0.07";




%avt_profile_map = (
	'0' => 'audio/PCMU/8000/1',
	'3' => 'audio/GSM/8000/1',
	'4' => 'audio/G723/8000/1',
	'5' => 'audio/DVI4/8000/1',
	'6' => 'audio/DVI4/16000/1',
	'7' => 'audio/LPC/8000/1',
	'8' => 'audio/PCMA/8000/1',
	'9' => 'audio/G722/8000/1',
	'10' => 'audio/L16/44100/2',
	'11' => 'audio/L16/44100/1',
	'12' => 'audio/QCELP/8000/1',
	'13' => 'audio/CN/8000/1',
	'14' => 'audio/MPA/90000',
	'15' => 'audio/G728/8000/1',
	'16' => 'audio/DVI4/11025/1',
	'17' => 'audio/DVI4/22050/1',
	'18' => 'audio/G729/8000/1',
	'25' => 'video/CelB/90000',
	'26' => 'video/JPEG/90000',
	'28' => 'video/nv/90000',
	'31' => 'video/H261/90000',
	'32' => 'video/MPV/90000',
	'33' => 'video/MP2T/90000',
	'34' => 'video/H263/90000',
);





sub new {
	my $class = shift;
	my $self = {
		'm_media' => 'unknown',
		'm_port' => 0,
		'm_transport' => 'RTP/AVP',
		'm_fmt_list' => [],
		'i' => undef,
		'c_net_type' => 'IN',
		'c_addr_type' => 'IP4',
		'c_address' => '0.0.0.0',
		'c_ttl' => 5,
		'a' => {}
	};
	bless $self, $class;	

	# Initial value provided ?
	my ($m) = @_;
	$self->_parse_m($m) if (defined $m);
	
	return $self;
}




sub _parse_m {
	my $self = shift;
	my ($m) = @_;
	
	($self->{'m_media'},
	 my $port,
	 $self->{'m_transport'},
	 my @formats ) = split(/\s/, $m);

	$self->{'m_fmt_list'} = \@formats;
	
	if (defined $port) {
		($self->{'m_port'}, my $range) = split(/\//, $port);
		if (defined $range and $range ne '' and $range ne '1') {
			carp "Port ranges are not supported by Net::SDP.";
		}
	}
	
	# Success
	return 1;
}

sub _generate_m {
	my $self = shift;

	return	'm='.$self->{'m_media'}.' '.
			$self->{'m_port'}.' '.
			$self->{'m_transport'}.' '.
			join(' ', @{$self->{'m_fmt_list'}})."\n";
}

sub _parse_c {
	my $self = shift;
	my ($c) = @_;
	
	($self->{'c_net_type'}, $self->{'c_addr_type'}, my $address) = split(/\s/, $c);
	($self->{'c_address'}, $self->{'c_ttl'}, my $range) = split(/\//, $address);
	
	if ($self->{'c_net_type'} ne 'IN') {
		carp "Network type is not Internet (IN): ".$self->{'c_net_type'};
	}
	
	if ($self->{'c_addr_type'} ne 'IP4' and $self->{'c_addr_type'} ne 'IP6') {
		carp "Address type is not IP4 or IP6: ".$self->{'c_addr_type'};
	}
	
	if (!defined $self->{'c_ttl'}) {
		$self->{'c_ttl'} = 0;
	}
	
	if (defined $range and $range ne '' and $range ne '1') {
		carp "Address ranges are not supported by Net::SDP.";
	}
	
	# Success
	return 1;
}

sub _generate_c {
	my $self = shift;
	my $c = 'c='.$self->{'c_net_type'}.' '.
			$self->{'c_addr_type'}.' '.
			$self->{'c_address'};
			
	if ($self->{'c_ttl'}) {
		$c .= '/'.$self->{'c_ttl'};
	}

	return "$c\n";
}





sub address {
    my $self=shift;
	my ($address) = @_;
    $self->{'c_address'} = $address if defined $address;
    return $self->{'c_address'};
}

sub address_type {
    my $self=shift;
	my ($addr_type) = @_;
    $self->{'c_addr_type'} = $addr_type if defined $addr_type;
    return $self->{'c_addr_type'};
}


sub port {
    my $self=shift;
	my ($port) = @_;
    $self->{'m_port'} = $port if defined $port;
    return $self->{'m_port'};
}

sub ttl {
    my $self=shift;
	my ($ttl) = @_;
    $self->{'c_ttl'} = $ttl if defined $ttl;
    return $self->{'c_ttl'};
}

sub media_type {
    my $self=shift;
	my ($media) = @_;
    $self->{'m_media'} = $media if defined $media;
    return $self->{'m_media'};
}

sub title {
    my $self=shift;
	my ($title) = @_;
    $self->{'i'} = $title if defined $title;
    return $self->{'i'};
}

sub transport {
    my $self=shift;
	my ($transport) = @_;
    $self->{'m_transport'} = $transport if defined $transport;
    return $self->{'m_transport'};
}

sub network_type {
    my $self=shift;
	my ($net_type) = @_;
    $self->{'c_net_type'} = $net_type if defined $net_type;
    return $self->{'c_net_type'};
}


sub attribute {
    my $self=shift;

    return Net::SDP::_attribute( $self, @_);
}

sub attributes {
    my $self=shift;

    return $self->{'a'};
}

sub add_attribute {
	my $self = shift;
	my ($name, $value) = @_;
	carp "Missing attribute name" unless (defined $name);
	
	my $attrib = $name;
	$attrib .= ":$value" if (defined $value);
	Net::SDP::_add_attribute( $self, 'a', $attrib );
}



sub remove_format_num {
    my $self=shift;
	my ($fmt_num) = @_;
	carp "Missing format number to remove" unless (defined $fmt_num);
	
	foreach my $n ( 0 .. $#{$self->{'m_fmt_list'}}) {
		if ($self->{'m_fmt_list'}->[$n] == $fmt_num) {
			splice( @{$self->{'m_fmt_list'}}, $n, 1 );
			return 1;
		}
	}

	# Failed to delete
	return 0;
}


sub default_format_num {
	my $self=shift;
	my ($fmt_num) = @_;

	if (defined $fmt_num) {

		# Remove it from elsewhere in the list
		$self->remove_format_num( $fmt_num );

		# Put it at the start of the format list
		unshift( @{$self->{'m_fmt_list'}}, $fmt_num );
	}

	return $self->{'m_fmt_list'}->[0];
}

sub default_format {
    my $self=shift;
    my $fmt_list = $self->format_list();
    
	return $fmt_list->{ $self->default_format_num() };
}


sub format_num_list {
    my $self=shift;
	my ($fmt_list) = @_;

	if (defined $fmt_list) {
		carp "Parameter should be an array ref" if (ref $fmt_list ne 'ARRAY');
    	$self->{'m_fmt_list'} = $fmt_list;
    }
    
    return $self->{'m_fmt_list'};
}

sub format_list {
    my $self=shift;
	my $fmt_list = {};

	# Build a list of formats from rtpmap attributes
	my %rtpmap = ();
    foreach( @{$self->{'a'}->{'rtpmap'}} ) {
    	/(\d+)\s(.*)$/;
    	$rtpmap{$1} = $self->{'m_media'}."/$2";
    }

	# Build our payload type map
	foreach (@{$self->{'m_fmt_list'}}) {
		if (exists $rtpmap{$_}) {
			$fmt_list->{$_} = $rtpmap{$_};
		} elsif (exists $avt_profile_map{$_}) {
			$fmt_list->{$_} = $avt_profile_map{$_};
		} else {
			$fmt_list->{$_} = '';
		}
	}

	return $fmt_list;	
}


sub add_format {
    my $self=shift;
	my ($format_num, $mime) = @_;
	carp "Missing format number to add" unless (defined $format_num);
	
	
	# Appened the format number to the list
	# (which means the first one you add is default)
	push( @{$self->{'m_fmt_list'}}, $format_num );


	# Mime type specified ?
	if (!defined $mime) {
		$mime = $avt_profile_map{$format_num};
	}
	
	if (!defined $mime) {
		warn "Payload format $format_num is unknown dynamic type.";
		return;
	}
	
	# Work out rtpmap entry
	my ($mime_media, $mime_format) = ($mime =~ /^([^\/]+)\/(.+)$/);
	if ($mime_media ne $self->{'m_media'}) {
		warn "This Media Description is for ".$self->{'m_media'};
	}
	
	# Appened the rtpmap entry for this format
	push( @{$self->{'a'}->{'rtpmap'}}, "$format_num $mime_format" );
}

sub as_string {
	my $self=shift;
	my $type = $self->{'m_media'};
	$type =~ s/^(.+)/\u\L$1/;
	return "$type Stream";
}

1;

__END__

