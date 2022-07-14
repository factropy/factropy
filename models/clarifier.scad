use <lib.scad>

hd = 72;
ld = 16;
vld = 8;

module baseHD() {
	translate([0,0,0.125]) rbox([7.8,7.8,0.25], 0.05, $fn=8);
}

module baseLD() {
	translate([0,0,0.125]) box([7.8,7.8,0.25]);
}

module poolHD() {
	translate([0,0,0.5], $fn=hd) difference() {
		cyl(7.5/2.0, 7.5/2.0, 0.9);
		cyl(7.0/2.0, 7.0/2.0, 0.95);
	}
}

module poolLD() {
	translate([0,0,0.5], $fn=ld) difference() {
		cyl(7.5/2.0, 7.5/2.0, 0.9);
		cyl(7.0/2.0, 7.0/2.0, 0.95);
	}
}

module fluidHD() {
	#translate([0,0,0.9], $fn=hd) difference() {
		cyl(7.0/2.0, 7.0/2.0, 0.1);
	}
}

module fluidLD() {
	#translate([0,0,0.9], $fn=ld) difference() {
		cyl(7.0/2.0, 7.0/2.0, 0.1);
	}
}

module armHD() {
	translate([0,0,0.5], $fn=hd) union() {
		cyl(0.5, 0.5, 0.9);
		rbox([6.9, 0.5, 1], 0.05, $fn=8);
	}
}

module armLD() {
	translate([0,0,0.5], $fn=ld) union() {
		cyl(0.5, 0.5, 0.9);
		box([6.9, 0.5, 1]);
	}
}

//union() {
//	baseLD();
//	poolLD();
//}

//fluidLD();
//armLD();