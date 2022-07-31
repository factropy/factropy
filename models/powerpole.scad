use <lib.scad>

hd = 16;
ld = 8;
vld = 6;

module poleSmallBodyHD($fn=hd) {
	translate([0,0,3]) cyl(0.1, 0.1, 6.0);
}

module poleSmallBaseHD($fn=hd) {
	hull() {
		translate([0,0,0.25]) cyl(0.2, 0.2, 0.5);
		translate([0,0,2]) cyl(0.05, 0.05, 0.5);
		// underground
		cyl(0.2, 0.2, 0.5);
	}
}

module poleSmallHeadHD($fn=hd) {
	translate([0,0,5.75]) cyl(0.15, 0.15, 0.5);
}

module poleSmallBodyLD($fn=ld) {
	translate([0,0,3]) cyl(0.1, 0.1, 6.0);
}

module poleSmallBaseLD($fn=ld) {
	hull() {
		translate([0,0,0.25]) cyl(0.2, 0.2, 0.5);
		translate([0,0,2]) cyl(0.05, 0.05, 0.5);
		// underground
		cyl(0.2, 0.2, 0.5);
	}
}

module poleSmallHeadLD($fn=ld) {
	translate([0,0,5.75]) cyl(0.15, 0.15, 0.5);
}

module poleSmallVLD() {
	translate([0,0,3]) cyl(0.15, 0.15, 6.0, $fn=vld);
}

//poleSmallBodyHD();
//poleSmallBaseHD();
//poleSmallHeadHD();
//poleSmallBodyLD();
//poleSmallBaseLD();
//poleSmallHeadLD();
//poleSmallVLD();

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

//poleLargeHD();
