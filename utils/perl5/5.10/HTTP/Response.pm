package HTTP::Response;

require HTTP::Message;
@ISA = qw(HTTP::Message);
$VERSION = "5.811";

use strict;
use HTTP::Status ();



sub new
{
    my($class, $rc, $msg, $header, $content) = @_;
    my $self = $class->SUPER::new($header, $content);
    $self->code($rc);
    $self->message($msg);
    $self;
}


sub parse
{
    my($class, $str) = @_;
    my $status_line;
    if ($str =~ s/^(.*)\n//) {
	$status_line = $1;
    }
    else {
	$status_line = $str;
	$str = "";
    }

    my $self = $class->SUPER::parse($str);
    my($protocol, $code, $message);
    if ($status_line =~ /^\d{3} /) {
       # Looks like a response created by HTTP::Response->new
       ($code, $message) = split(' ', $status_line, 2);
    } else {
       ($protocol, $code, $message) = split(' ', $status_line, 3);
    }
    $self->protocol($protocol) if $protocol;
    $self->code($code) if defined($code);
    $self->message($message) if defined($message);
    $self;
}


sub clone
{
    my $self = shift;
    my $clone = bless $self->SUPER::clone, ref($self);
    $clone->code($self->code);
    $clone->message($self->message);
    $clone->request($self->request->clone) if $self->request;
    # we don't clone previous
    $clone;
}


sub code      { shift->_elem('_rc',      @_); }
sub message   { shift->_elem('_msg',     @_); }
sub previous  { shift->_elem('_previous',@_); }
sub request   { shift->_elem('_request', @_); }


sub status_line
{
    my $self = shift;
    my $code = $self->{'_rc'}  || "000";
    my $mess = $self->{'_msg'} || HTTP::Status::status_message($code) || "Unknown code";
    return "$code $mess";
}


sub base
{
    my $self = shift;
    my $base = $self->header('Content-Base')     ||  # used to be HTTP/1.1
               $self->header('Content-Location') ||  # HTTP/1.1
               $self->header('Base');                # HTTP/1.0
    if ($base && $base =~ /^$URI::scheme_re:/o) {
	# already absolute
	return $HTTP::URI_CLASS->new($base);
    }

    my $req = $self->request;
    if ($req) {
        # if $base is undef here, the return value is effectively
        # just a copy of $self->request->uri.
        return $HTTP::URI_CLASS->new_abs($base, $req->uri);
    }

    # can't find an absolute base
    return undef;
}


sub as_string
{
    require HTTP::Status;
    my $self = shift;
    my($eol) = @_;
    $eol = "\n" unless defined $eol;

    my $status_line = $self->status_line;
    my $proto = $self->protocol;
    $status_line = "$proto $status_line" if $proto;

    return join($eol, $status_line, $self->SUPER::as_string(@_));
}


sub is_info     { HTTP::Status::is_info     (shift->{'_rc'}); }
sub is_success  { HTTP::Status::is_success  (shift->{'_rc'}); }
sub is_redirect { HTTP::Status::is_redirect (shift->{'_rc'}); }
sub is_error    { HTTP::Status::is_error    (shift->{'_rc'}); }


sub error_as_HTML
{
    require HTML::Entities;
    my $self = shift;
    my $title = 'An Error Occurred';
    my $body  = HTML::Entities::encode($self->status_line);
    return <<EOM;
<html>
<head><title>$title</title></head>
<body>
<h1>$title</h1>
<p>$body</p>
</body>
</html>
EOM
}


sub current_age
{
    my $self = shift;
    # Implementation of RFC 2616 section 13.2.3
    # (age calculations)
    my $response_time = $self->client_date;
    my $date = $self->date;

    my $age = 0;
    if ($response_time && $date) {
	$age = $response_time - $date;  # apparent_age
	$age = 0 if $age < 0;
    }

    my $age_v = $self->header('Age');
    if ($age_v && $age_v > $age) {
	$age = $age_v;   # corrected_received_age
    }

    my $request = $self->request;
    if ($request) {
	my $request_time = $request->date;
	if ($request_time) {
	    # Add response_delay to age to get 'corrected_initial_age'
	    $age += $response_time - $request_time;
	}
    }
    if ($response_time) {
	$age += time - $response_time;
    }
    return $age;
}


sub freshness_lifetime
{
    my $self = shift;

    # First look for the Cache-Control: max-age=n header
    my @cc = $self->header('Cache-Control');
    if (@cc) {
	my $cc;
	for $cc (@cc) {
	    my $cc_dir;
	    for $cc_dir (split(/\s*,\s*/, $cc)) {
		if ($cc_dir =~ /max-age\s*=\s*(\d+)/i) {
		    return $1;
		}
	    }
	}
    }

    # Next possibility is to look at the "Expires" header
    my $date = $self->date || $self->client_date || time;      
    my $expires = $self->expires;
    unless ($expires) {
	# Must apply heuristic expiration
	my $last_modified = $self->last_modified;
	if ($last_modified) {
	    my $h_exp = ($date - $last_modified) * 0.10;  # 10% since last-mod
	    if ($h_exp < 60) {
		return 60;  # minimum
	    }
	    elsif ($h_exp > 24 * 3600) {
		# Should give a warning if more than 24 hours according to
		# RFC 2616 section 13.2.4, but I don't know how to do it
		# from this function interface, so I just make this the
		# maximum value.
		return 24 * 3600;
	    }
	    return $h_exp;
	}
	else {
	    return 3600;  # 1 hour is fallback when all else fails
	}
    }
    return $expires - $date;
}


sub is_fresh
{
    my $self = shift;
    $self->freshness_lifetime > $self->current_age;
}


sub fresh_until
{
    my $self = shift;
    return $self->freshness_lifetime - $self->current_age + time;
}

1;


__END__

