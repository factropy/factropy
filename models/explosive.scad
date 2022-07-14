use <lib.scad>

module stick() {
	cyl(0.05, 0.05, 0.6);
}

module pin() {
	cyl(0.01, 0.01, 0.6);
}

module sticks() {
	union() {
		stick();
		for (i = [0:1:6])
			rotate([0,0,i*60]) translate([0.095,0,0]) stick();
	}
}

module pins() {
	translate([0,0,0.025]) union() {
		pin();
		for (i = [0:1:6])
			rotate([0,0,i*60]) translate([0.1,0,0]) pin();
	}
}

module strap() {
	difference() {
		hull() {
			for (i = [0:1:6])
				rotate([0,0,i*60]) translate([0.1,0,0]) cyl(0.06,0.06,0.1);
		}
		hull() {
			for (i = [0:1:6])
				rotate([0,0,i*60]) translate([0.1,0,0]) cyl(0.05,0.05,0.11);
		}
	}
}

//sticks($fn=12);
//pins($fn=12);
strap($fn=36);
