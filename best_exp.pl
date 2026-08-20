#!/usr/bin/perl
use strict;
use warnings;
use File::Copy;
use File::Spec;

# Check if we have at least one directory argument
die "Usage: $0 <dir1> [dir2] ...\n" unless @ARGV;

my $min_value = undef;
my $min_dir = undef;

# Process each directory
foreach my $dir (@ARGV) {
  my $opttimes_file = File::Spec->catfile($dir, "opttimes.!!!");

  unless (-f $opttimes_file) {
    warn "Warning: $opttimes_file not found, skipping\n";
    next;
  }

  # Read the last line
  open(my $fh, '<', $opttimes_file) or do {
    warn "Warning: Cannot open $opttimes_file: $!\n";
    next;
  };

  my $last_line;
  $last_line = $_ while <$fh>;
  close($fh);

  # Parse the second number from format: { 1763353605,  457731 },
  if ($last_line =~ /\{\s*\d+\s*,\s*(\d+)\s*\}/) {
    my $value = $1;

    if (!defined($min_value) || $value < $min_value) {
      $min_value = $value;
      $min_dir = $dir;
    }

    print "Directory: $dir, Value: $value\n";
  } else {
    warn "Warning: Cannot parse last line in $opttimes_file: $last_line\n";
  }
}

# Check if we found a valid directory
die "Error: No valid opttimes.!!! files found\n" unless defined($min_dir);

print "\n=== Minimum found ===\n";
print "Directory: $min_dir\n";
print "Value: $min_value\n\n";

# Delete export.!!! in current directory if it exists
my $current_export = "export.!!!";
if (-f $current_export) {
  unlink($current_export) or warn "Warning: Cannot delete $current_export: $!\n";
  print "Deleted: $current_export\n";
}

# Copy export.!!! from the winning directory
my $source_export = File::Spec->catfile($min_dir, "export.!!!");

unless (-f $source_export) {
  die "Error: $source_export not found\n";
}

copy($source_export, $current_export) or die "Error: Cannot copy file: $!\n";
print "Copied: $source_export -> $current_export\n";
print "Done!\n";
