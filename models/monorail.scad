use <lib.scad>

hd=24;
ld=12;
vld=8;

module container() {
	translate([0,0,1.5]) box([3,7,3]);
}

module containerGap() {
	translate([0,0,1.5]) box([3.02,7.02,3.02]);
}

module towerHD() {
	translate([0,0,6]) union() {
		cyl(0.5,0.5,12,$fn=hd);
		translate([0,0,-5.5]) cyl(1,1,1,$fn=hd);
		translate([0,0,5.5]) box([1,10,1]);
	}
}

module towerLD() {
	translate([0,0,6]) union() {
		cyl(0.5,0.5,12,$fn=ld);
		translate([0,0,-5.5]) cyl(1,1,1,$fn=ld);
		translate([0,0,5.5]) box([1,10,1]);
	}
}

module towerVLD() {
	translate([0,0,6]) union() {
		cyl(0.5,0.5,12,$fn=vld);
		translate([0,0,5.5]) box([1,10,1]);
	}
}

//towerHD();
//towerLD();
//towerVLD();

module tower2HD() {
	translate([0,0,9]) union() {
		cyl(0.5,0.5,18,$fn=hd);
		translate([0,0,-8.5]) cyl(1,1,1,$fn=hd);
		translate([0,0,8.5]) box([1,10,1]);
	}
}

module tower2LD() {
	translate([0,0,9]) union() {
		cyl(0.5,0.5,18,$fn=ld);
		translate([0,0,-8.5]) cyl(1,1,1,$fn=ld);
		translate([0,0,8.5]) box([1,10,1]);
	}
}

module tower2VLD() {
	translate([0,0,9]) union() {
		cyl(0.5,0.5,18,$fn=vld);
		translate([0,0,8.5]) box([1,10,1]);
	}
}

//tower2HD();
//tower2LD();
//tower2VLD();

module stopHD() {
	union() {
		translate([0,0,6]) union() {
			translate([0,4.5,0]) cyl(0.5,0.5,12,$fn=hd);
			translate([0,-4.5,0]) cyl(0.5,0.5,12,$fn=hd);
			translate([0,4.5,-5.5]) cyl(1,1,1,$fn=hd);
			translate([0,-4.5,-5.5]) cyl(1,1,1,$fn=hd);
			translate([0,0,5.5]) box([1,11,1]);
		}
		difference() {
			translate([0,0,11.5]) box([9,10,0.5]);
			translate([-3,0,11]) containerGap();
			translate([3,0,11]) containerGap();
		}
		box([9.1,7.1,0.1]);
	}
}

module stopLD() {
	union() {
		translate([0,0,6]) union() {
			translate([0,4.5,0]) cyl(0.5,0.5,12,$fn=ld);
			translate([0,-4.5,0]) cyl(0.5,0.5,12,$fn=ld);
			translate([0,4.5,-5.5]) cyl(1,1,1,$fn=ld);
			translate([0,-4.5,-5.5]) cyl(1,1,1,$fn=ld);
			translate([0,0,5.5]) box([1,11,1]);
		}
		difference() {
			translate([0,0,11.5]) box([9,10,0.5]);
			translate([-3,0,11]) containerGap();
			translate([3,0,11]) containerGap();
		}
		box([9.1,7.1,0.1]);
	}
}

module stopVLD() {
	union() {
		translate([0,0,6]) union() {
			translate([0,4.5,0]) cyl(0.5,0.5,11.5,$fn=ld);
			translate([0,-4.5,0]) cyl(0.5,0.5,11.5,$fn=ld);
//			translate([0,4.5,-5.5]) cyl(1,1,1,$fn=ld);
//			translate([0,-4.5,-5.5]) cyl(1,1,1,$fn=ld);
//			translate([0,0,5.5]) box([1,11,1]);
		}
		difference() {
			translate([0,0,11.5]) box([9,10,0.5]);
//			translate([-3,0,11]) containerGap();
//			translate([3,0,11]) containerGap();
		}
//		box([9.1,7.1,0.1]);
	}
}

//stopHD();
//stopLD();
//stopVLD();

module carHD() {
	union() {
		intersection() {
			difference() {
				translate([0,0,-0.5]) rbox([10,3,2], 0.05, $fn=8);
				translate([0,0,1]) box([7.1,4,4]);
			}
		}
		translate([0,1,-1.5]) rbox([8,0.51,1], 0.05, $fn=8);
		translate([0,-1,-1.5]) rbox([8,0.51,1], 0.05, $fn=8);
		translate([4.4,0,0.15]) cyl(0.3, 0.3, 1.0, $fn=16);
	}
}

module cabHD() {
	translate([4.4,0,0.15]) difference() {
		rbox([1.5,2.8,1.0], 0.05, $fn=8);
		cyl(0.3, 0.3, 2.0, $fn=16);
	}
}

module stripeHD() {
	translate([0,0,-1.25]) rbox([10.1,3.1,0.35], 0.05, $fn=8);
}

module stripeLD() {
	translate([0,0,-1.25]) box([10.1,3.1,0.35]);
}

module carLD() {
	union() {
		intersection() {
			difference() {
				translate([0,0,-0.5]) box([10,3,2]);
				translate([0,0,1]) box([7.1,4,4]);
			}
		}
		translate([0,1,-1.5]) box([8,0.51,1]);
		translate([0,-1,-1.5]) box([8,0.51,1]);
		translate([4.4,0,0.15]) cyl(0.3, 0.3, 1.0, $fn=8);
	}
}

module cabLD() {
	translate([4.4,0,0.15]) difference() {
		box([1.5,2.8,1.0]);
		cyl(0.3, 0.3, 2.0, $fn=8);
	}
}

//carHD();
//carLD();
//cabHD();
//cabLD();
//stripeHD();
stripeLD();
//rotate([0,0,90]) translate([0,0,-0.95]) container();
