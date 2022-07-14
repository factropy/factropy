use <lib.scad>

module chassisHD() {
	translate([0,0,0.5]) difference() {
		rbox([0.98,0.98,1.0], 0.02, $fn=8);
		translate([0,0.5,0]) box([0.8,0.1,0.8]);
		translate([0,-0.5,0]) box([0.8,0.1,0.8]);
		for (i = [-2:1:2]) translate([0.5,0,i*0.18]) rotate([0,45,0]) box([0.3,0.8,0.06]);
		for (i = [-2:1:2]) translate([-0.5,0,i*0.18]) rotate([0,-45,0]) box([0.3,0.8,0.06]);
	}
}

//chassisHD();

module insetHD() {
	translate([0,0,0.5]) difference() {
		box([0.8,0.95,0.8]);
	}
}

insetHD();

module chassisLD() {
	translate([0,0,0.5]) difference() {
		box([0.98,0.98,1.0]);
		translate([0,0.5,0]) box([0.8,0.1,0.8]);
		translate([0,-0.5,0]) box([0.8,0.1,0.8]);
		for (i = [-2:2:2]) translate([0.5,0,i*0.15]) rotate([0,45,0]) box([0.2,0.8,0.12]);
		for (i = [-2:2:2]) translate([-0.5,0,i*0.15]) rotate([0,-45,0]) box([0.2,0.8,0.12]);
	}
}

//chassisLD();

module chassisVLD() {
	translate([0,0,0.5]) difference() {
		box([0.98,0.98,1.0]);
	}
}

//chassisVLD();
