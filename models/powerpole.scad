use <lib.scad>

module poleSmallHD() {
	translate([0,0,3]) cyl(0.1, 0.1, 6.0, $fn=8);
}

module poleLargeHD() {
	for (i = [0:90:270]) rotate([0,0,i]) hull() {
		translate([1.4,1.4,0]) box([0.2,0.2,0.2]);
		translate([0.8,0.8,4]) box([0.2,0.2,0.2]);
	}

	translate([0,0,4]) difference() {
		box([1.8,1.8,0.1]);
		box([1.3,1.3,0.2]);
	}

	for (i = [0:90:270]) rotate([0,0,i]) hull() {
		translate([0.8,0.8,4]) box([0.2,0.2,0.2]);
		translate([0.6,0.6,8]) box([0.2,0.2,0.2]);
	}

	translate([0,0,8]) difference() {
		box([1.4,1.4,0.1]);
		box([0.9,0.9,0.2]);
	}

	for (i = [0:90:270]) rotate([0,0,i]) hull() {
		translate([0.6,0.6,8]) box([0.2,0.2,0.2]);
		translate([0.2,0.2,12]) box([0.2,0.2,0.2]);
	}

	translate([0,0,12]) difference() {
		box([0.6,0.6,0.1]);
		box([0.38,0.38,0.2]);
	}
}

poleLargeHD();
