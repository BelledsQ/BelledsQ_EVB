package HTTP::Headers;

use strict;
use Carp ();

use vars qw($VERSION $TRANSLATE_UNDERSCORE);
$VERSION = "5.810";

$TRANSLATE_UNDERSCORE = 1 unless defined $TRANSLATE_UNDERSCORE;


my @general_headers = qw(
   Cache-Control Connection Date Pragma Trailer Transfer-Encoding Upgrade
   Via Warning
);

my @request_headers = qw(
   Accept Accept-Charset Accept-Encoding Accept-Language
   Authorization Expect From Host
   If-Match If-Modified-Since If-None-Match If-Range If-Unmodified-Since
   Max-Forwards Proxy-Authorization Range Referer TE User-Agent
);

my @response_headers = qw(
   Accept-Ranges Age ETag Location Proxy-Authenticate Retry-After Server
   Vary WWW-Authenticate
);

my @entity_headers = qw(
   Allow Content-Encoding Content-Language Content-Length Content-Location
   Content-MD5 Content-Range Content-Type Expires Last-Modified
);

my %entity_header = map { lc($_) => 1 } @entity_headers;

my @header_order = (
   @general_headers,
   @request_headers,
   @response_headers,
   @entity_headers,
);

my %header_order;
my %standard_case;

{
    my $i = 0;
    for (@header_order) {
	my $lc = lc $_;
	$header_order{$lc} = ++$i;
	$standard_case{$lc} = $_;
    }
}



sub new
{
    my($class) = shift;
    my $self = bless {}, $class;
    $self->header(@_) if @_; # set up initial headers
    $self;
}


sub header
{
    my $self = shift;
    Carp::croak('Usage: $h->header($field, ...)') unless @_;
    my(@old);
    my %seen;
    while (@_) {
	my $field = shift;
        my $op = @_ ? ($seen{lc($field)}++ ? 'PUSH' : 'SET') : 'GET';
	@old = $self->_header($field, shift, $op);
    }
    return @old if wantarray;
    return $old[0] if @old <= 1;
    join(", ", @old);
}

sub clear
{
    my $self = shift;
    %$self = ();
}


sub push_header
{
    Carp::croak('Usage: $h->push_header($field, $val)') if @_ != 3;
    shift->_header(@_, 'PUSH');
}


sub init_header
{
    Carp::croak('Usage: $h->init_header($field, $val)') if @_ != 3;
    shift->_header(@_, 'INIT');
}


sub remove_header
{
    my($self, @fields) = @_;
    my $field;
    my @values;
    foreach $field (@fields) {
	$field =~ tr/_/-/ if $field !~ /^:/ && $TRANSLATE_UNDERSCORE;
	my $v = delete $self->{lc $field};
	push(@values, ref($v) eq 'ARRAY' ? @$v : $v) if defined $v;
    }
    return @values;
}

sub remove_content_headers
{
    my $self = shift;
    unless (defined(wantarray)) {
	# fast branch that does not create return object
	delete @$self{grep $entity_header{$_} || /^content-/, keys %$self};
	return;
    }

    my $c = ref($self)->new;
    for my $f (grep $entity_header{$_} || /^content-/, keys %$self) {
	$c->{$f} = delete $self->{$f};
    }
    $c;
}


sub _header
{
    my($self, $field, $val, $op) = @_;

    # $push is only used interally sub push_header
    Carp::croak('Need a field name') unless length($field);

    unless ($field =~ /^:/) {
	$field =~ tr/_/-/ if $TRANSLATE_UNDERSCORE;
	my $old = $field;
	$field = lc $field;
	unless(defined $standard_case{$field}) {
	    # generate a %standard_case entry for this field
	    $old =~ s/\b(\w)/\u$1/g;
	    $standard_case{$field} = $old;
	}
    }

    my $h = $self->{$field};
    my @old = ref($h) eq 'ARRAY' ? @$h : (defined($h) ? ($h) : ());

    $op ||= defined($val) ? 'SET' : 'GET';
    unless ($op eq 'GET' || ($op eq 'INIT' && @old)) {
	if (defined($val)) {
	    my @new = ($op eq 'PUSH') ? @old : ();
	    if (ref($val) ne 'ARRAY') {
		push(@new, $val);
	    }
	    else {
		push(@new, @$val);
	    }
	    $self->{$field} = @new > 1 ? \@new : $new[0];
	}
	elsif ($op ne 'PUSH') {
	    delete $self->{$field};
	}
    }
    @old;
}


sub _sorted_field_names
{
    my $self = shift;
    return sort {
        ($header_order{$a} || 999) <=> ($header_order{$b} || 999) ||
         $a cmp $b
    } keys %$self
}


sub header_field_names {
    my $self = shift;
    return map $standard_case{$_} || $_, $self->_sorted_field_names
	if wantarray;
    return keys %$self;
}


sub scan
{
    my($self, $sub) = @_;
    my $key;
    foreach $key ($self->_sorted_field_names) {
        next if $key =~ /^_/;
	my $vals = $self->{$key};
	if (ref($vals) eq 'ARRAY') {
	    my $val;
	    for $val (@$vals) {
		&$sub($standard_case{$key} || $key, $val);
	    }
	}
	else {
	    &$sub($standard_case{$key} || $key, $vals);
	}
    }
}


sub as_string
{
    my($self, $endl) = @_;
    $endl = "\n" unless defined $endl;

    my @result = ();
    $self->scan(sub {
	my($field, $val) = @_;
	$field =~ s/^://;
	if ($val =~ /\n/) {
	    # must handle header values with embedded newlines with care
	    $val =~ s/\s+$//;          # trailing newlines and space must go
	    $val =~ s/\n\n+/\n/g;      # no empty lines
	    $val =~ s/\n([^\040\t])/\n $1/g;  # intial space for continuation
	    $val =~ s/\n/$endl/g;      # substitute with requested line ending
	}
	push(@result, "$field: $val");
    });

    join($endl, @result, '');
}


sub clone
{
    my $self = shift;
    my $clone = new HTTP::Headers;
    $self->scan(sub { $clone->push_header(@_);} );
    $clone;
}


sub _date_header
{
    require HTTP::Date;
    my($self, $header, $time) = @_;
    my($old) = $self->_header($header);
    if (defined $time) {
	$self->_header($header, HTTP::Date::time2str($time));
    }
    $old =~ s/;.*// if defined($old);
    HTTP::Date::str2time($old);
}


sub date                { shift->_date_header('Date',                @_); }
sub expires             { shift->_date_header('Expires',             @_); }
sub if_modified_since   { shift->_date_header('If-Modified-Since',   @_); }
sub if_unmodified_since { shift->_date_header('If-Unmodified-Since', @_); }
sub last_modified       { shift->_date_header('Last-Modified',       @_); }

sub client_date         { shift->_date_header('Client-Date',         @_); }


sub content_type      {
  my $ct = (shift->_header('Content-Type', @_))[0];
  return '' unless defined($ct) && length($ct);
  my @ct = split(/;\s*/, $ct, 2);
  for ($ct[0]) {
      s/\s+//g;
      $_ = lc($_);
  }
  wantarray ? @ct : $ct[0];
}

sub _is_html          {
    my $self = shift;
    return $self->content_type eq 'text/html' || $self->_is_xhtml;
}

sub _is_xhtml         {
    my $ct = shift->content_type;
    for (qw(application/xhtml+xml application/vnd.wap.xhtml+xml)) {
        return 1 if $_ eq $ct;
    }
    return 0;
}

sub referer           {
    my $self = shift;
    if (@_ && $_[0] =~ /#/) {
	# Strip fragment per RFC 2616, section 14.36.
	my $uri = shift;
	if (ref($uri)) {
	    $uri = $uri->clone;
	    $uri->fragment(undef);
	}
	else {
	    $uri =~ s/\#.*//;
	}
	unshift @_, $uri;
    }
    ($self->_header('Referer', @_))[0];
}
*referrer = \&referer;  # on tchrist's request

sub title             { (shift->_header('Title',            @_))[0] }
sub content_encoding  { (shift->_header('Content-Encoding', @_))[0] }
sub content_language  { (shift->_header('Content-Language', @_))[0] }
sub content_length    { (shift->_header('Content-Length',   @_))[0] }

sub user_agent        { (shift->_header('User-Agent',       @_))[0] }
sub server            { (shift->_header('Server',           @_))[0] }

sub from              { (shift->_header('From',             @_))[0] }
sub warning           { (shift->_header('Warning',          @_))[0] }

sub www_authenticate  { (shift->_header('WWW-Authenticate', @_))[0] }
sub authorization     { (shift->_header('Authorization',    @_))[0] }

sub proxy_authenticate  { (shift->_header('Proxy-Authenticate',  @_))[0] }
sub proxy_authorization { (shift->_header('Proxy-Authorization', @_))[0] }

sub authorization_basic       { shift->_basic_auth("Authorization",       @_) }
sub proxy_authorization_basic { shift->_basic_auth("Proxy-Authorization", @_) }

sub _basic_auth {
    require MIME::Base64;
    my($self, $h, $user, $passwd) = @_;
    my($old) = $self->_header($h);
    if (defined $user) {
	Carp::croak("Basic authorization user name can't contain ':'")
	  if $user =~ /:/;
	$passwd = '' unless defined $passwd;
	$self->_header($h => 'Basic ' .
                             MIME::Base64::encode("$user:$passwd", ''));
    }
    if (defined $old && $old =~ s/^\s*Basic\s+//) {
	my $val = MIME::Base64::decode($old);
	return $val unless wantarray;
	return split(/:/, $val, 2);
    }
    return;
}


1;

__END__

