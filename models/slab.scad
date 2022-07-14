use <lib.scad>

module pinHD() {
	hull() { cyl(0.24,0.24,0.965,$fn=16); cyl(0.2,0.2,1,$fn=16); }
}

module slabPavedHD() {
	translate([0,0,-0.48]) intersection() {
		union() {
			difference() {
				box([12,12,1]);
				for (i = [-6:2:6]) translate([0,i,0.57]) scale([1,2,1]) rotate([30,0,0]) rotate([0,90,0]) cyl(0.1, 0.1, 13, $fn=6);
				rotate([0,0,90]) for (i = [-6:2:6]) translate([0,i,0.57]) scale([1,2,1]) rotate([30,0,0]) rotate([0,90,0]) cyl(0.1, 0.1, 13, $fn=6);
			}
			translate([6,6,0]) pinHD();
			translate([-6,6,0]) pinHD();
			translate([6,-6,0]) pinHD();
			translate([-6,-6,0]) pinHD();
		}
		box([12,12,1]);
	}
}

module slabPavedLD() {
	translate([0,0,-0.48]) difference() {
		box([12,12,1]);
		for (i = [-6:4:6]) translate([0,i,0.575]) scale([1,2.5,1]) rotate([30,0,0]) rotate([0,90,0]) cyl(0.1, 0.1, 13, $fn=6);
		rotate([0,0,90]) for (i = [-6:4:6]) translate([0,i,0.575]) scale([1,2.5,1]) rotate([30,0,0]) rotate([0,90,0]) cyl(0.1, 0.1, 13, $fn=6);
	}
}

module slabPavedVLD() {
	translate([0,0,-0.48]) difference() {
		box([12,12,1]);
	}
}

//slabPavedHD();
//slabPavedLD();
slabPavedVLD();
