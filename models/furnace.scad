use <lib.scad>

ld = 16;
hd = 64;

bounds = [3.98,3.98,4];

module tunnel() {
	union() {
		rotate([90,0,0]) cyl(0.6,0.6,1);
		translate([0,0,-0.4]) box([1.2, 1, 0.8]);
	}
}

module fire() {
	translate([0,-1.6,0]) intersection() {
		tunnel();
		box([2,0.1,2]);
	}
}

module smoke() {
	translate([0,0,1.8]) box([1,1,0.1]);
}

module pyramidHD() {
	hull() {
		translate([0,0,-bounds.z/2+0.25]) rbox([bounds.x,bounds.y,0.5],0.01,$fn=8);
		translate([0,0,bounds.z/2-0.25]) rbox([1.4,1.4,0.5],0.01,$fn=8);
	}
}

module pyramidLD() {
	hull() {
		translate([0,0,-bounds.z/2+0.25]) box([bounds.x,bounds.y,0.5]);
		translate([0,0,bounds.z/2-0.25]) box([1.4,1.4,0.5]);
	}
}

module furnaceHD() {
	difference() {
		union() {
			difference() {
				pyramidHD();
				intersection() {
					difference() {
						scale(1.01) pyramidHD();
						scale(0.98) pyramidHD();
					}
					for (i = [0:5]) {
						translate([0,0,i/2-1]) box([5,5,0.05]);
					}
				}
			}
			translate([0,-1.3,0]) union() {
				rotate([90,0,0]) cyl(0.8,0.8,1,$fn=hd);
				translate([0,0,-0.8]) box([1.6, 1, 1.6]);
			}
		}
		translate([0,-1.5,0]) tunnel($fn=hd);
		translate([0,0,2.3]) box([1,1,1]);
	}
}

module furnaceLD() {
	difference() {
		union() {
			pyramidLD();
			translate([0,-1.3,0]) union() {
				rotate([90,0,0]) cyl(0.8,0.8,1,$fn=ld);
				translate([0,0,-0.8]) box([1.6, 1, 1.6]);
			}
		}
		translate([0,-1.5,0]) tunnel($fn=ld);
		translate([0,0,2.3]) box([1,1,1]);
	}
}


//furnaceHD();
furnaceLD();
//hull() furnace($fn=ld);
//fire($fn=hd);
//fire($fn=ld);
//smoke($fn=hd);
//smoke($fn=ld);
