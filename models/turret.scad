use <lib.scad>

module base() {
	translate([0,0,0.625]) hull() {
		translate([0,0,-0.5]) box([1.8,1.8,0.25]);
		translate([0,0,0.5]) box([0.35,0.35,0.25]);
	}
}

module mountHD($fn=72) {
	translate([0,0,1.3]) union() {
		cyl(0.4,0.4,0.2);
		for (i = [-1:2:1]) translate([0,i*0.41,0]) hull() {
			box([0.2,0.2,0.2]);
			translate([-0.3,0,0.7]) box([0.2,0.2,0.2]);
		}
	}
}

module mountLD($fn=8) {
	translate([0,0,1.3]) union() {
		cyl(0.4,0.4,0.2);
		for (i = [-1:2:1]) translate([0,i*0.41,0]) hull() {
			box([0.2,0.2,0.2]);
			translate([-0.3,0,0.7]) box([0.2,0.2,0.2]);
		}
	}
}

module butt() {
	box([0.6,0.6,0.6]);
}

module barrelHD() {
	rotate([0,0,90]) rotate([90,0,0]) difference() {
		union() {
			for (i = [0:60:300]) rotate([0,0,i]) translate([0,0.12,0]) difference() {
				cyl(0.05,0.05,1.0,$fn=32);
			}
			minkowskiOutsideRound(0.01,$fn=8) translate([0,0,-0.4]) cyl(0.2,0.2,0.2,$fn=64);
			minkowskiOutsideRound(0.01,$fn=8) translate([0,0,0.35]) cyl(0.2,0.2,0.1,$fn=64);
		}
		for (i = [0:60:300]) rotate([0,0,i]) translate([0,0.12,0]) difference() {
			translate([0,0,0.25]) cyl(0.03,0.03,1,$fn=24);
		}
	}
}

module barrelLD($fn=8) {
	rotate([0,0,90]) rotate([90,0,0]) union() {
		for (i = [0:60:300]) rotate([0,0,i]) translate([0,0.12,0]) difference() {
			cyl(0.05,0.05,1.0);
			//translate([0,0,0.5]) cyl(0.03,0.03,0.1);
		}
		translate([0,0,-0.4]) cyl(0.2,0.2,0.2);
		translate([0,0,0.35]) cyl(0.2,0.2,0.1);
	}
}

module barrelVLD($fn=6) {
	rotate([0,0,90]) rotate([90,0,0]) union() {
		cyl(0.18,0.18,1.0);
	}
}

module burstHD($fn=12) {
	hull() {
		sphere(0.03);
		translate([-0.1,0,0]) sphere(0.02);
		translate([2.0,0,0]) box([0.001,0.001,0.001]);
	}
}

module burstLD($fn=6) {
	hull() {
		box([0.06,0.06,0.06]);
		translate([-0.1,0,0]) box([0.04,0.04,0.04]);
		translate([2.0,0,0]) box([0.001,0.001,0.001]);
	}
}

module laserHD() {
	rotate([0,0,90]) rotate([90,0,0]) difference() {
		union() {
			cyl(0.05,0.05,1.0,$fn=24);
			translate([0,0,-0.4]) cyl(0.2,0.2,0.2,$fn=24);
		}
		cyl(0.04,0.04,1.1,$fn=24);
	}
}

module laserLD() {
	rotate([0,0,90]) rotate([90,0,0]) difference() {
		union() {
			cyl(0.05,0.05,1.0,$fn=8);
			translate([0,0,-0.4]) cyl(0.2,0.2,0.2,$fn=8);
		}
	}
}

module laserVLD() {
	rotate([0,0,90]) rotate([90,0,0]) difference() {
		union() {
			cyl(0.05,0.05,1.0,$fn=8);
			translate([0,0,-0.4]) cyl(0.2,0.2,0.2,$fn=8);
			cyl(0.15,0.15,0.6,$fn=8);
		}
	}
}

module laserRingHD() {
	rotate([0,0,90]) rotate([90,0,0]) cyl(0.15,0.15,0.05,$fn=24);
}

module laserRingLD() {
	rotate([0,0,90]) rotate([90,0,0]) cyl(0.15,0.15,0.05,$fn=8);
}

//base();
//fillet(0.01,$fn=8) base($fn=72);
//minkowskiOutsideRound(0.01,$fn=8) mountHD();
//mountLD();
//fillet(0.01,$fn=8) butt($fn=72);
//barrelHD();
//barrelLD();
//barrelVLD();
//burstHD();
//burstLD();
//laserHD();
//laserLD();
//laserVLD();
//laserRingHD();
laserRingLD();

//base();
//butt();
//mountLD();