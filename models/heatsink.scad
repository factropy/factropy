use <lib.scad>

module finsHD() {
	difference() {
		box([0.6,0.6,0.6]);
		for (i = [-3:1:3]) translate([0,0.082*i,0.2]) box([0.7,0.05,0.6]);
		translate([0,0,0.2]) box([0.05,0.7,0.6]);
	}
}

module finsLD() {
	difference() {
		box([0.6,0.6,0.6]);
		for (i = [-1:1:1]) translate([0,0.175*i,0.2]) box([0.7,0.1,0.6]);
	}
}

module baseHD() {
	translate([0,0,-0.2]) union() {
		box([0.63,0.55,0.1]);
		box([0.55,0.63,0.1]);
	}
}

module baseLD() {
	translate([0,0,-0.2])
		box([0.63,0.63,0.1]);
}

//translate([0,0,0.3]) finsHD();
//translate([0,0,0.3]) baseHD();

//translate([0,0,0.3]) finsLD();
translate([0,0,0.3]) baseLD();

