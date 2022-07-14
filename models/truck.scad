use <lib.scad>

bounds = [2,3,2];
body = [1.8, 2.75, 1.2];

module chassisEngineerHD() {
	difference() {
		union() {
			rbox(body, 0.05, $fn=8);
			translate([0,0,-0.4]) rbox([x(body)*0.5, y(body)*0.8, z(body)], 0.05, $fn=8);
			translate([0,-1,0.45]) cyl(0.3, 0.3, 0.5, $fn=16);
		};
	};
}

module chassisEngineerLD() {
	difference() {
		union() {
			box(body);
			translate([0,0,-0.4]) box([x(body)*0.5, y(body)*0.8, z(body)]);
			translate([0,-1,0.45]) cyl(0.3, 0.3, 0.5, $fn=8);
		}
	}
}

module chassisEngineerVLD() {
	difference() {
		union() {
			box(body);
		};
	};
}

module cabHD() {
	translate([0,-1,0.45]) difference() {
		rbox([1.5,1,0.5], 0.05, $fn=8);
		cyl(0.3, 0.3, 1.0, $fn=16);
	}
}

module cabLD() {
	translate([0,-1,0.45]) difference() {
		box([1.5,1,0.5]);
		cyl(0.3, 0.3, 1.0, $fn=8);
	}
}

module chassisHaulerHD() {
	difference() {
		union() {
			rbox(body, 0.05, $fn=8);
			translate([0,0,-0.4]) rbox([x(body)*0.5, y(body)*0.8, z(body)], 0.05, $fn=8);
			translate([0,-1,0.45]) cyl(0.3, 0.3, 0.5, $fn=16);
		};
		translate([0,0.45,0.8]) rbox([1.5, 1.5, 1.0], 0.05, $fn=8);
	};
}

module chassisHaulerLD() {
	difference() {
		union() {
			box(body);
			translate([0,0,-0.4]) box([x(body)*0.5, y(body)*0.8, z(body)]);
			translate([0,-1,0.45]) cyl(0.3, 0.3, 0.5, $fn=8);
		};
		translate([0,0.45,0.8]) box([1.5, 1.5, 1.0]);
	};
}

module chassisHaulerVLD() {
	difference() {
		union() {
			box(body);
		};
	};
}

module engineerStripeHD() {
	v = 0.325;
	difference() {
		union() {
			translate([0,0,v]) box([1.7,2.6,0.3]);
			intersection() {
				translate([0,0,v]) box([1.82,2.45,0.3]);
				for (y = [2.1:-0.2:-2]) translate([0,y,v]) rotate([30,0,0]) box([2,0.1,0.5]);
			}
			intersection() {
				translate([0,0.14,v]) box([1.5,2.5,0.3]);
				union() {
					for (x = [-0.69:0.2:0]) translate([x,0,v]) rotate([0,30,0]) box([0.1,2.9,0.5]);
					for (x = [0.09:0.2:0.78]) translate([x,0,v]) rotate([0,-30,0]) box([0.1,2.9,0.5]);
				}
			}
		}
		translate([0,0,v]) box([1.5,2.4,0.4]);
	}
}

module engineerStripeLD() {
	v = 0.325;
	difference() {
		union() {
			translate([0,0,v]) box([1.7,2.6,0.3]);
			intersection() {
				translate([0,0,v]) box([1.82,2.45,0.3]);
				for (y = [2.2:-0.4:-2]) translate([0,y,v]) rotate([30,0,0]) box([2,0.2,0.5]);
			}
			intersection() {
				translate([0,0.14,v]) box([1.5,2.5,0.3]);
				union() {
					for (x = [-0.97:0.4:0]) translate([x,0,v]) rotate([0,30,0]) box([0.2,2.9,0.5]);
					for (x = [0.17:0.4:0.78]) translate([x,0,v]) rotate([0,-30,0]) box([0.2,2.9,0.5]);
				}
			}
		}
		translate([0,0,v]) box([1.5,2.4,0.4]);
	}
}

module wheel() {
	rotate([0,90,0]) cyl(0.25, 0.25, 0.35, $fn=24);
}

//cabHD();
//cabLD();
//chassisEngineerHD();
//engineerStripeHD();
engineerStripeLD();
//chassisEngineerLD();
//chassisHaulerHD();
//chassisHaulerLD();
//wheel();