use <lib.scad>

module chassis() {
	rotate([90,0,0]) union() {
		cyl(0.25, 0.2, 2.0);
		translate([0,0,0.5]) box([1.0, 0.05, 0.5]);
		translate([0,0,-0.5]) box([1.0, 0.05, 0.5]);
		translate([0,0,-0.5]) box([0.05, 1.0, 0.5]);
	};
}

module plume() {
	cyl(0.01, 0.24, 3.0);
}

//chassis($fn=32);
translate([0,2.5,0]) rotate([90,0,0]) plume($fn=12);