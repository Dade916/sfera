#!/usr/bin/perl

# Benchmark pack
#$maxDepth = 4.0;
# Sfera pack
$maxDepth = 2.0;
$nameIndex = 0;

sub PrintSphere {
    my $depth = shift;
    my $posx = shift;
    my $posy = shift;
    my $posz = shift;
    my $rad = shift;

    my $k = $depth / $maxDepth;

	print "scene.spheres.sph$nameIndex.geometry=$posx $posy $posz $rad\n";
	print "scene.spheres.sph$nameIndex.static=yes\n";
	if ($depth % 2 == 0) {
		print "scene.spheres.sph$nameIndex.material=mat01\n";
	} else {
		print "scene.spheres.sph$nameIndex.material=mat02\n";
	}

	$nameIndex++;
}

sub HyperSphere {
    my $depth = shift;
    if ($depth <= $maxDepth) {
        my $posx = shift;
        my $posy = shift;
        my $posz = shift;
        my $rad = shift;
        my $direction = shift;

        PrintSphere($depth, $posx, $posy, $posz, $rad);

        my $newRad = $rad / 2.0;
        if ($direction != 0) {
            HyperSphere($depth + 1.0, $posx - $rad - $newRad, $posy, $posz, $newRad, 1);
        }
        if ($direction != 1) {
            HyperSphere($depth + 1.0, $posx + $rad + $newRad, $posy, $posz, $newRad, 0);
        }
        if ($direction != 2) {
            HyperSphere($depth + 1.0, $posx, $posy - $rad - $newRad, $posz, $newRad, 3);
        }
        if ($direction != 3) {
            HyperSphere($depth + 1.0, $posx, $posy + $rad + $newRad, $posz, $newRad, 2);
        }
        if ($direction != 4) {
            HyperSphere($depth + 1.0, $posx, $posy, $posz - $rad - $newRad, $newRad, 5);
        }
        if ($direction != 5) {
            HyperSphere($depth + 1.0, $posx, $posy, $posz + $rad + $newRad, $newRad, 4);
        }
    }
}

# Directions:
# 0 - from -x
# 1 - from +x
# 2 - from -y
# 3 - from +y
# 4 - from -z
# 5 - from +z

HyperSphere(0.0, 0.0, 0.0, 0.0, 15.0, -1);

print "# Sphere count: $nameIndex\n";
