package Net::SDP;



use strict;
use vars qw/$VERSION/;

use Net::SDP::Media;
use Net::SDP::Time;
use Sys::Hostname;
use Net::hostent;
use Carp;

$VERSION="0.07";



sub new {
    my $class = shift;
    my ($data) = @_;
    
    my $self = {'v'=>'0',
    			'session'=> {
     				'o_uname' => '',
    				'o_sess_id' => 0,
    				'o_sess_vers' => 0,
    				'o_net_type' => '',
    				'o_addr_type' => '',
    				'o_address' => '',
    				'p' => [],
    				'e' => [],
    				'a' => {}
   				},
   				'media'=>[],
   				'time'=>[]
   	};
    bless $self, $class;
   	
   	
   	# Parse data if we are passed some
   	if (defined $data) {
		unless ($self->parse( $data )) {
			# Failed to parse
			return undef;
		}
    } else {
   		# Use sane defaults
   		$self->{'session'}->{'o_uname'} = $ENV{'USER'} || '-';
   		$self->{'session'}->{'o_sess_id'} = Net::SDP::Time::_ntptime();
   		$self->{'session'}->{'o_sess_vers'} = Net::SDP::Time::_ntptime();
   		$self->{'session'}->{'o_net_type'} = 'IN';
   		$self->{'session'}->{'o_addr_type'} = 'IP4';
    	
   		my $hostname = hostname();
   		if (defined $hostname) {
   			if (my $h = gethost($hostname)) {
   				$self->{'session'}->{'o_address'} = $h->name();
   			}
   		}
   	}	

    return $self;
}

sub parse {
	my $self = shift;
	my ($source) = @_;

	if (@_ == 1) {

		if (ref $source eq 'Net::SAP::Packet') {
			# It is a SAP packet
			if ($source->payload_type() ne 'application/sdp') {
				carp "Payload type of Net::SAP::Packet is not application/sdp.";
				return 0;
			}
			return $self->parse_data( $source->payload() );

    	} elsif ($source =~ /^v=0/) {
    		# Looks like start of SDP file
    		return $self->parse_data( $source );
    		
    	} elsif ($source =~ /^\w+:/) {
    		# Looks like a URL
    		return $self->parse_url( $source );
    		
    	} elsif ($source eq '-') {
    		# Parse STDIN
			return $self->parse_stdin();
    		
    	} elsif ($source ne '') {
    		# Assume it is a filename
     		return $self->parse_file( $source );

    	} else {
    		carp "Failed to parse empty string.";
    		return 0;
    	}
    
	} elsif (@_ == 0) {
		return $self->parse_stdin();
		
	} else {
		croak "Too many parameters for parse()";
	}
	
}

sub parse_file {
	my $self = shift;
	my ($filename) = @_;

	open(SDP, $filename) or croak "can't open SDP file ($filename): $!";
	local $/ = undef;  # slurp full file
	my $data = <SDP>;
	close (SDP);
	
	return $self->parse_data( $data );
}

sub parse_url {
	my $self = shift;
	my ($url) = @_;

	eval "use LWP::Simple";
	croak "Couldn't fetch URL because LWP::Simple is unavailable." if ($@);

	my $data = LWP::Simple::get($url) or
	croak "Failed to fetch the URL '$url' with LWP: $!\n";

	return $self->parse_data( $data );
}

sub parse_stdin {
	my $self = shift;

	local $/ = undef;  # slurp STDIN
	my $data = <>;

	return $self->parse_data( $data );
}





sub parse_data {
	my $self = shift;
	my ($data) = @_;
	croak "Missing SDP data parameter.\n" unless (defined $data);
	
	# Undefine defaults
	undef $self->{'v'};
	undef $self->{'session'}->{'s'};
	undef $self->{'session'}->{'o_sess_id'};
	undef $self->{'session'}->{'o_sess_vers'};
	
	
	# Sections of sdp file: 'session', 'media'
	my $section = "session";

	
	# Split the file up into an array of its lines
	my @lines = split(/[\r\n]+/, $data);
	

	while (my $line = shift(@lines)) {
		my ($field, $value) = ($line =~ /^(\w)=(.*?)\s*$/);
		if ($field eq '') {
			carp "Failed to parse line of SDP data: $line\n";
			next;	
		}
		
		# Ignore empty values
		next if ($value eq '');
		
	
		## Session description
		if ($section eq 'session') {
		
			if ($field eq 'v') {

				$self->_parse_v( $value ) || return 0;

			} elsif ($field eq 'm') {
			
				# Move on to the media section
				$section = 'media';
				
			} elsif ($field eq 't') {
			
				my $time = new Net::SDP::Time( $value );
				
				push( @{$self->{'time'}}, $time );

			} elsif ($field eq 'r') {
				
				# Add to last time descriptor
				unless ( $self->{'time'}->[-1] ) {
				  carp "No previous 't' parameter to associate 'r' with: $line\n";
				  next;
				}

				$self->{'time'}->[-1]->_parse_r($value);

			} elsif ($field eq 'o') {

				$self->_parse_o( $value );

			} elsif ($field eq 'p' || $field eq 'e') {
				
				# Phone and email can have more than one value
				push( @{$self->{'session'}->{$field}}, $value );

			} elsif ($field eq 'a' || $field eq 'b') {
			
				# More than one value is allowed
				_add_attribute( $self->{'session'}, $field, $value );
				
			} else {
	
				# Single value
				$self->{'session'}->{$field} = $value;
			}
		}


		## Media description
		if ($section eq 'media') {
		
			if ($field eq 'm') {
				my $media = new Net::SDP::Media( $value );
				
				# Copy accross connection information for easier access
				if (defined $self->{'session'}->{'c'}) {
					$media->_parse_c( $self->{'session'}->{'c'} );
				}
				push( @{$self->{'media'}}, $media );
				
			} elsif ($field =~ /a|b/) {

				# XXXXXX Check array exists? XXXXXX
				_add_attribute( $self->{'media'}->[-1], $field, $value );
				
			} elsif ($field =~ /c/) {

				my $media = $self->{'media'}->[-1];
				$media->_parse_c( $value );
				
			} else {
				$self->{'media'}->[-1]->{$field} = $value;
			}
		
		}
		
	}


	# Ensure we have the required elements
	$self->_validate_self();


	# Success
	return 1;	
}


sub _validate_self {
	my $self = shift;
	my $session = $self->{'session'};

	# The following elements are required
	if (!defined $self->{'v'}) {
		carp "Invalid SDP file: Missing version field";
		return 1;
	}
	if (!defined $session->{'o_sess_id'}) {
		carp "Invalid SDP file: Missing origin session ID field";
		return 1;
	}
	if (!defined $session->{'o_sess_vers'}) {
		carp "Invalid SDP file: Missing origin version field";
		return 1;
	}
	if (!defined $session->{'s'}) {
		carp "Invalid SDP file: Missing session name field";
		return 1;
	}
		
	
	# We should have a Time Description...
	if (!exists $self->{'time'}->[0]) {
		carp "Invalid SDP file: Session is missing required time discription";
		
		# Make it valid :-/
		$self->{'time'}->[0] = new Net::SDP::Time();
	}
	
	# Everything is ok :)
	return 0;
}

sub generate {
	my $self=shift;
	my $session = $self->{'session'};
	my $sdp = '';

	# The order of the fields must be as stated in the RFC
	$sdp .= $self->_generate_v();
	$sdp .= $self->_generate_o();
	$sdp .= _generate_lines($session, 's', 0 );
	$sdp .= _generate_lines($session, 'i', 1 );
	$sdp .= _generate_lines($session, 'u', 1 );
	$sdp .= _generate_lines($session, 'e', 1 );
	$sdp .= _generate_lines($session, 'p', 1 );
	#c=	- I don't like having c lines here !
	#	The module will put c= lines in the media description
	$sdp .= _generate_lines($session, 'b', 1 );


	# Time Descriptions
	if (scalar(@{$self->{'time'}})==0) {
		# At least one is required
		warn "Missing Time description";
		return undef;
	}
	foreach my $time ( @{$self->{'time'}} ) {
		$sdp .= $time->_generate_t();
		#$sdp .= _generate_lines($time, 'z', 1 );
		$sdp .= $time->_generate_r();
	}

	$sdp .= _generate_lines($session, 'k', 1 );
	$sdp .= _generate_lines($session, 'a', 1 );


	# Media Descriptions
	foreach my $media ( @{$self->{'media'}} ) {
		$sdp .= $media->_generate_m();
		$sdp .= _generate_lines($media, 'i', 1 );
		# 'c' is non-optional because we dont have one 
		# in the session description
		$sdp .= $media->_generate_c();
		$sdp .= _generate_lines($media, 'b', 1 );
		$sdp .= _generate_lines($media, 'k', 1 );
		$sdp .= _generate_lines($media, 'a', 1 );
	}

	# Return the SDP description we just generated
	return $sdp;
}

sub _generate_lines {
	my ($hashref, $field, $optional) = @_;
	my $lines = '';

	if (exists $hashref->{$field} and
	    defined $hashref->{$field}) {
		if (ref $hashref->{$field} eq 'ARRAY') {
			foreach( @{$hashref->{$field}} ) {
				$lines .= "$field=$_\n";
			}
		} elsif (ref $hashref->{$field} eq 'HASH') {
			foreach my $att_field ( sort keys %{$hashref->{$field}} ) {
				my $attrib = $hashref->{$field}->{$att_field};
				if (ref $attrib eq 'ARRAY') {
					foreach my $att_value (@{$attrib}) {
						$lines .= "$field=$att_field:$att_value\n";
					}
				} else {
					$lines .= "$field=$att_field\n";
				}
			}
		} else {
			$lines = $field.'='.$hashref->{$field}."\n";
		}
	} else {
		if (!$optional) {
			warn "Non-optional field '$field' missing";
		}
	}
	
	return $lines;
}


sub _parse_o {
	my $self = shift;
	my $session = $self->{'session'};
	my ($o) = @_;

	($session->{'o_uname'},
	 $session->{'o_sess_id'},
	 $session->{'o_sess_vers'},
	 $session->{'o_net_type'},
	 $session->{'o_addr_type'},
	 $session->{'o_address'}) = split(/\s/, $o);
		
	# Success
	return 1;
}


sub _generate_o {
	my $self = shift;
	return "o=".$self->session_origin()."\n";
}


sub _parse_v {
	my $self = shift;
	$self->{'v'} = shift;
	
	# Check the version number
	if ($self->{'v'} ne '0') {
		carp "Unsupported SDP format version number: ".$self->{'v'};
		return 0;
	}
	
	# Success
	return 1;
}


sub _generate_v {
	my $self = shift;
	return "v=0\n";
}

sub _add_attribute {
	my ($hashref, $field, $value) = @_;
	
	if (!defined $hashref->{$field}) {
		$hashref->{$field} = {};
	}
	
	if ( my($att_field, $att_value) = ($value =~ /^([\w\-\_]+):(.*)$/) ) {
		my $fieldref = $hashref->{$field};
		
		if (!defined $fieldref->{$att_field}) {
			$fieldref->{$att_field} = [];
		}
		
		push( @{$fieldref->{$att_field}}, $att_value );
	
	} else {
		$hashref->{$field}->{$value} = '';
	}
}

sub session_origin {
    my $self=shift;
	my $session = $self->{'session'};
	my ($o) = @_;
	
	$self->_parse_o( $o ) if (defined $o);

	return  $session->{'o_uname'} .' '.
			$session->{'o_sess_id'} .' '.
			$session->{'o_sess_vers'} .' '.
			$session->{'o_net_type'} .' '.
			$session->{'o_addr_type'} .' '.
			$session->{'o_address'};
}

sub session_origin_username {
    my $self=shift;
	my ($uname) = @_;
	$self->{'session'}->{'o_uname'} = $uname if (defined $uname);
	return $self->{'session'}->{'o_uname'};
}

sub session_origin_id {
    my $self=shift;
	my ($id) = @_;
	$self->{'session'}->{'o_sess_id'} = $id if (defined $id);
	return $self->{'session'}->{'o_sess_id'};
}

sub session_origin_version {
	my $self=shift;
	my ($vers) = @_;
    $self->{'session'}->{'o_sess_vers'} = $vers if defined $vers;
	return $self->{'session'}->{'o_sess_vers'};
}

sub session_origin_net_type {
	my $self=shift;
	my ($net_type) = @_;
    $self->{'session'}->{'o_net_type'} = $net_type if defined $net_type;
	return $self->{'session'}->{'o_net_type'};
}

sub session_origin_addr_type {
	my $self=shift;
	my ($addr_type) = @_;
    $self->{'session'}->{'o_addr_type'} = $addr_type if defined $addr_type;
	return $self->{'session'}->{'o_addr_type'};
}

sub session_origin_address {
	my $self=shift;
	my ($addr) = @_;
    $self->{'session'}->{'o_address'} = $addr if defined $addr;
	return $self->{'session'}->{'o_address'};
}



sub session_identifier {
	my $self=shift;
	my $session = $self->{'session'};

	return	$session->{'o_uname'} . 
			sprintf("%x",$session->{'o_sess_id'}) .
			$session->{'o_net_type'} .
			$session->{'o_addr_type'} .
			$session->{'o_address'};
}


sub session_name {
    my $self=shift;
	my ($s) = @_;
    $self->{'session'}->{'s'} = $s if defined $s;
    return $self->{'session'}->{'s'};
}

sub session_info {
    my $self=shift;
	my ($i) = @_;
    $self->{'session'}->{'i'} = $i if defined $i;
    return $self->{'session'}->{'i'};
}

sub session_uri {
    my $self=shift;
	my ($u) = @_;
    $self->{'session'}->{'u'} = $u if defined $u;
    return $self->{'session'}->{'u'};
}

sub session_email {
    my $self=shift;
	my ($e) = @_;
    my $session = $self->{'session'};
    
	# An ARRAYREF may be passed to set more than one email address
	if (defined $e) {
		if (ref $e eq 'ARRAY') {
			$session->{'e'} = $e;
		} else {
			$session->{'e'} = [ $e ];
		}
	}

    # Multiple emails are allowed, but we just return the first
    if (exists $session->{'e'}->[0]) {
    	return $session->{'e'}->[0];
    }
    return undef;
}

sub session_email_arrayref {
    my $self=shift;
    my $session = $self->{'session'};
    
    if (defined $session->{'e'}) {
        return $session->{'e'};
    }
    return undef;
}

sub session_phone {
    my $self=shift;
	my ($p) = @_;
    my $session = $self->{'session'};
    
	# An ARRAYREF may be passed to set more than one phone number
	if (defined $p) {
		if (ref $p eq 'ARRAY') {
			$session->{'p'} = $p;
		} else {
			$session->{'p'} = [ $p ];
		}
	}

    # Multiple phone numbers are allowed, but we just return the first
    if (exists $session->{'p'}->[0]) {
    	return $session->{'p'}->[0];
    }
    return undef;
}

sub session_phone_arrayref {
    my $self=shift;
    my $session = $self->{'session'};
    
    if (defined $session->{'p'}) {
        return $session->{'p'};
    }
    return undef;
}

sub session_key {
    my $self=shift;
	my ($method, $key) = @_;
	
    $self->{'session'}->{'k'} = $method if defined $method;
    $self->{'session'}->{'k'} .= ":$key" if defined $key;
	
    return ($self->{'session'}->{'k'} =~ /^([\w-]+):?(.*)$/);
}



sub _attribute {
	my ($hashref, $attr_name, $attr_value) = @_;
	carp "Missing attribute name" unless (defined $attr_name);
	
	# Set attribute to value, if value supplied
	# Warning - all other values are lost
	if (defined $attr_value) {
		if (ref $attr_value eq 'ARRAY') {
			$hashref->{'a'}->{$attr_name} = $attr_value;
		} else {
			$hashref->{'a'}->{$attr_name} = [ $attr_value ];
		}
	}
	
	# Return undef if attribute doesn't exist
	if (!exists $hashref->{'a'}->{$attr_name}) {
		return undef;
	}
	
	# Return 1 if attribute exists but has no value
	# Return value if attribute has single value
	# Return arrayref if attribute has more than one value
	my $attrib = $hashref->{'a'}->{$attr_name};
	if (ref $attrib eq 'ARRAY') {
		if (scalar(@{ $attrib }) == 1) {
			return $attrib->[0];
		} else {
			return $attrib;
		}
	} else {
		return '';
	}
}

sub session_attribute {
    my $self=shift;

    return Net::SDP::_attribute( $self->{'session'}, @_);
}

sub session_attributes {
    my $self=shift;

    return $self->{'session'}->{'a'};
}

sub session_add_attribute {
	my $self = shift;
	my ($name, $value) = @_;
	carp "Missing attribute name" unless (defined $name);
	
	my $attrib = $name;
	$attrib .= ":$value" if (defined $value);
	Net::SDP::_add_attribute( $self->{'session'}, 'a', $attrib );
}

sub session_del_attribute {
	my $self = shift;
	my ($name) = @_;
	carp "Missing attribute name" unless (defined $name);

	if ( exists $self->{'session'}->{'a'}->{$name} ) {
		delete $self->{'session'}->{'a'}->{$name};
	}
}





sub media_desc_of_type {
	my $self = shift;
	my ($type) = @_;
	carp "Missing media type parameter" unless (defined $type);
	
    foreach my $media ( @{$self->{'media'}} ) {
    	return $media if ($media->media_type() eq $type);
	}
	
	return undef;
}


sub media_desc_arrayref {
	my ($self) = @_;
	
	return $self->{'media'};
}

sub media_desc_delete_all {
	my ($self) = @_;

	$self->{'media'} = [ ];
	
	return 0;
}

sub media_desc_delete {
	my $self = shift;
             my ($num) = @_;
	
	return 1 if ( !defined($num) || !defined($self->{'media'}->[$num]) );

	my $results = [ ];
	for my $loop ( 0...(scalar(@{$self->{'media'}}) - 1) ) {
		next if ( $loop == $num );
		
		push @$results, $self->{'media'}->[$loop];
	}
	$self->{'media'} = $results;

	return 0;
}

sub time_desc {
	my $self = shift;
	my ($num) = @_;
	
	$num = 0 unless ( defined $num );
	return undef unless ( defined($self->{'time'}->[$num]) );

	## Ensure that one exists ?
	return $self->{'time'}->[$num];
}

sub time_desc_arrayref {
	my ($self) = @_;

	return $self->{'time'};
}

sub time_desc_delete_all {
	my ($self) = @_;

	$self->{'time'} = [ ];
	
	return 0;
}

sub time_desc_delete {
	my $self = shift;
	my ($num) = @_;
	
	return 1 unless ( defined $num );
	return 1 unless ( defined $self->{'time'}->[$num] );

	my $results = [ ];
	for my $loop ( 0...(scalar(@{$self->{'time'}}) - 1) ) {
		next if ( $loop == $num );
		
		push @$results, $self->{'time'}->[$loop];
	}
	$self->{'time'} = $results;

	return 0;
}


sub new_time_desc {
	my $self = shift;
	
	my $time = new Net::SDP::Time();
	push( @{$self->{'time'}}, $time );

	return $time;
}


sub new_media_desc {
	my $self = shift;
	my ($media_type) = @_;
	
	my $media = new Net::SDP::Media();
	$media->media_type( $media_type ) if (defined $media_type);
	push( @{$self->{'media'}}, $media );

	return $media;
}



sub DESTROY {
    my $self=shift;
    
}


1;

__END__

