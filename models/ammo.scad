use <lib.scad>

module handleHD() {
	union() {
		hull() {
			translate([-0.1,0,0]) rotate([90,0,0]) cyl(0.03, 0.03, 0.03, $fn=12);
			translate([0.1,0,0]) rotate([90,0,0]) cyl(0.03, 0.03, 0.03, $fn=12);
		}
		hull() {
			translate([-0.1,0,-0.1]) box([0.02,0.02,0.02]);
			translate([0.1,0,-0.1]) box([0.02,0.02,0.02]);
		}
		hull() {
			translate([-0.1,0,-0.1]) box([0.02,0.02,0.02]);
			translate([-0.1,0,0]) box([0.02,0.02,0.02]);
		}
		hull() {
			translate([0.1,0,-0.1]) box([0.02,0.02,0.02]);
			translate([0.1,0,0]) box([0.02,0.02,0.02]);
		}
	}
}

module cutoutHD(len) {
	hull() {
		translate([-len/2,0,0]) sphere(r=0.03,$fn=8);
		translate([len/2,0,0]) sphere(r=0.03,$fn=8);
	}
}

module ammoHD() {
	translate([0,0,0.2]) difference() {
		union() {
			rbox([0.35, 0.6, 0.4], 0.01, $fn=8);
			translate([0,0,-0.04]) difference() {
				rbox([0.36, 0.61, 0.5], 0.01, $fn=8);
				translate([0,0,-0.07]) rotate([5,0,0]) box([0.42, 1, 0.5]);
			}
			translate([0,-0.3,0.08]) handleHD();
			translate([0,0.3,0.08]) handleHD();
		}

		translate([0.2,0,-0.05]) rotate([20,0,0]) rotate([0,0,90]) cutoutHD(0.5);
		translate([0.2,0,-0.05]) rotate([-20,0,0]) rotate([0,0,90]) cutoutHD(0.5);
		translate([-0.2,0,-0.05]) rotate([20,0,0]) rotate([0,0,90]) cutoutHD(0.5);
		translate([-0.2,0,-0.05]) rotate([-20,0,0]) rotate([0,0,90]) cutoutHD(0.5);

		for (i = [-2:1:2])
			translate([0,i*0.11,0.23]) cutoutHD(0.2);
	}
}

module ammoLD() {
	translate([0,0,0.2]) difference() box([0.35, 0.6, 0.4]);
}

ammoHD();
//ammoLD();