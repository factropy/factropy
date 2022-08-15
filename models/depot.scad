use <lib.scad>

hd = 72;
ld = 12;

module slatsHD() {
	for (i = [0:30:360]) rotate([0,0,i]) translate([0,1.35,1.6]) rbox([0.5,0.2,2.5],0.02,$fn=8);
}

module slatsLD() {
	for (i = [0:30:360]) rotate([0,0,i]) translate([0,1.35,1.6]) box([0.5,0.2,2.5]);
}

module baseHD($fn=hd) {
	translate([0,0,1.5]) difference() {
		union() {
			cyl(1.4, 1.4, 3.0);
			translate([0,0,0.6]) cyl(0.6, 0.6, 2.0,$fn=hd/2);
			translate([0,0,-1.45]) hull($fn=8) {
				translate([0,0,0.15]) cyl(1.4, 1.4, 0.1);
				box([2.975,2.975,0.1]);
			}
		}
		translate([0,0,1]) cyl(0.5,0.5,2.0,$fn=$fn/2);
	}
}

module baseLD($fn=ld) {
	translate([0,0,1.5]) difference() {
		union() {
			cyl(1.4, 1.4, 3.0);
			translate([0,0,0.6]) cyl(0.6, 0.6, 2.0);
			translate([0,0,-1.45]) hull($fn=8) {
				translate([0,0,0.15]) cyl(1.4, 1.4, 0.1);
				box([2.975,2.975,0.1]);
			}
		}
		translate([0,0,1]) cyl(0.5,0.5,2.0);
	}
}

module baseVLD($fn=ld) {
	translate([0,0,1.5]) difference() {
		union() {
			cyl(1.4, 1.4, 3.0);
			translate([0,0,-1.45]) box([2.975,2.975,0.3]);
		}
		translate([0,0,1]) cyl(0.5,0.5,2.0,$fn=$fn/2);
	}
}

slatsHD();
//baseLD();