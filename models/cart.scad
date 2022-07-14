use <lib.scad>

hd = 24;
ld = 12;

module wheel() {
	rotate([0,90,0]) cyl(0.25, 0.25, 0.35);
}

module wheelHD() {
	wheel($fn=hd);
}

module wheelLD() {
	wheel($fn=ld);
}

module wheels() {
	translate([0.6,0.6,0.25]) wheel();
	translate([0.6,-0.6,0.25]) wheel();
	translate([-0.6,-0.6,0.25]) wheel();
	translate([-0.6,0.6,0.25]) wheel();
}

module cabHD() {
	translate([0,0.55,0.85]) difference() {
		rbox([1.1, 0.9, 0.5], 0.05, $fn=8);
		cyl(0.3, 0.3, 1.0, $fn=16);
	}
}

module cabLD() {
	translate([0,0.55,0.85]) difference() {
		box([1.1, 0.9, 0.5]);
		cyl(0.3, 0.3, 1.0, $fn=8);
	}
}

module chassisHaulerHD() {
	translate([0,0,0.6], $fn=hd) difference() {
		union() {
			rbox([1.5, 1.8, 0.8], 0.05, $fn=8);
			translate([0,0.55,0.25]) cyl(0.3, 0.3, 0.5, $fn=16);
		}
		translate([0,-0.35,0.35]) rbox([1.1, 0.8, 0.5], 0.05, $fn=8);
		translate([0.6,0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([0.6,-0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([-0.6,-0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([-0.6,0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
	};
}

module chassisHaulerLD() {
	translate([0,0,0.6], $fn=ld) difference() {
		union() {
			box([1.5, 1.8, 0.8]);
			translate([0,0.55,0.25]) cyl(0.3, 0.3, 0.5, $fn=8);
		}
		translate([0,-0.35,0.35]) box([1.1, 0.8, 0.5]);
		translate([0.6,0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([0.6,-0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([-0.6,-0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([-0.6,0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
	};
}

module chassisEngineerHD() {
	translate([0,0,0.6], $fn=hd) difference() {
		union() {
			rbox([1.5, 1.8, 0.8], 0.05, $fn=8);
			translate([0,0.55,0.25]) cyl(0.3, 0.3, 0.5, $fn=16);
		}
		translate([0.6,0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([0.6,-0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([-0.6,-0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([-0.6,0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
	};
}

module chassisEngineerLD() {
	translate([0,0,0.6], $fn=ld) difference() {
		union() {
			box([1.5, 1.8, 0.8]);
			translate([0,0.55,0.25]) cyl(0.3, 0.3, 0.5, $fn=8);
		}
		translate([0.6,0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([0.6,-0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([-0.6,-0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
		translate([-0.6,0.6,-0.35]) scale([1.1,1.1,1.1]) wheel();
	};
}

module engineerStripeHD() {
	difference() {
		union() {
			translate([0,0,0.75]) box([1.4,1.7,0.3]);
			intersection() {
				translate([0,0,0.75]) box([1.52,1.55,0.3]);
				for (y = [1:-0.2:-1]) translate([0,y,0.75]) rotate([30,0,0]) box([2,0.1,0.5]);
			}
			intersection() {
				translate([0,-0.1,0.75]) box([1.25,1.62,0.3]);
				union() {
					for (x = [-0.69:0.2:0]) translate([x,0,0.75]) rotate([0,30,0]) box([0.1,2,0.5]);
					for (x = [0.09:0.2:0.78]) translate([x,0,0.75]) rotate([0,-30,0]) box([0.1,2,0.5]);
				}
			}
		}
		translate([0,0,0.75]) box([1.3,1.6,1]);
	}
}

module engineerStripeLD() {
	difference() {
		union() {
			translate([0,0,0.75]) box([1.4,1.7,0.3]);
			intersection() {
				translate([0,0,0.75]) box([1.52,1.55,0.3]);
				for (y = [1:-0.4:-1]) translate([0,y,0.75]) rotate([30,0,0]) box([2,0.2,0.5]);
			}
			intersection() {
				translate([0,-0.1,0.75]) box([1.25,1.62,0.3]);
				union() {
					for (x = [-0.5:0.4:0]) translate([x,0,0.75]) rotate([0,30,0]) box([0.2,2,0.5]);
					for (x = [0.1:0.4:0.68]) translate([x,0,0.75]) rotate([0,-30,0]) box([0.2,2,0.5]);
				}
			}
		}
		translate([0,0,0.75]) box([1.3,1.6,1]);
	}
}

//chassisEngineerHD();
//chassisEngineerLD();
engineerStripeHD();
//engineerStripeLD();
//chassisHaulerHD();
//chassisHaulerLD();
//cabHD();
//cabLD();
//wheelHD();
//wheelLD();