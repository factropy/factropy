use <lib.scad>

hd = 72;
ld = 12;

module bodyHD() {
	translate([0,0,2.5]) union() {
		box([3,15,3]);
		translate([0,-6,1.025]) box([3.1,3.1,1]);
	}
}

bodyHD();
