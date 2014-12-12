package feature;

our $VERSION = '1.11';

my %feature = (
    switch => 'feature_switch',
    say    => "feature_say",
    state  => "feature_state",
);

my %feature_bundle = (
    "5.10.0" => [qw(switch say state)],
);

$feature_bundle{"5.10"} = $feature_bundle{sprintf("%vd",$^V)};

$feature_bundle{"5.9.5"} = $feature_bundle{"5.10.0"};



sub import {
    my $class = shift;
    if (@_ == 0) {
	croak("No features specified");
    }
    while (@_) {
	my $name = shift(@_);
	if (substr($name, 0, 1) eq ":") {
	    my $v = substr($name, 1);
	    if (!exists $feature_bundle{$v}) {
		unknown_feature_bundle($v);
	    }
	    unshift @_, @{$feature_bundle{$v}};
	    next;
	}
	if (!exists $feature{$name}) {
	    unknown_feature($name);
	}
	$^H{$feature{$name}} = 1;
    }
}

sub unimport {
    my $class = shift;

    # A bare C<no feature> should disable *all* features
    if (!@_) {
	delete @^H{ values(%feature) };
	return;
    }

    while (@_) {
	my $name = shift;
	if (substr($name, 0, 1) eq ":") {
	    my $v = substr($name, 1);
	    if (!exists $feature_bundle{$v}) {
		unknown_feature_bundle($v);
	    }
	    unshift @_, @{$feature_bundle{$v}};
	    next;
	}
	if (!exists($feature{$name})) {
	    unknown_feature($name);
	}
	else {
	    delete $^H{$feature{$name}};
	}
    }
}

sub unknown_feature {
    my $feature = shift;
    croak(sprintf('Feature "%s" is not supported by Perl %vd',
	    $feature, $^V));
}

sub unknown_feature_bundle {
    my $feature = shift;
    croak(sprintf('Feature bundle "%s" is not supported by Perl %vd',
	    $feature, $^V));
}

sub croak {
    require Carp;
    Carp::croak(@_);
}

1;
