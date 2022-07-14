use <lib.scad>

module aerial() {
	union() {
		hull() {
			translate([0,0,0.2]) sphere(0.01);
			#translate([0,0,0.05]) cyl(0.015, 0.015, 0.1);
		}
		hull() {
			translate([0,0,0.3]) sphere(0.01);
			translate([0,0,0.2]) sphere(0.01);
		}
	}
}

module base() {
	translate([0,0,0.01]) cyl(0.05, 0.05, 0.02);
}

aerial($fn=6);
base($fn=6);